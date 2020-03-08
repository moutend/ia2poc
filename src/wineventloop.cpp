#include <cpplogger/cpplogger.h>
#include <ia2/ia2.h>
#include <windows.h>

#include <Commctrl.h>
#include <oleacc.h>

#include "context.h"
#include "types.h"
#include "util.h"
#include "wineventloop.h"

extern Logger::Logger *Log;

static WinEventLoopContext *winEventLoopCtx{};

void eventCallback(HWINEVENTHOOK hHook, DWORD eventId, HWND hWindow,
                   LONG objectId, LONG childId, DWORD threadId,
                   DWORD eventTime) {
  if (objectId <= OBJID_ALERT) {
    return;
  }
  if (!IsWindow(hWindow)) {
    return;
  }
  if (objectId == 0 && childId == 0) {
    objectId = OBJID_CLIENT;
  }

  IAccessible *pAcc{nullptr};
  IAccessible2 *pAcc2{nullptr};
  IServiceProvider *pServe{nullptr};
  VARIANT vChild{};

  HRESULT hr{};

  hr = AccessibleObjectFromEvent(hWindow, objectId, childId, &pAcc, &vChild);

  if (FAILED(hr)) {
    return;
  }
  if (vChild.vt == VT_I4) {
    // todo
  }

  Log->Info(L"IAccessible event received", GetCurrentThreadId(), __LONGFILE__);
  goto CLEANUP;

  if (pAcc->QueryInterface(IID_IServiceProvider,
                           reinterpret_cast<void **>(&pServe)) != S_OK) {
    Log->Info(L"Failed to query IServiceProvider", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  if (FAILED(pServe->QueryService(IID_IAccessible, IID_IAccessible2,
                                  reinterpret_cast<void **>(&pAcc2)))) {
    Log->Info(L"Failed to query IAccessible2", GetCurrentThreadId(),
              __LONGFILE__);
  } else {
    Log->Info(L"Success to query IAccessible2", GetCurrentThreadId(),
              __LONGFILE__);
  }

  goto CLEANUP;

  RawEvent *pRawEvent{};
  RawProcessInfo *pRawProcessInfo{};

  if (FAILED(RawEventFromIAccessible(eventId, pAcc, &pRawEvent))) {
    return;
  }
  if (FAILED(GetProcessInfo(hWindow, &pRawProcessInfo))) {
    return;
  }
  if (winEventLoopCtx->HandleFunc(pRawEvent, pRawProcessInfo) != 0) {
    return;
  }

CLEANUP:

  SafeRelease(&pAcc2);
  SafeRelease(&pServe);
  SafeRelease(&pAcc);
  SafeDelete(&pRawEvent);
  SafeDelete(&pRawProcessInfo);
}

DWORD WINAPI winEventLoop(LPVOID context) {
  Log->Info(L"Start Windows event loop thread", GetCurrentThreadId(),
            __LONGFILE__);

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call CoInitializeEx", GetCurrentThreadId(),
              __LONGFILE__);
    return hr;
  }

  winEventLoopCtx = static_cast<WinEventLoopContext *>(context);

  if (winEventLoopCtx == nullptr) {
    Log->Fail(L"Failed to obtain context", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  HWINEVENTHOOK hookIds[24];
  const DWORD events[24] = {EVENT_SYSTEM_DESKTOPSWITCH,
                            EVENT_SYSTEM_FOREGROUND,
                            EVENT_SYSTEM_ALERT,
                            EVENT_SYSTEM_MENUSTART,
                            EVENT_SYSTEM_MENUEND,
                            EVENT_SYSTEM_MENUPOPUPSTART,
                            EVENT_SYSTEM_MENUPOPUPEND,
                            EVENT_SYSTEM_SCROLLINGSTART,
                            EVENT_SYSTEM_SWITCHEND,
                            EVENT_OBJECT_FOCUS,
                            EVENT_OBJECT_SHOW,
                            EVENT_OBJECT_HIDE,
                            EVENT_OBJECT_DESTROY,
                            EVENT_OBJECT_DESCRIPTIONCHANGE,
                            EVENT_OBJECT_LOCATIONCHANGE,
                            EVENT_OBJECT_NAMECHANGE,
                            EVENT_OBJECT_REORDER,
                            EVENT_OBJECT_SELECTION,
                            EVENT_OBJECT_SELECTIONADD,
                            EVENT_OBJECT_SELECTIONREMOVE,
                            EVENT_OBJECT_SELECTIONWITHIN,
                            EVENT_OBJECT_STATECHANGE,
                            EVENT_OBJECT_VALUECHANGE,
                            EVENT_OBJECT_LIVEREGIONCHANGED};

  for (int i = 0; i < 24; i++) {
    hookIds[i] = SetWinEventHook(events[i], events[i], nullptr, eventCallback,
                                 0, 0, WINEVENT_OUTOFCONTEXT);

    if (hookIds[i] == 0) {
      Log->Warn(L"Failed to call SetWinEventHook()", GetCurrentThreadId(),
                __LONGFILE__);
    }
  }

  Log->Info(L"Register callbacks", GetCurrentThreadId(), __LONGFILE__);

  // HANDLE waitArray[1] = {winEventLoopCtx->QuitEvent};
  // DWORD waitResult = WaitForMultipleObjects(1, waitArray, FALSE, INFINITE);

  UINT_PTR timerId = SetTimer(nullptr, 0, 3000, nullptr);
  MSG msg;

  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    if (!winEventLoopCtx->IsActive) {
      Log->Info(L"Win events no longer used", GetCurrentThreadId(),
                __LONGFILE__);
      break;
    }
  }

  KillTimer(nullptr, timerId);

  /*
    switch (waitResult) {
    case WAIT_OBJECT_0 + 0: // ctx->QuitEvent
      break;
    }
    */
  for (int i = 0; i < 24; i++) {
    if (hookIds[i] == 0) {
      continue;
    }

    UnhookWinEvent(hookIds[i]);
  }

  CoUninitialize();

  Log->Info(L"Finish Windows event loop thread", GetCurrentThreadId(),
            __LONGFILE__);

  return S_OK;
}
