#include "VideoDecoder.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdint.h>
#include <mutex>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <inttypes.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
};
#include "../logger.h"
#include "xfltransfer.h"
#include "../MediaFormat.h"
#include "../protocol.h"

LOGGER_Declare("videodecoder");

extern std::mutex g_ffmpeg_open_lock;

VideoDecoder::VideoDecoder(void)
{
	m_avcodec = NULL;
	m_avstream = NULL;
	//m_avpacket = NULL;
}

VideoDecoder::~VideoDecoder(void)
{
}

static void _avpriv_set_pts_info(AVStream *s, int pts_wrap_bits,
	unsigned int pts_num, unsigned int pts_den)
{
	AVRational new_tb;
	if (av_reduce(&new_tb.num, &new_tb.den, pts_num, pts_den, INT_MAX)) {
		if (new_tb.num != pts_num)
			LOGGER_Debug("st:%d removing common factor %d from timebase",
			s->index, pts_num / new_tb.num);
	}
	else{
		LOGGER_Warn("st:%d has too large timebase, reducing", s->index);
	}
	if (new_tb.num <= 0 || new_tb.den <= 0) {
		LOGGER_Error("Ignoring attempt to set invalid timebase %d/%d for st:%d\n",
			new_tb.num, new_tb.den,
			s->index);
		return;
	}
	s->time_base = new_tb;
#if FF_API_LAVF_AVCTX
	//FF_DISABLE_DEPRECATION_WARNINGS
		s->codec->pkt_timebase = new_tb;
	//FF_ENABLE_DEPRECATION_WARNINGS
#endif
	//	s->internal->avctx->pkt_timebase = new_tb;
	s->pts_wrap_bits = pts_wrap_bits;
}

#define INVALID_FIRST_FRAME_NO	0xFFFFFFFFFFFFFFFF

bool VideoDecoder::Create(int codecType, int width, int height, int fps, int gop)
{
	m_firstICount = INVALID_FIRST_FRAME_NO;
	m_lastICount = 0;
	m_lastPtsMs = 0;

	m_width = width;
	m_height = height;
	m_lastPtsMs = 0;
	m_fps = fps;
	//m_gop = gop;

	AVCodecID codecId;
	switch (codecType) {
	case VIDEO_TYPE_H265:
		codecId = AV_CODEC_ID_H265;
		break;
	default:
		codecId = AV_CODEC_ID_H264;
		break;
	}
#if 0
	if (m_avpacket == NULL) {
		m_avpacket = (AVPacket*)malloc(sizeof(AVPacket));
		memset(m_avpacket, 0, sizeof(AVPacket));
	}
#endif

	int i, ret;
	AVCodec *codec = avcodec_find_decoder(codecId);
	if (!codec) {
		return NULL;
	}

	AVStream *avstream = (AVStream*)av_mallocz(sizeof(AVStream));
	if (!avstream) {
		return false;
	}

	avstream->codec = avcodec_alloc_context3(codec);
	if (!avstream->codec) {
		av_free(avstream);
		return false;
	}
	//avcodec_close(avstream->codec);
	//av_free(avstream->codec);
	avstream->id = 0;
	avstream->index = 0;
	avstream->start_time = AV_NOPTS_VALUE;
	avstream->duration = AV_NOPTS_VALUE;
	avstream->cur_dts = 0;
	avstream->first_dts = AV_NOPTS_VALUE;
	_avpriv_set_pts_info(avstream, 1000 / fps, 1, 90000);

	avstream->last_IP_pts = AV_NOPTS_VALUE;
	for (i = 0; i<MAX_REORDER_DELAY + 1; i++) {
		avstream->pts_buffer[i] = AV_NOPTS_VALUE;
	}
	// avstream->reference_dts = AV_NOPTS_VALUE;
	avstream->sample_aspect_ratio.num = 0;
	avstream->sample_aspect_ratio.den = 1;
	avstream->probe_packets = 2;

	avstream->codec->codec_type = AVMEDIA_TYPE_VIDEO;
	avstream->codec->codec_id = codecId;
	if (codec->capabilities&AV_CODEC_CAP_TRUNCATED) {
		avstream->codec->flags |= AV_CODEC_FLAG_TRUNCATED;
	}
	// avcodec_open2 func 非线程安全
	g_ffmpeg_open_lock.lock();
	ret = avcodec_open2(avstream->codec, codec, NULL);
	g_ffmpeg_open_lock.unlock();
	if (ret != 0) {
		avcodec_close(avstream->codec);
		av_free(avstream->codec);
		av_free(avstream);
		return false;
	}
	m_avcodec = codec;
	m_avstream = avstream;
	return true;
}

