#pragma once

#include "env.h"

//显示登录窗口
//#define MAIN_PAGE_CTRL_SHOWLOGIN		0
//创建内容发布窗口
#define MAIN_PAGE_CTRL_CREATE_PUBLISHER		1
//客户端窗口
#define MAIN_PAGE_CTRL_CREATE_SUBSCRIBER	2

class CWindowMain : public WindowImplBase
{
public:
	CWindowMain(void);
	~CWindowMain(void);

	void doClose();
public:
	virtual UINT GetClassStyle() const {
		return CWindowWnd::GetClassStyle() | CS_DBLCLKS;
	}
	virtual CDuiString GetSkinFolder();
	virtual CDuiString GetSkinFile();
	virtual UILIB_RESOURCETYPE GetResourceType() const;
	virtual CDuiString GetZIPFileName() const;
	virtual void InitWindow();
	virtual void OnFinalMessage(HWND);
	virtual LPCTSTR GetWindowClassName(void) const { return _T("LianGuiMainWindow"); }
	virtual void Notify( TNotifyUI &msg );
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, bool& /*bHandled*/);
	virtual CControlUI *CreateControl(LPCTSTR pstrClass) { return NULL; }
	bool OnWindowSizeChanged(void *param);
public:
	void ShowMaximized();
	void ShowMinimized();
private:
	LRESULT OnMyPageCtrl(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMyDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMyTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNcCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNcDlbClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNetMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	void    GetWorkArea(CDuiRect &rcWork);
private:
	//int     m_titleHeight;
	CWindowWnd *m_pBusinessWnd = NULL;
	CControlUI *m_pWndBody = NULL;
};

