#pragma once

#include "d3d9.h"
#include "d3d9caps.h"
#include "d3d9types.h"
//#include "D3dx9core.h"
#include "MediaFormat.h"

#define Use_Separate_D3D9_Device

#define D3D_Safe_Release(pObj) try { if (pObj) { pObj->Release(); pObj = NULL; } } catch (...) { pObj = NULL; } 

class CD3D9Render : public MediaSourceSink
{
public:
	CD3D9Render(void);
	~CD3D9Render(void);
public:
	bool Create(HWND hWnd, int width, int height);
	void Destroy();
	
	virtual bool onFrame(double dblSampleTime, media_frame_header_t *frame);
	//YUV ����
	void Display(struct video_frame_data_t *yuv420);

private:
	// D3D9����
	IDirect3D9				*m_pID3D9;
	// D3D9�豸
	IDirect3DDevice9		*m_pDevice;
	// �豸����
	D3DPRESENT_PARAMETERS	m_DeviceParam;
	UINT					m_D3DAdapter;
	// �Կ�֧�ֵ������ʽ
	//unsigned int			m_FormatSupported;
	// �������������������ڴ����ͻָ�����
	//D3DPRESENT_PARAMETERS	m_D3DSwapChainParam;
	// ���������൱����Ⱦ��ˮ��
	//IDirect3DSwapChain9		*m_pID3DSwapChain;
	// ����ҳ��Ŀ�,��,��ʽ
	//D3DFORMAT				m_d3dFormat;
	// ����ҳ�棬������ת����
	IDirect3DSurface9		*m_pOffScreenSurface;
	HWND m_hWnd;
	//CPictureConvert			m_pic_conv;
};



