#include "VideoEncoder.h"
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

LOGGER_Declare("videoencoder");

std::mutex g_ffmpeg_open_lock;

int registerFfmpeg() {
	av_register_all();
	avcodec_register_all();
	return 0;
}
void unregisterFfmpeg() {
}

VideoEncoder::VideoEncoder()
	: m_codec(NULL)
	, m_context(NULL)
{
	m_cache.pdata = NULL;
	m_cache.size = 0;
}


VideoEncoder::~VideoEncoder()
{
}


bool VideoEncoder::start(int32_t width, int32_t height, int32_t fps) {
	AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (codec == NULL) {
		LOGGER_Error("%s", "didnot find h264 encoder");
		return false;
	}
	if (!checkSize(width, height)) {
		LOGGER_Error("checkSIze failed.(%d,%d)", width, height);
		return false;
	}
	m_frameNo = 0;
	m_encFrameNo = 0;
	m_codec = codec;
	m_fps = fps;
	m_pixelFormat = AV_PIX_FMT_YUV420P;
	if (!makeContext(width, height)) {
		LOGGER_Error("makeContext failed.(%d,%d)", width, height);
		return false;
	}
	return true;
}

void VideoEncoder::stop() {
	if (m_context) {
		avcodec_close(m_context);
		avcodec_free_context(&m_context);
		m_context = NULL;
	}
	if (m_cache.pdata != NULL) {
		free(m_cache.pdata);
		m_cache.pdata = NULL;
		m_cache.size = 0;
	}
}

bool VideoEncoder::checkSize(int width, int height) {
	int size = ((width * height) * 2);
	if (m_cache.size < size) {
		uint8_t *d = (uint8_t*)malloc(size);
		if (d == NULL)	return false;
		if (m_cache.pdata != NULL) {
			free(m_cache.pdata);
		}
		m_cache.pdata = d;
		m_cache.size = size;
	}
	return true;
}

bool VideoEncoder::makeContext(int width, int height) {
	int ret;
	AVRational rate;
	AVCodecContext *cc;

	//AVDictionary *opts = NULL;
	//av_dict_set(&opts, "b", "2.5M", 0);
	cc = avcodec_alloc_context3(m_codec);
	if (cc == NULL) {
		LOGGER_Error("alloc context3 failed (%d,%d)", width, height);
		return false;
	}
	cc->width = width;
	cc->height = height;//   
						// frames per second   
	rate.num = 1;
	rate.den = m_fps;
	cc->time_base = rate;//(AVRational){1,25};  
	cc->gop_size = m_fps; // emit one intra frame every ten frames   
	cc->max_b_frames = 1;
	cc->thread_count = 1;
	//cc->refs = 2;
	cc->pix_fmt = (AVPixelFormat)m_pixelFormat;//PIX_FMT_RGB24;  

	cc->bit_rate = 300000;// put sample parameters   2.4Mbits/s
	cc->bit_rate_tolerance = cc->bit_rate;
	cc->rc_min_rate = cc->bit_rate / 2;
	cc->rc_max_rate = cc->bit_rate + cc->bit_rate / 2;
	cc->rc_buffer_size = cc->bit_rate + cc->bit_rate;
	//cc->rc_initial_buffer_occupancy = cc->rc_buffer_size * 3 / 4;
	//cc->rc_buffer_aggressivity = (float)1.0;
	//cc->rc_initial_cplx = 0.5;

	cc->qcompress = (float)0.6; // qcomp=0.6
	cc->qmin = 10;   // qmin=10
	cc->qmax = 30;   // qmax=51 默认 51 会导致画面质量下降厉害
	//cc->max_qdiff = 4;

	if (cc->priv_data != NULL) {
		//av_opt_set(cc->priv_data, "b", "300000", 0);
		//编码加快，意味着信息丢失越严重，输出图像质量越差。
		//ultrafast  superfast veryfast faster fast medium slow slower veryslow
		av_opt_set(cc->priv_data, "preset", "superfast", 0);
		//av_opt_set(cc->priv_data, "preset", "medium", 0);
		// 实时编码关键看这句，上面那条无所谓  
		//film animation grain stillimage psnr ssim fastdecoder zerolatency
		av_opt_set(cc->priv_data, "tune", "zerolatency", 0);
		//main high
		av_opt_set(cc->priv_data, "profile", "main", 0);
		//av_opt_set(cc->priv_data, "profile", "high", 0);

		//CRF(Constant Rate Factor) : 范围 0 - 51 : 0是编码毫无丢失信息, 23 is 默认, 51 是最差的情况。相对合理的区间是18 - 28.
		//值越大，压缩效率越高，但也意味着信息丢失越严重，输出图像质量越差。
		av_opt_set_int(cc->priv_data, "crf", 23, AV_OPT_SEARCH_CHILDREN);
		//av_opt_set_int(cc->priv_data, "qp", 23, AV_OPT_SEARCH_CHILDREN);
	}

	//avcodec_open2 是否要加锁? 
	g_ffmpeg_open_lock.lock();
	ret = avcodec_open2(cc, m_codec, NULL);
	g_ffmpeg_open_lock.unlock();
	//ret = avcodec_open2(cc, m_params.codec, &opts);
	if (ret < 0) {
		LOGGER_Error("open codec failed %d.(%d,%d)", ret, width, height);
		avcodec_free_context(&cc);
		return false;
	}
	if (m_context != NULL) {
		avcodec_close(m_context);
		avcodec_free_context(&m_context);
	}
	m_context = cc;
	return true;
}

