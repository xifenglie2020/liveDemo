#include "D3D9Render.h"
#include "logger.h"
#include "MediaFormat.h"
#include "libyuv.h"
#include "env.h"

LOGGER_Declare("d3d9");

CD3D9Render::CD3D9Render(void)
{
	// D3D9对象
	m_pID3D9 = NULL;
	m_pDevice = NULL;
	// 离屏页面的宽,高,格式
	// 离屏页面，数据中转容器
	m_pOffScreenSurface = NULL;
}

CD3D9Render::~CD3D9Render(void)
{
}

/*
pDecoder->m_YUV420PFrame.width  = VCodecCtx->width;
pDecoder->m_YUV420PFrame.height = VCodecCtx->height;
// 尝试从解码出来的帧直接转换到编码需要的帧
pDecoder->m_decode2yuv420p = sws_getContext(VCodecCtx->width,VCodecCtx->height,VCodecCtx->pix_fmt,
VCodecCtx->width,VCodecCtx->height,PIX_FMT_YUV420P,
SWS_BICUBIC,NULL,NULL,NULL);
*/

UINT GetD3DAdapter(IDirect3D9 *pID3D9, HWND hDisWnd)
{
	// 默认为主显示设备
	UINT D3DAdapterIndex = D3DADAPTER_DEFAULT;
	do {
		// 参数校验
		if (pID3D9==NULL||!IsWindow(hDisWnd)){
			break;
		}

		// 获取目标窗口对应的显示设备句柄
		HMONITOR hWndHM = NULL;
		hWndHM = MonitorFromWindow(hDisWnd,MONITOR_DEFAULTTONEAREST);
		if (hWndHM==NULL){
			break;
		}

		// 获取指定的显示设备信息
		MONITORINFOEX mi;
		memset(&mi,0,sizeof(mi));
		mi.cbSize = sizeof(MONITORINFOEX);
		if (GetMonitorInfo(hWndHM,&mi)!=TRUE){
			break;
		}
		//XLOG_INFO("[D3D9] Device: %s \r\n",mi.szDevice);

		// 获取显示适配器数量
		UINT AdapterCount = 0;
		AdapterCount = pID3D9->GetAdapterCount();
		if (AdapterCount<=0){
			break;
		}

		// 枚举和匹配显示适配器
		D3DADAPTER_IDENTIFIER9 IDentifier;
		for (UINT index=0;index<AdapterCount;index++){
			memset(&IDentifier,0,sizeof(IDentifier));
			if (pID3D9->GetAdapterIdentifier(index,0,&IDentifier)==D3D_OK){
				// 匹配成功
				if (strncmp(IDentifier.DeviceName,mi.szDevice,CCHDEVICENAME)==0){
					D3DAdapterIndex = index;
					break;
				}
			}
		}

	} while(0);

	return D3DAdapterIndex;
}


