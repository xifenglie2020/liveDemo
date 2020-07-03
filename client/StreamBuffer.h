#pragma once

#include <stdlib.h>
#include <stdint.h>

struct stream_frame_t{
	uint8_t  stream;//streamid
	uint8_t  media;	//1 audio or 0 video
	uint8_t  type;	//XT_FRAME_TYPE_CORE ...
	uint8_t  pad;
	uint32_t seqNo;
};

//ѭ������ Ŀ���Ǽ����ڴ濽����ȥ����
//������̷߳������� ����Ҫ����
class StreamBuffer
{
public:
	StreamBuffer();
	~StreamBuffer();

	//bufSize ������2��n�η�
	//maxPacketSize ������С
	bool create(uint32_t bufSize = 2 * 1024 * 1024, uint32_t maxPacketSize = 1024*1024);
	void destroy();
	//����
	void reset() {
		m_writed = 0;
		m_readed = 0;
	}

	//bool putPacket(const uint8_t *pdata, uint32_t dataLen);
	bool putPacket(const stream_frame_t *frame, const uint8_t *pdata, uint32_t dataLen);
	bool hasData() { return m_writed > m_readed; }
	//��Ҫ���ж� hasData
	uint8_t *getPacket(stream_frame_t **frame, uint32_t *dataLen);
	uint8_t *getPacketChecked(stream_frame_t **frame, uint32_t *dataLen) {
		return hasData() ? getPacket(frame, dataLen) : NULL;
	}
	//getPacket/getPacketChecked����ã���ָ��������
	void offsetPacket();

private:
	uint8_t *m_pbuffer = NULL;
	uint32_t m_bufSize = 0;
	uint32_t m_maxPacketSize = 0;
	uint64_t m_writed = 0;
	uint64_t m_readed = 0;
};