void VideoDecoder::Destroy()
{
	AVStream *avstream;
#if 0
	if (m_avpacket != NULL) {
		if (m_avpacket->data) {
			av_free_packet(m_avpacket);
			m_avpacket->data = NULL;
		}
		free(m_avpacket);
		m_avpacket = NULL;
	}
#endif
	if( (avstream=m_avstream) != NULL ){
		m_avstream = NULL;
		if( avstream->codec ){
			avcodec_close(avstream->codec);
			av_free(avstream->codec);
		}
		av_free(avstream);
	}
}

bool VideoDecoder::Decode(unsigned char *data, int len, int frameType, uint32_t frameNo,
	h26xDecodeDataCallback func, void *userdata)
{
	//AVFrame picture;
	video_frame_data_t vfd;
	AVPacket avpkt;
	AVFrame  avfrm;
	memset(&avfrm, 0, sizeof(avfrm));
	av_init_packet(&avpkt);
	avpkt.data = data;
	avpkt.size = len;

	if (frameType == XT_FRAME_TYPE_KEY) {
		if (INVALID_FIRST_FRAME_NO == m_firstICount) {
			m_firstICount = frameNo;
			m_lastICount = frameNo;
		}
		else {
			if (frameNo < (m_lastICount & 0xFFFFFFFF)) {
				m_lastICount = (m_lastICount & 0xFFFFFFFF00000000) + 0x100000000 + frameNo;
			}
			else {
				m_lastICount = (m_lastICount & 0xFFFFFFFF00000000) + frameNo;
			}
		}
		LOGGER_Debug("set i frame number %lld", m_lastICount - m_firstICount);
	}
	vfd.width = m_width;
	vfd.height = m_height;

	AVStream *avstream = m_avstream;
	if (avstream != NULL){
		int ret, f, fgot = 0;
		AVSubtitle subtitle;
		AVFrame *pframe = &avfrm;
		do {
			f = 0;
			//avcodec_get_frame_defaults(pframe);
			switch (avstream->codec->codec_type){
			case AVMEDIA_TYPE_VIDEO:
				if (avstream->codec->max_pixels < avstream->codec->width * avstream->codec->height) {
					avstream->codec->max_pixels = avstream->codec->width * avstream->codec->height;
				}
				ret = avcodec_decode_video2(avstream->codec, pframe, &f, &avpkt);
				if (ret >= 0 && f){
					fgot = 1;
					//i帧计算时间
					switch (pframe->pict_type) {
					case AV_PICTURE_TYPE_I:
						m_lastPtsMs = (m_lastICount- m_firstICount) * 1000 / m_fps;
						LOGGER_Debug("decoder i frame time %lld i count %lld fps %d", 
							m_lastPtsMs, m_lastICount- m_firstICount, m_fps);
						break;
					case AV_PICTURE_TYPE_P:
					case AV_PICTURE_TYPE_B:
						m_lastPtsMs += 1000 / m_fps;
						LOGGER_Debug("decoder frame time %lld i count %lld fps %d", 
							m_lastPtsMs, m_lastICount- m_firstICount, m_fps);
						break;
					default:
						break;
					}
					//只考虑YUV420数据
					vfd.data[3] = pframe->data[0];
					vfd.line[3] = pframe->linesize[0];
					if (pframe->data[1] != NULL) {
						vfd.data[0] = pframe->data[0];
						vfd.line[0] = pframe->linesize[0];
						vfd.data[1] = pframe->data[1];
						vfd.line[1] = pframe->linesize[1];
						vfd.data[2] = pframe->data[2];
						vfd.line[2] = pframe->linesize[2];
					}
					else {
						vfd.data[0] = pframe->data[0];
						vfd.line[0] = pframe->linesize[0];
						vfd.data[1] = vfd.data[0] + pframe->linesize[0] * pframe->height;
						vfd.line[1] = vfd.line[0]/2;
						vfd.data[2] = vfd.data[1] + (pframe->linesize[0] * pframe->height) / 4;
						vfd.line[2] = vfd.line[1];
					}
					func(userdata, m_lastPtsMs, &vfd);
					// 需要处理一个Packet包含多个数据帧的情况
					avpkt.data += ret;
					avpkt.size -= ret;
				}
				break;
			case AVMEDIA_TYPE_SUBTITLE:
				ret = avcodec_decode_subtitle2(avstream->codec, &subtitle, &f, &avpkt);
				avsubtitle_free(&subtitle);
				break;
			default:
				break;
			}
		} while (avpkt.size>0 && f);

		return fgot;
	}
	return false;
}

