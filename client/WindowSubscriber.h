#pragma once

#include "WindowPlayer.h"
#include "ContextSubscriber.h"
#include "ContextPublisher.h"
#include "MissionObject.h"

class CWindowSubscriber;
class CListHolder : public IListCallbackUI {
public:
	void setOwner(CWindowSubscriber *p) { m_pOwner = p; }
	void attach(CListUI *p);
	void add(struct proto_query_publishers_rsp_t *);
	virtual LPCTSTR GetItemText(CControlUI* pList, int iItem, int iSubItem);
protected:
	bool OnListItemEvent(void *params);
	CListUI *m_pList;
	CWindowSubscriber *m_pOwner;
};

class CWindowSubscriber : public CWindowWnd
	, public IMessageFilterUI
	, public INotifyUI
	, public IDialogBuilderCallback
{
public: 
	CWindowSubscriber();
	~CWindowSubscriber();

	void startPlay(const char *url);
	void stopPlay();

public:
	virtual LPCTSTR GetWindowClassName() const { return _T("SubscriberWindow"); }
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

	void queryPlayers();
private:
	CPaintManagerUI m_pm;
	//CWindowPlayer	m_wndForCapture;
	CWindowEmpty	m_wndEmpty;
	CWindowPlayer	m_wndPlayer;
	CControlUI		*m_pCtrlPlayer = NULL;
	CListHolder		m_playerList;

	ContextPublisher m_publisher;
	ContextSubscriber m_subscriber;
	MissionObject	m_missionObject;

};