int parseH26xFrames(bool h264, unsigned char *pdata, int datalen, h26x_frame_info_t *frames, int size) {
	uint32_t xo = 0xFF;
	int count = 0;
	for (int i = 0; i < datalen && count < size; i++) {
		xo = (xo << 8) | pdata[i];
		if ((xo & 0xFFFFFF) == 0x000001) {
			if (i > 2) {
				frames[count].code = pdata[i - 3] == 0 ? 4 : 3;
			}
			else {
				frames[count].code = 3;
			}
			frames[count].pdata = pdata + i + 1 - frames[count].code;
			//1frame 
			if (h264) {
				switch (pdata[i + 1] & 0x1F) {
				case 0x05:
					frames[count].type = XT_FRAME_TYPE_KEY;
					break;
				case 0x06:
					frames[count].type = XT_FRAME_TYPE_EXTEND;
					break;
				case 0x07:	//sps pps idr sei一起
					frames[count].type = XT_FRAME_TYPE_CORE;
					break;
				case 0x08:
					frames[count].type = XT_FRAME_TYPE_CORE;
					break;
				default:
					frames[count].type = XT_FRAME_TYPE_DATA;
					break;
				}
			}
			else {
				//265
				switch ((pdata[i + 1] & 0x7E) >> 1) {
				case 39:
					frames[count].type = XT_FRAME_TYPE_EXTEND;
					break;
				case 33:
					frames[count].type = XT_FRAME_TYPE_CORE;
					break;
				case 34:
					frames[count].type = XT_FRAME_TYPE_CORE;
					break;
				case 19:
					frames[count].type = XT_FRAME_TYPE_KEY;
					break;
				default:
					frames[count].type = XT_FRAME_TYPE_DATA;
					break;
				}
			}
			if (count > 0) {
				frames[count - 1].len = frames[count].pdata - frames[count - 1].pdata;
			}
			count++;
		}
	}
	if (count > 0) {
		frames[count - 1].len = datalen - (frames[count - 1].pdata - pdata);
	}
	return count;
}

bool VideoEncoder::inputFrame(unsigned char *srcData, int width, int stride, int origHeight,
	h26xEncoderCallback func, void *userData) {
	AVFrame  avfrm = { 0 };
	AVPacket avpkt;
	uint64_t curFrame;
	int ret;
	int got_packet_ptr = 0;
	int height = origHeight > 0 ? origHeight : -origHeight;
	//ffmpeg要求 宽 和 高 都是偶数
	if (m_context == NULL) {
		return true;
	}

	if (!checkSize(width, height)) {
		LOGGER_Error("checkSIze failed.(%d,%d)", width, height);
		return false;
	}

	if (m_context->width != width ||
		m_context->height != height) {
		if (!makeContext(width, height)) {
			LOGGER_Error("makeContext failed.(%d,%d)", width, height);
			return false;
		}
	}

	ret = avpicture_fill((AVPicture*)&avfrm, (uint8_t*)srcData, (AVPixelFormat)m_pixelFormat, width, height);
	if (ret < 0) {
		LOGGER_Error("fill data to frame failed %d.", ret);
		return false;
	}

	curFrame = m_frameNo++;
	av_init_packet(&avpkt);
	avpkt.data = m_cache.pdata;
	avpkt.size = m_cache.size;
	avfrm.width = width;
	avfrm.height = height;
	avfrm.pts = curFrame * 1000 / m_fps;
	if ((curFrame % m_fps) == 0) {
		avfrm.key_frame = 1;
		avfrm.pict_type = AV_PICTURE_TYPE_I;
	}
	else {
		avfrm.key_frame = 0;
		avfrm.pict_type = AV_PICTURE_TYPE_P;
	}
	//DWORD dwStart = GetTickCount();
	ret = avcodec_encode_video2(m_context, &avpkt, &avfrm, &got_packet_ptr);
	//printf("Encoder Cost %d ms\r\n", GetTickCount() - dwStart);
	if (ret >= 0 && got_packet_ptr) {
		h26x_frame_info_t frames[32];
		int count = parseH26xFrames(true, avpkt.data, avpkt.size, frames, 32);
		if (count > 0) {
			for (int i = 0; i < count; i++) {
				switch (frames[i].type) {
				case XT_FRAME_TYPE_CORE:
					//sps pps 一起
					func(userData, frames[i].pdata, frames[i].len+ frames[i+1].len, frames[i].type, m_encFrameNo);
					i++;
					break;
				case XT_FRAME_TYPE_KEY:
					func(userData, frames[i].pdata, frames[i].len, frames[i].type, m_encFrameNo);
					LOGGER_Debug("****send i frame %lld", m_encFrameNo);
					m_encFrameNo += m_fps;
					break;
				//case XT_FRAME_TYPE_DATA:
				//case XT_FRAME_TYPE_DATA_LOW:
				default:
					func(userData, frames[i].pdata, frames[i].len, frames[i].type, m_encFrameNo);
					break;
				}
			}//end for 
		}
	}

	return true;
}