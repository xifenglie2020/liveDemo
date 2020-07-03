#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <wchar.h>
#include <strmif.h>
#include <uuids.h>
#include <wtypes.h>
#include <combaseapi.h>
#include <wmsdkidl.h>
#include <dvdmedia.h>
//#include <control.h>
#include <dshow.h>
#include "qedit.h"

//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>

#define SAFE_RELEASE(x)	do{ if(x != NULL){x->Release();x=NULL;} }while(0)

struct direct_show_base_graph_t {
	IBaseFilter *baseFilter;
	IGraphBuilder *graphBuilder;
	ICaptureGraphBuilder2 *captureBuilder;
};

class DeviceManager;
extern int createDirectShowGraph(DeviceManager *mgr, const wchar_t *deviceName, direct_show_base_graph_t *graph);
extern void destroyDirectShowGraph(direct_show_base_graph_t *graph);


