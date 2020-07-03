#pragma once

#include "env.h"

class CWindowPlayer : public CWindowWnd
{
public:
	CWindowPlayer();
	~CWindowPlayer() {}
	void setLocation(LPRECT rc);
public:
	virtual LPCTSTR GetWindowClassName() const { return _T("PlayerWindow"); }
	virtual UINT GetClassStyle() const {
		return CWindowWnd::GetClassStyle() | CS_DBLCLKS;
	}
protected:
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMyPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnVideoSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	int m_width = 0;
	int m_height = 0;
};


class CWindowEmpty : public CWindowWnd
{
public:
	CWindowEmpty() {}
	~CWindowEmpty() {}
	virtual LPCTSTR GetWindowClassName() const { return _T("EmptyWindow"); }
};

