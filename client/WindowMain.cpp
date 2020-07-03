#include "WindowMain.h"
#include "App.h"
#include "WindowPublisher.h"
#include "WindowSubscriber.h"
//////////////////////////////////////////////////////////////////////////
///

#define TitleBtnCloseName	_T("title.btn.close")
#define TitleBtnMinName		_T("title.btn.min")
#define TitleBtnMaxName		_T("title.btn.max")
#define TitleBtnRestoreName	_T("title.btn.restore")

#define TIMERID_NETWORK		8000

CWindowMain::CWindowMain(void)
{
}

CWindowMain::~CWindowMain(void)
{
}


CDuiString CWindowMain::GetSkinFolder()
{
	return _T("..\\skins\\");
}

CDuiString CWindowMain::GetSkinFile()
{
	return _T("window.main.xml");
}

UILIB_RESOURCETYPE CWindowMain::GetResourceType() const
{
	return UILIB_FILE;
}

CDuiString CWindowMain::GetZIPFileName() const
{
	return _T("");
	//return _T("info.zip");
}

void CWindowMain::InitWindow() {
	CControlUI *pCtrl;
	//m_titleHeight = 0;
	//pCtrl = m_PaintManager.FindControl(_T("window.title"));
	//if (pCtrl != NULL) {
	//	m_titleHeight = pCtrl->GetFixedHeight();
	//}
	pCtrl = m_PaintManager.FindControl(_T("title.name"));
	if (pCtrl != NULL) {
		::SetWindowText(m_hWnd, pCtrl->GetText());
	}
	
	pCtrl = m_PaintManager.FindControl(_T("window.body")); 
	m_pWndBody = pCtrl;
	if (pCtrl != NULL) {
		pCtrl->OnSize += MakeDelegate(this, &CWindowMain::OnWindowSizeChanged);
	}
	CenterWindow();

	SetTimer(m_hWnd, TIMERID_NETWORK, 10, NULL);
}

void CWindowMain::OnFinalMessage(HWND hWnd)
{
	KillTimer(hWnd, TIMERID_NETWORK);
	__super::OnFinalMessage(hWnd);
}

LRESULT CWindowMain::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM  lParam, bool& bHandled)
{
	switch (uMsg){
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE){
			bHandled = true;
		}
		break;
	default:
		break;
	}
	return S_OK;
	//return __super::MessageHandler(uMsg, wParam, lParam, bHandled);
}
LRESULT CWindowMain::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = FALSE;
	switch (uMsg){
	case WM_TIMER:			lRes = OnMyTimer(uMsg, wParam, lParam, bHandled); break;
	case WM_NCHITTEST:      lRes = OnNcHitTest(uMsg, wParam, lParam, bHandled); break;
	case WM_NCCALCSIZE:		lRes = OnNcCalcSize(uMsg, wParam, lParam, bHandled); break;
	case WM_GETMINMAXINFO:	lRes = OnGetMinMaxInfo(uMsg, wParam, lParam, bHandled); break;
	case WM_SYSCOMMAND:		lRes = OnSysCommand(uMsg, wParam, lParam, bHandled);break;
	case WM_NCLBUTTONDBLCLK:lRes = OnNcDlbClick(uMsg, wParam, lParam, bHandled); break;
	case WMUSER_NET_MSG:	lRes = OnNetMessage(uMsg, wParam, lParam, bHandled); break;
	case WMUSER_PAGE_CTRL:	lRes = OnMyPageCtrl(uMsg, wParam, lParam, bHandled);break;
	case WM_CLOSE:	
	case WM_DESTROY:		lRes = OnMyDestroy(uMsg, wParam, lParam, bHandled);	break;
	default:	break;
	}
	if (bHandled) return lRes;
	return __super::HandleMessage(uMsg, wParam, lParam);
}