bool CD3D9Render::Create(HWND hWnd, int width, int height)
{
	HRESULT hRet = D3D_OK;
	m_hWnd = (HWND)hWnd;
	m_pID3D9	= NULL;
	m_pDevice	= NULL;

	// Create D3D9
	m_pID3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (m_pID3D9==NULL){
		LOGGER_Error("[D3D9] Direct3DCreate9() Error! \r\n");
		return false;
	}

	UINT D3DAdapter = D3DADAPTER_DEFAULT;
#ifdef Use_Separate_D3D9_Device
	D3DAdapter = GetD3DAdapter(m_pID3D9,m_hWnd);
#endif // Use_Separate_D3D9_Device

	D3DDISPLAYMODE displaymode;
	hRet = m_pID3D9->GetAdapterDisplayMode(D3DAdapter,&displaymode);
	if (hRet!=D3D_OK){
		LOGGER_Error("[D3D9] GetAdapterDisplayMode() Error! hRet = 0x%08X \r\n",hRet);
		Destroy();
		return false;
	}

	m_D3DAdapter = D3DAdapter;
	memset(&m_DeviceParam,0,sizeof(D3DPRESENT_PARAMETERS));
	// Create DisplayCard_Depending Device
	m_DeviceParam.BackBufferWidth	= displaymode.Width;
	m_DeviceParam.BackBufferHeight	= displaymode.Height;
	m_DeviceParam.BackBufferFormat	= D3DFMT_X8R8G8B8; // displaymode.Format;
	m_DeviceParam.BackBufferCount	= 1;
	m_DeviceParam.MultiSampleType	= D3DMULTISAMPLE_NONE;
	m_DeviceParam.MultiSampleQuality = 0;
	m_DeviceParam.SwapEffect		= D3DSWAPEFFECT_DISCARD;
	m_DeviceParam.hDeviceWindow		= m_hWnd; // hWndTemp
	m_DeviceParam.Windowed			= TRUE;
	m_DeviceParam.EnableAutoDepthStencil = FALSE;
	m_DeviceParam.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
	m_DeviceParam.Flags				= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	m_DeviceParam.FullScreen_RefreshRateInHz = 0;
	m_DeviceParam.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	hRet = m_pID3D9->CreateDevice(m_D3DAdapter, D3DDEVTYPE_HAL, m_hWnd, // hWndTemp
		D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&m_DeviceParam,&m_pDevice);
	if (hRet!=D3D_OK){
		Destroy();
		LOGGER_Error("[D3D9] CreateDevice() Error! hRet = 0x%08X \r\n",hRet);
		return false;
	}

	hRet = m_pDevice->CreateOffscreenPlainSurface(width, height, D3DFMT_X8R8G8B8,
		D3DPOOL_DEFAULT, &m_pOffScreenSurface, NULL);
	if (hRet != D3D_OK){
		Destroy();
		LOGGER_Error("[D3D9] CreateOffscreenPlainSurface() Error! hRet = 0x%08X \r\n", hRet);
		return false;
	}
	PostMessage(hWnd, WMUSER_VIDEO_SIZE, width, height);

	return true;
}

void CD3D9Render::Destroy()
{
	D3D_Safe_Release(m_pOffScreenSurface);
	//D3D_Safe_Release(m_pID3DSwapChain);
	D3D_Safe_Release(m_pDevice);
	D3D_Safe_Release(m_pID3D9);
}

bool CD3D9Render::onFrame(double dblSampleTime, media_frame_header_t *frame) {
	Display(&frame->u.video);
	return false;
}

void CD3D9Render::Display(struct video_frame_data_t *yuv420) {
	HRESULT hRet = D3D_OK;
	IDirect3DSurface9 *pBackBufSurface = NULL;
	do {
		D3DSURFACE_DESC	surface_desc;
		D3DLOCKED_RECT	lock_rect;
		hRet = m_pOffScreenSurface->GetDesc(&surface_desc);
		if (hRet != D3D_OK){
			break;
		}

		// Fill the picture data
		hRet = m_pOffScreenSurface->LockRect(&lock_rect, NULL, D3DLOCK_DONOTWAIT | D3DLOCK_NOSYSLOCK);
		if (hRet != D3D_OK){
			break;
		}
		libyuv::I420ToARGB(yuv420->data[0], yuv420->line[0],
			yuv420->data[1], yuv420->line[1], yuv420->data[2], yuv420->line[2],
			(uint8_t *)lock_rect.pBits, lock_rect.Pitch, surface_desc.Width, surface_desc.Height);

		// 这里默认不会失败，要不如何处理？
		hRet = m_pOffScreenSurface->UnlockRect();
		hRet = m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBufSurface);
		if (hRet != D3D_OK){
			break;
		}

		// Use D3DDevice to Stretch the picture to BackBuffer
		hRet = m_pDevice->StretchRect(m_pOffScreenSurface, NULL, pBackBufSurface, NULL, D3DTEXF_LINEAR);
		if (hRet != D3D_OK){
			break;
		}

		hRet = m_pDevice->Present(NULL, NULL, (HWND)m_hWnd, NULL);
		if (hRet == D3DERR_DEVICELOST){
			break;
		}
	} while (0);

	// 每次获取使用后需要释放
	D3D_Safe_Release(pBackBufSurface);
}



