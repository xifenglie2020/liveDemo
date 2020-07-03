#include "WindowPlayer.h"

CWindowPlayer::CWindowPlayer()
{
}

LRESULT CWindowPlayer::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = FALSE;
	switch (uMsg)
	{
	case WM_PAINT:		lRes = OnMyPaint(uMsg, wParam, lParam, bHandled); break;
	case WMUSER_VIDEO_SIZE: lRes = OnVideoSize(uMsg, wParam, lParam, bHandled); break;
	default:			break;
	}
	if (bHandled) return lRes;
	return __super::HandleMessage(uMsg, wParam, lParam);
}

LRESULT CWindowPlayer::OnMyPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	//bHandled = TRUE;
	return S_OK;
}

LRESULT CWindowPlayer::OnVideoSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = TRUE;
	m_width = wParam;
	m_height = lParam;
	::SendMessage(GetParent(m_hWnd), uMsg, 0, (LPARAM)this);
	return S_OK;
}

void CWindowPlayer::setLocation(LPRECT rc) {
	int w = (rc->right - rc->left);
	int h = (rc->bottom - rc->top);
	if (m_width == 0 || m_height == 0 || w <= 0 || h <= 0) {
		SetWindowPos(m_hWnd, NULL,
			rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, SWP_NOZORDER);
	}
	else {
		//w / h ? m_width / m_height  
		if (w * m_height < h * m_width) {
			//高度调整
			int r = w * m_height / m_width;
			SetWindowPos(m_hWnd, NULL,
				rc->left, rc->top + (h-r)/2, w, r, SWP_NOZORDER);
		}
		else {
			//宽度调整
			int r = h * m_width / m_height;
			SetWindowPos(m_hWnd, NULL,
				rc->left + (w-r)/2, rc->top, r, h, SWP_NOZORDER);
		}
	}
}

