LRESULT CWindowMain::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = TRUE;

	POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
	::ScreenToClient(*this, &pt);

	RECT rcClient;
	::GetClientRect(*this, &rcClient);

	if (!::IsZoomed(*this))
	{
		RECT rcSizeBox = m_PaintManager.GetSizeBox();
		if (pt.y < rcClient.top + rcSizeBox.top)
		{
			if (pt.x < rcClient.left + rcSizeBox.left) return HTTOPLEFT;
			if (pt.x > rcClient.right - rcSizeBox.right) return HTTOPRIGHT;
			return HTTOP;
		}
		else if (pt.y > rcClient.bottom - rcSizeBox.bottom)
		{
			if (pt.x < rcClient.left + rcSizeBox.left) return HTBOTTOMLEFT;
			if (pt.x > rcClient.right - rcSizeBox.right) return HTBOTTOMRIGHT;
			return HTBOTTOM;
		}

		if (pt.x < rcClient.left + rcSizeBox.left) return HTLEFT;
		if (pt.x > rcClient.right - rcSizeBox.right) return HTRIGHT;
	}

	RECT rcCaption = m_PaintManager.GetCaptionRect();
	if (pt.x >= rcClient.left + rcCaption.left && pt.x < rcClient.right - rcCaption.right \
		&& pt.y >= rcCaption.top && pt.y < rcCaption.bottom) {
		CControlUI* pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(pt));
		if (pControl == NULL ||
			(_tcsicmp(pControl->GetClass(), DUI_CTR_BUTTON) != 0 &&
			_tcsicmp(pControl->GetClass(), DUI_CTR_OPTION) != 0)){
			//_tcsicmp(pControl->GetClass(), DUI_CTR_TEXT) != 0
			return HTCAPTION;
		}
	}
	return HTCLIENT;
}

void CWindowMain::ShowMaximized(){
	//::ShowWindow(m_hWnd, SW_SHOWMAXIMIZED);
	PostMessage(WM_SYSCOMMAND, SC_MAXIMIZE, 0);
}
void CWindowMain::ShowMinimized(){
	//::ShowWindow(m_hWnd, SW_SHOWMAXIMIZED);
	PostMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

void CWindowMain::GetWorkArea(CDuiRect &rcWork){
	MONITORINFO oMonitor = {};
	oMonitor.cbSize = sizeof(oMonitor);
	::GetMonitorInfo(::MonitorFromWindow(*this, MONITOR_DEFAULTTONEAREST), &oMonitor);
	CDuiRect rcMonitor = oMonitor.rcMonitor;
	rcWork = oMonitor.rcWork;
	rcWork.Offset(-oMonitor.rcMonitor.left, -oMonitor.rcMonitor.top);
}

LRESULT CWindowMain::OnNcCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LPRECT pRect = NULL;
	if (wParam == TRUE){
		LPNCCALCSIZE_PARAMS pParam = (LPNCCALCSIZE_PARAMS)lParam;
		pRect = &pParam->rgrc[0];
	}
	else{
		pRect = (LPRECT)lParam;
	}

	if (::IsZoomed(m_hWnd))
	{	// 最大化时，计算当前显示器最适合宽高度
		CDuiRect rcWork;
		GetWorkArea(rcWork);
		pRect->right = pRect->left + rcWork.GetWidth();
		pRect->bottom = pRect->top + rcWork.GetHeight();
		bHandled = TRUE;
		return WVR_REDRAW;
	}
	return 0;
}

LRESULT CWindowMain::OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;

	CDuiRect rcWork;
	GetWorkArea(rcWork);

	// 计算最大化时，正确的原点坐标
	lpMMI->ptMaxPosition.x = rcWork.left;
	lpMMI->ptMaxPosition.y = rcWork.top;

	lpMMI->ptMaxSize.x = rcWork.GetWidth();
	lpMMI->ptMaxSize.y = rcWork.GetHeight();

	lpMMI->ptMaxTrackSize.x = rcWork.GetWidth();
	lpMMI->ptMaxTrackSize.y = rcWork.GetHeight();

	lpMMI->ptMinTrackSize.x = m_PaintManager.GetMinInfo().cx;
	lpMMI->ptMinTrackSize.y = m_PaintManager.GetMinInfo().cy;

	bHandled = TRUE;
	return TRUE;
}
LRESULT CWindowMain::OnNcDlbClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	BOOL b;
	if (HTCAPTION == OnNcHitTest(WM_NCHITTEST, wParam, lParam, b)){
		bHandled = TRUE;
		if (::IsZoomed(m_hWnd)){
			PostMessage(WM_SYSCOMMAND, SC_RESTORE, 0);
		}
		else{
			PostMessage(WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		}
	}
	return S_OK;
}
LRESULT CWindowMain::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// 有时会在收到WM_NCDESTROY后收到wParam为SC_CLOSE的WM_SYSCOMMAND
	bHandled = TRUE;
	if (wParam == SC_CLOSE) {
		return S_OK;
	}
	BOOL bZoomed = ::IsZoomed(*this);
	LRESULT lRes = CWindowWnd::HandleMessage(uMsg, wParam, lParam);
	if (::IsZoomed(*this) != bZoomed) {
		if (!bZoomed) {
			CControlUI* pControl;
			pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(TitleBtnMaxName));
			if (pControl) pControl->SetVisible(false);
			pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(TitleBtnRestoreName));
			if (pControl) pControl->SetVisible(true);
		}
		else {
			CControlUI* pControl;
			pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(TitleBtnMaxName));
			if (pControl) pControl->SetVisible(true);
			pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(TitleBtnRestoreName));
			if (pControl) pControl->SetVisible(false);
		}
		//OnWindowMoved();
	}
	return lRes;
}

