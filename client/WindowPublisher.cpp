#include "WindowPublisher.h"
#include "App.h"

CWindowPublisher::CWindowPublisher()
{
}

CWindowPublisher::~CWindowPublisher()
{
}


LRESULT CWindowPublisher::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = FALSE;
	switch (uMsg)
	{
	case WMUSER_NET_MSG:	lRes = OnNetMessage(uMsg, wParam, lParam, bHandled); break;
	case WMUSER_VIDEO_SIZE: OnWindowSizeChanged(NULL); bHandled = TRUE;	break;
	case WM_CREATE:			lRes = OnMyCreate(uMsg, wParam, lParam, bHandled); break;
	case WM_CLOSE:			
	case WM_DESTROY:		lRes = OnMyDestroy(uMsg, wParam, lParam, bHandled); break;
	default:				bHandled = FALSE; break;
	}
	if (bHandled) return lRes;
	if (m_pm.MessageHandler(uMsg, wParam, lParam, lRes)) return lRes;
	return __super::HandleMessage(uMsg, wParam, lParam);
}

void CWindowPublisher::OnFinalMessage(HWND /*hWnd*/) {
	stopCapture();
	m_pm.RemovePreMessageFilter(this);
	delete this;
}

LRESULT CWindowPublisher::OnMyCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_pm.Init(m_hWnd);
	m_pm.AddPreMessageFilter(this);
	CDialogBuilder builder;
	CControlUI* pRoot = builder.Create(_T("window.publisher.xml"), (UINT)0, this, &m_pm);
	ASSERT(pRoot && "Failed to parse XML");
	m_pm.AttachDialog(pRoot);
	m_pm.AddNotifier(this);

	m_wndEmpty.Create(m_hWnd, m_wndEmpty.GetWindowClassName(), WS_CHILD, 0);
	m_wndEmpty.ShowWindow(false, false);

	m_wndPlayer.Create(m_hWnd, m_wndPlayer.GetWindowClassName(), WS_CHILD, 0);
	m_wndPlayer.ShowWindow(false, false);

	CControlUI *pCtrl = m_pm.FindControl(_T("window.player"));
	m_pCtrlPlayer = pCtrl;
	if (pCtrl != NULL) {
		pCtrl->OnSize += MakeDelegate(this, &CWindowPublisher::OnWindowSizeChanged);
	}

	startCapture();

	return S_OK;
}


LRESULT CWindowPublisher::OnMyDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	stopCapture();
	return S_OK;
}

bool CWindowPublisher::OnWindowSizeChanged(void *param) {
	if (m_pCtrlPlayer != NULL && ::IsWindow(m_wndPlayer.GetHWND())) {
		RECT rc = m_pCtrlPlayer->GetPos();
		m_wndPlayer.setLocation(&rc);
		//if (m_videoCapture != NULL) {
		//	m_videoCapture->onWndSizeChanged();
		//}
	}
	return true;
}


void CWindowPublisher::Notify(TNotifyUI& msg) {

}

void CWindowPublisher::startCapture() {
	if (theApp.config().capFile) {
		if (m_subscriber.startShow(m_wndPlayer.GetHWND())) {
			m_publisher.setRollback(&m_subscriber);
			m_missionObject.start(NULL);
			m_publisher.setMissionObject(&m_missionObject);
			if (m_publisher.startFile("./xtfile.conf")) {
				m_wndPlayer.ShowWindow(true, false);
				return;
			}
		}
		stopCapture();
	}
	else {
		struct captureInfo_t {
			int no;
			int fps;
		}ci = { 0, 25 };	//默认25帧
		wchar_t devname[1024] = { 0 };
		theApp.devMgr().enumVideoDevice([](void *userdata, int32_t no, wchar_t *name, IMoniker *moniker, DeviceProps *prop)->int {
			wchar_t *devname = (wchar_t *)userdata;
			if (devname[0] == '\0') {
				wcscpy(devname, name);
			}
			//wprintf(L"device pos:%d name:%s\r\n", no, name);
			return 0;
		}, devname);
		if (devname[0] != 0) {
			theApp.devMgr().enumVideoCaptureInfo(devname, [](void *userdata, int32_t no,
				const char *format, int minFps, int maxFps, int width, int height) {
				captureInfo_t *ci = (captureInfo_t *)userdata;
				printf("no %2d: %s fps(%d-%d) %d*%d\r\n", no, format, minFps, maxFps, width, height);
				if (no == ci->no) {
					if (ci->fps < minFps)		ci->fps = minFps;
					else if (ci->fps > maxFps)	ci->fps = maxFps;
				}
			}, &ci);

#if 1	//不设置rollback了
			if (m_subscriber.startShow(m_wndPlayer.GetHWND())) {
				m_publisher.setRollback(&m_subscriber);
			}
			else
#endif
			{
				m_publisher.setWindowShow(m_wndPlayer.GetHWND());
			}
			m_missionObject.start(NULL);
			m_publisher.setMissionObject(&m_missionObject);
			if (m_publisher.startCapture(devname, m_wndEmpty.GetHWND(), ci.no, ci.fps)) {
				m_wndPlayer.ShowWindow(true, false);
			}
			else {
				stopCapture();
			}
		}
	}
}

void CWindowPublisher::stopCapture() {
	m_missionObject.stop();
	m_publisher.stopCapture();
	if (::IsWindow(m_wndPlayer.GetHWND())) {
		m_wndPlayer.ShowWindow(false, false);
	}
	m_subscriber.stopShow();
}

LRESULT CWindowPublisher::OnNetMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	bHandled = TRUE;

	proto_base_t *msg = (proto_base_t *)lParam;
	uint32_t command;
	n2h_u16(msg->cmd, command);
	if (command > PRO_CMD_LOCAL_END) {
		m_missionObject.onResponse(command, msg, (uint32_t)wParam);
	}
	else {
		//nothing
	}
	return S_OK;
}
