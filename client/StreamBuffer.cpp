#include "StreamBuffer.h"
#include <memory.h>

#pragma pack(1)
struct stream_packet_inner_header_t {
	uint32_t mark : 1;	//是否分包了
	uint32_t len  : 31;
};
#pragma pack()

StreamBuffer::StreamBuffer()
{
}

StreamBuffer::~StreamBuffer()
{
}

bool StreamBuffer::create(uint32_t bufSize, uint32_t maxPacketSize) {
	m_pbuffer = (uint8_t *)malloc(bufSize + maxPacketSize);
	if (m_pbuffer == NULL) {
		return false;
	}
	m_bufSize = bufSize;
	m_maxPacketSize = maxPacketSize;
	m_readed = 0;
	m_writed = 0;
	return true;
}

void StreamBuffer::destroy() {
	if (m_pbuffer != NULL) {
		free(m_pbuffer);
		m_pbuffer = NULL;
	}
	m_readed = 0;
	m_writed = 0;
}

//4字节对齐
#define dataPadSize(x)	(((x)&3)!=0 ? (4-((x)&3)) : 0)
#if 0
bool StreamBuffer::putPacket(const uint8_t *pdata, uint32_t dataLen) {
	uint32_t padSize = dataPadSize(dataLen);
	uint32_t totalLen = dataLen + padSize + sizeof(stream_packet_inner_header_t);
	if (m_bufSize < totalLen + (m_writed - m_readed)) {
		return false;
	}
#if 0
	if (totalLen > m_maxPacketSize) {
		alert("data len error");
	}
#endif
	//判断是否在外部
	uint32_t startpos = (m_writed & (m_bufSize - 1));
	uint8_t *wr = m_pbuffer + startpos;
	stream_packet_inner_header_t *ph = (stream_packet_inner_header_t *)wr;
	if (startpos + totalLen > m_bufSize) {
		ph->mark = 1;
		ph->len = dataLen;
		memcpy(wr + sizeof(stream_packet_inner_header_t), pdata, dataLen);
		m_writed += m_bufSize - startpos;
	}
	else {
		ph->mark = 0;
		ph->len = dataLen;
		memcpy(wr + sizeof(stream_packet_inner_header_t), pdata, dataLen);
		m_writed += totalLen;
	}
	return true;
}
#endif

bool StreamBuffer::putPacket(const stream_frame_t *frame, const uint8_t *pdata, uint32_t dataLen) {
	uint32_t padSize = dataPadSize(dataLen);
	uint32_t totalLen = dataLen + padSize + sizeof(stream_frame_t) + sizeof(stream_packet_inner_header_t);
	if (m_bufSize < totalLen + (m_writed - m_readed)) {
		return false;
	}
#if 0
	if (totalLen > m_maxPacketSize) {
		alert("data len error");
	}
#endif
	//判断是否在外部
	uint32_t startpos = (m_writed & (m_bufSize - 1));
	uint8_t *wr = m_pbuffer + startpos;
	stream_packet_inner_header_t *ph = (stream_packet_inner_header_t *)wr;
	if (startpos + totalLen > m_bufSize) {
		ph->mark = 1;
		ph->len = dataLen;
		memcpy(wr + sizeof(stream_packet_inner_header_t), frame, sizeof(stream_frame_t));
		memcpy(wr + sizeof(stream_packet_inner_header_t) + sizeof(stream_frame_t), pdata, dataLen);
		m_writed += m_bufSize - startpos;
	}
	else {
		ph->mark = 0;
		ph->len = dataLen;
		memcpy(wr + sizeof(stream_packet_inner_header_t), frame, sizeof(stream_frame_t));
		memcpy(wr + sizeof(stream_packet_inner_header_t) + sizeof(stream_frame_t), pdata, dataLen);
		m_writed += totalLen;
	}
	return true;
}


uint8_t *StreamBuffer::getPacket(stream_frame_t **frame, uint32_t *dataLen) {
	uint32_t startpos = (m_readed & (m_bufSize - 1));
	uint8_t *rd = m_pbuffer + startpos;
	stream_packet_inner_header_t *ph = (stream_packet_inner_header_t *)rd;
	*frame = (stream_frame_t *)(rd + sizeof(stream_packet_inner_header_t));
	*dataLen = ph->len;
	return rd + sizeof(stream_frame_t) + sizeof(stream_packet_inner_header_t);
}

void StreamBuffer::offsetPacket() {
	uint32_t startpos = (m_readed & (m_bufSize - 1));
	stream_packet_inner_header_t *ph = (stream_packet_inner_header_t *)(m_pbuffer + startpos);
	if (ph->mark) {
		m_readed += m_bufSize - startpos;
	}
	else {
		uint32_t padSize = dataPadSize(ph->len);
		m_readed += ph->len + padSize + sizeof(stream_frame_t) + sizeof(stream_packet_inner_header_t);
	}
}

