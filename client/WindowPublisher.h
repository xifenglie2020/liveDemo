#pragma once

#include "WindowPlayer.h"
#include "ContextPublisher.h"
#include "ContextSubscriber.h"
#include "MissionObject.h"

class CWindowPublisher : public CWindowWnd
	, public IMessageFilterUI
	, public INotifyUI
	, public IDialogBuilderCallback
{
public: 
	CWindowPublisher();
	~CWindowPublisher();


public:
	virtual LPCTSTR GetWindowClassName() const { return _T("PublisherWindow"); }
protected:
	virtual UINT GetClassStyle() const {
		return CWindowWnd::GetClassStyle() | CS_DBLCLKS;
	}
	virtual LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled){ return S_OK; }
	virtual void OnFinalMessage(HWND /*hWnd*/);
	virtual void Notify(TNotifyUI& msg);
	virtual CControlUI* CreateControl(LPCTSTR pstrClass) { return NULL; }
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	LRESULT OnMyCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMyDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNetMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	bool OnWindowSizeChanged(void *param);

	void startCapture();
	void stopCapture();
private:
	CPaintManagerUI m_pm;
	//CWindowPlayer	m_wndForCapture;
	CWindowEmpty	m_wndEmpty;
	CWindowPlayer	m_wndPlayer;
	CControlUI		*m_pCtrlPlayer = NULL;

	ContextPublisher m_publisher;
	ContextSubscriber m_subscriber;
	MissionObject	m_missionObject;

};

