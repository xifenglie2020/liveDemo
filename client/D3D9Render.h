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
	//YUV 数据
	void Display(struct video_frame_data_t *yuv420);

private:
	// D3D9对象
	IDirect3D9				*m_pID3D9;
	// D3D9设备
	IDirect3DDevice9		*m_pDevice;
	// 设备参数
	D3DPRESENT_PARAMETERS	m_DeviceParam;
	UINT					m_D3DAdapter;
	// 显卡支持的输入格式
	//unsigned int			m_FormatSupported;
	// 交换链创建参数，用于创建和恢复操作
	//D3DPRESENT_PARAMETERS	m_D3DSwapChainParam;
	// 交换链，相当于渲染流水线
	//IDirect3DSwapChain9		*m_pID3DSwapChain;
	// 离屏页面的宽,高,格式
	//D3DFORMAT				m_d3dFormat;
	// 离屏页面，数据中转容器
	IDirect3DSurface9		*m_pOffScreenSurface;
	HWND m_hWnd;
	//CPictureConvert			m_pic_conv;
};



