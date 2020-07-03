#include "WindowSubscriber.h"
#include "App.h"

class CListPlayerListItem : public CListTextElementUI {
public:
	proto_query_publishers_rsp_t::item_t info;
};

void CListHolder::attach(CListUI *p) {
	m_pList = p;
	if (p) {
		p->SetTextCallback(this);
	}
}

void CListHolder::add(proto_query_publishers_rsp_t *rsp) {
	if (m_pList != NULL) {
		uint32_t start, count;
		n2h_u32(rsp->start, start);
		n2h_u32(rsp->count, count);
		if (start == 0) {
			m_pList->RemoveAll();
		}
		for (uint32_t i = 0; i < count; i++) {
			CListPlayerListItem *it = new CListPlayerListItem();
			if (it != NULL) {
				memcpy(&it->info, &rsp->items[i], sizeof(proto_query_publishers_rsp_t::item_t));
				m_pList->Add(it);
				it->SetAttribute(_T("align"), _T("center"));
				it->OnEvent += MakeDelegate(this, &CListHolder::OnListItemEvent);
			}
		}
	}
}

bool CListHolder::OnListItemEvent(void *params) {
	TEventUI *pevent = (TEventUI *)params;
	if (pevent->Type == UIEVENT_DBLCLICK) {
		if (_tcscmp(pevent->pSender->GetClass(), DUI_CTR_LISTTEXTELEMENT) == 0) {
			CListPlayerListItem *it = (CListPlayerListItem *)pevent->pSender;
			if (m_pOwner) m_pOwner->startPlay(it->info.url);
		}
	}
	return true;
}


LPCTSTR CListHolder::GetItemText(CControlUI* pItem, int iItem, int iSubItem) {
	if (pItem != NULL) {
		if (_tcscmp(pItem->GetClass(), DUI_CTR_LISTTEXTELEMENT) == 0) {
			CListPlayerListItem *it = (CListPlayerListItem *)pItem;
			switch (iSubItem){
			case 0:	return it->info.name;
			default:
				break;
			}
		}
	}
	return _T("");
}


CWindowSubscriber::CWindowSubscriber()
{
}

CWindowSubscriber::~CWindowSubscriber()
{
}


LRESULT CWindowSubscriber::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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

void CWindowSubscriber::OnFinalMessage(HWND /*hWnd*/) {
	stopPlay();
	m_pm.RemovePreMessageFilter(this);
	delete this;
}

LRESULT CWindowSubscriber::OnMyCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_pm.Init(m_hWnd);
	m_pm.AddPreMessageFilter(this);
	CDialogBuilder builder;
	CControlUI* pRoot = builder.Create(_T("window.subscriber.xml"), (UINT)0, this, &m_pm);
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
		pCtrl->OnSize += MakeDelegate(this, &CWindowSubscriber::OnWindowSizeChanged);
	}

	m_playerList.setOwner(this);
	pCtrl = m_pm.FindControl(_T("list.player"));
	if (pCtrl != NULL) {
		m_playerList.attach((CListUI *)pCtrl);
	}

	queryPlayers();

	return S_OK;
}


LRESULT CWindowSubscriber::OnMyDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	stopPlay();
	return S_OK;
}

bool CWindowSubscriber::OnWindowSizeChanged(void *param) {
	if (m_pCtrlPlayer != NULL && ::IsWindow(m_wndPlayer.GetHWND())) {
		RECT rc = m_pCtrlPlayer->GetPos();
		m_wndPlayer.setLocation(&rc);
	}
	return true;
}


void CWindowSubscriber::Notify(TNotifyUI& msg) {
	if (_tcsicmp(msg.sType, DUI_MSGTYPE_CLICK) == 0){
		CDuiString str = msg.pSender->GetName();
		if (str.Compare(_T("btn.player.query")) == 0) {
			queryPlayers();
		}
	}
}

void CWindowSubscriber::startPlay(const char *url) {
	stopPlay();

	m_missionObject.setCallback([](void *userData, 
		uint8_t streamid, uint8_t mediaType, uint8_t frameType, const uint8_t *data, int32_t size)->int32_t {
		ContextSubscriber *sub = (ContextSubscriber *)userData;
		if (mediaType == XT_MEDIA_TYPE_HEADER) {
			xp_media_header_t mediaHeader;
			memcpy(&mediaHeader, data, sizeof(xp_media_header_t));
			//里面的参数在onHeader函数内部转序
			//n2h_u16(mediaHeader.video.type, mediaHeader.video.type);
			//n2h_u16(mediaHeader.video.width, mediaHeader.video.width);
			//n2h_u16(mediaHeader.video.height, mediaHeader.video.height);
			sub->onHeader(&mediaHeader);
		}
		else {
			stream_frame_t frame;
			frame.stream = streamid;
			frame.media = mediaType;
			frame.type = frameType;
			if (frameType == XT_FRAME_TYPE_KEY && mediaType == 0) {
				const xp_video_keyframe_t *kf = (const xp_video_keyframe_t *)data;
				n2h_u32(kf->seqNo, frame.seqNo);
				sub->onData(&frame, data + sizeof(xp_video_keyframe_t), size - sizeof(xp_video_keyframe_t));
			}
			else {
				frame.seqNo = 0;
				sub->onData(&frame, data, size);
			}
		}
		return 0;
	}, &m_subscriber);
	m_missionObject.start(url);
	if (m_subscriber.startShow(m_wndPlayer.GetHWND())) {
		m_wndPlayer.ShowWindow(true, false);
	}
}

void CWindowSubscriber::stopPlay() {
	m_missionObject.stop();
	m_publisher.stopCapture();
	if (::IsWindow(m_wndPlayer.GetHWND())) {
		m_wndPlayer.ShowWindow(false, false);
	}
	m_subscriber.stopShow();
}

LRESULT CWindowSubscriber::OnNetMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	bHandled = TRUE;

	proto_base_t *msg = (proto_base_t *)lParam;
	uint32_t command;
	n2h_u16(msg->cmd, command);
	if (command > PRO_CMD_LOCAL_END) {
		m_missionObject.onResponse(command, msg, (uint32_t)wParam);
	}
	else {
		switch (command) {
		case PRO_CMD_STOPED:
			stopPlay();
			break;
		case PRO_CMD_QUERY_PUB_RSP:
			m_playerList.add((proto_query_publishers_rsp_t *)msg);
			break;
		}
	}
	return S_OK;
}

void CWindowSubscriber::queryPlayers() {
	proto_base_t req = { 0 };
	int len = (int)sizeof(req);
	h2n_u16(len, req.len);
	h2n_u16(PRO_CMD_QUERY_PUB_REQ, req.cmd);
	strcpy(req.from, theApp.netObject()->selfUrl());
	strcpy(req.to, theApp.netObject()->centUrl());
	theApp.netObject()->sendData(&req, len);
}