bool CWindowMain::OnWindowSizeChanged(void *param) {
	if (m_pBusinessWnd != NULL && m_pWndBody != NULL) {
		RECT rc = m_pWndBody->GetPos();
		::SetWindowPos(m_pBusinessWnd->GetHWND(), NULL,
			rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
	}
	return true;
}

void CWindowMain::doClose() {
	Close();
	PostQuitMessage(0);
}

LRESULT CWindowMain::OnMyDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	//子窗口自己会销毁
	return S_OK;
}

LRESULT CWindowMain::OnMyTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (wParam == TIMERID_NETWORK) {
		bHandled = TRUE;
		NetObject *no = theApp.netObject();
		if (no != NULL) {
			no->step();
		}
	}
	return S_OK;
}

LRESULT CWindowMain::OnMyPageCtrl(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	switch (wParam) {
	case MAIN_PAGE_CTRL_CREATE_PUBLISHER:
		do {
			CWindowPublisher *pWnd = new CWindowPublisher();
			if (pWnd != NULL) {
				pWnd->Create(m_hWnd, pWnd->GetWindowClassName(), WS_CHILD | WS_CLIPCHILDREN, 0);
				m_pBusinessWnd = pWnd;
				OnWindowSizeChanged(NULL);
				pWnd->ShowWindow(true, false);
			}
		} while (0);
		break;
	case MAIN_PAGE_CTRL_CREATE_SUBSCRIBER:
		do {
			CWindowSubscriber *pWnd = new CWindowSubscriber();
			if (pWnd != NULL) {
				pWnd->Create(m_hWnd, pWnd->GetWindowClassName(), WS_CHILD | WS_CLIPCHILDREN, 0);
				m_pBusinessWnd = pWnd;
				OnWindowSizeChanged(NULL);
				pWnd->ShowWindow(true, false);
			}
		} while (0);
	}
	return S_OK;
}

LRESULT CWindowMain::OnNetMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = TRUE;
	if (m_pBusinessWnd != NULL) {
		return ::SendMessage(m_pBusinessWnd->GetHWND(), uMsg, wParam, lParam);
	}
	return S_OK;
}

void CWindowMain::Notify(TNotifyUI& msg)
{
	 if (_tcsicmp(msg.sType, DUI_MSGTYPE_CLICK) == 0) {
		CDuiString &name = msg.pSender->GetName();
		if (name == TitleBtnCloseName) {
			doClose();
		}
		else if (name == TitleBtnMinName) {
			SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
		}
		else if (name == TitleBtnMaxName) {
			SendMessage(WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		}
		else if (name == TitleBtnRestoreName) {
			SendMessage(WM_SYSCOMMAND, SC_RESTORE, 0);
		}
		else if (name == "btn.login") {
			CControlUI *ip = m_PaintManager.FindControl(_T("login.ip"));
			CControlUI *port = m_PaintManager.FindControl(_T("login.port"));
			CControlUI *user = m_PaintManager.FindControl(_T("login.user"));
			CControlUI *pswd = m_PaintManager.FindControl(_T("login.pswd"));
			CCheckBoxUI *pub = (CCheckBoxUI *)m_PaintManager.FindControl(_T("login.publisher"));
			CCheckBoxUI *net = (CCheckBoxUI *)m_PaintManager.FindControl(_T("network.tcp"));
			CCheckBoxUI *capfile = (CCheckBoxUI *)m_PaintManager.FindControl(_T("capture.file"));
			bool isPublisher = pub->GetCheck();
			int nPort = atoi((LPCTSTR)port->GetText());
			if (ip->GetText().GetLength() <= 0 || 
				nPort <= 0 || 
				user->GetText().GetLength() <= 0) {
				//...
			}
			else {
				theApp.config().capFile = capfile && capfile->GetCheck() ? 1 : 0;
				theApp.config().netType = net && net->GetCheck() ? TRANSFER_TYPE_TCP : TRANSFER_TYPE_UDP;
				bool f = theApp.netObject()->login((LPCTSTR)ip->GetText(), nPort, 
					(LPCTSTR)user->GetText(), (LPCTSTR)pswd->GetText());
				if (f) {
					PostMessage(WMUSER_PAGE_CTRL, 
						isPublisher ? MAIN_PAGE_CTRL_CREATE_PUBLISHER : MAIN_PAGE_CTRL_CREATE_SUBSCRIBER, 
						0);
				}
			}
		}
	}
}

