#include <cpplogger/cpplogger.h>
#include <cstring>
#include <mutex>
#include <vector>
#include <windows.h>

#include <strsafe.h>

#include "api.h"
#include "context.h"
#include "logloop.h"
#include "msaa.h"
#include "types.h"
#include "uialoop.h"
#include "util.h"
#include "wineventloop.h"

extern Logger::Logger *Log;

static bool isActive{false};
static std::mutex apiMutex;

static LogLoopContext *logLoopCtx{};
static UIALoopContext *uiaLoopCtx{};
static WinEventLoopContext *winEventLoopCtx{};

static HANDLE logLoopThread{};
static HANDLE uiaLoopThread{};
static HANDLE winEventLoopThread{};

void __stdcall Setup(int32_t *code, int32_t logLevel,
                     EventHandler eventHandler) {
  std::lock_guard<std::mutex> lock(apiMutex);

  if (code == nullptr) {
    return;
  }
  if (isActive) {
    Log->Warn(L"Already initialized", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  Log = new Logger::Logger(L"ia2poc", L"v0.1.0-develop", 4096);

  Log->Info(L"Setup ia2poc", GetCurrentThreadId(), __LONGFILE__);

  logLoopCtx = new LogLoopContext();

  logLoopCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (logLoopCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  Log->Info(L"Create log loop thread", GetCurrentThreadId(), __LONGFILE__);

  logLoopThread = CreateThread(nullptr, 0, logLoop,
                               static_cast<void *>(logLoopCtx), 0, nullptr);

  if (logLoopThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  Sleep(10500);
  uiaLoopCtx = new UIALoopContext();

  uiaLoopCtx->HandleFunc = eventHandler;

  uiaLoopCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (uiaLoopCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  Log->Info(L"Create uia loop thread", GetCurrentThreadId(), __LONGFILE__);

  uiaLoopThread = CreateThread(nullptr, 0, uiaLoop,
                               static_cast<void *>(uiaLoopCtx), 0, nullptr);

  if (uiaLoopThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  Log->Info(L"Complete setup ia2poc", GetCurrentThreadId(), __LONGFILE__);

  winEventLoopCtx = new WinEventLoopContext();

  winEventLoopCtx->HandleFunc = eventHandler;

  winEventLoopCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (winEventLoopCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  Log->Info(L"Create windows event loop thread", GetCurrentThreadId(),
            __LONGFILE__);

  winEventLoopThread =
      CreateThread(nullptr, 0, winEventLoop,
                   static_cast<void *>(winEventLoopCtx), 0, nullptr);

  if (winEventLoopThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  Log->Info(L"Complete setup ia2poc", GetCurrentThreadId(), __LONGFILE__);

  isActive = true;
}

void __stdcall Teardown(int32_t *code) {
  std::lock_guard<std::mutex> lock(apiMutex);

  if (code == nullptr) {
    return;
  }
  if (!isActive) {
    *code = -1;
    return;
  }

  Log->Info(L"Teardown ia2poc", GetCurrentThreadId(), __LONGFILE__);

  if (uiaLoopThread == nullptr) {
    goto END_UIALOOP_CLEANUP;
  }
  if (!SetEvent(uiaLoopCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  WaitForSingleObject(uiaLoopThread, INFINITE);
  SafeCloseHandle(&uiaLoopThread);
  SafeCloseHandle(&(uiaLoopCtx->QuitEvent));

  delete uiaLoopCtx;
  uiaLoopCtx = nullptr;

  Log->Info(L"Delete uia loop thread", GetCurrentThreadId(), __LONGFILE__);

END_UIALOOP_CLEANUP:

  if (winEventLoopThread == nullptr) {
    goto END_WINEVENTLOOP_CLEANUP;
  }

  winEventLoopCtx->IsActive = false;
  /*
  if (!SetEvent(winEventLoopCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }
  */

  WaitForSingleObject(winEventLoopThread, INFINITE);
  SafeCloseHandle(&winEventLoopThread);
  SafeCloseHandle(&(winEventLoopCtx->QuitEvent));

  delete winEventLoopCtx;
  winEventLoopCtx = nullptr;

  Log->Info(L"Delete windows event loop thread", GetCurrentThreadId(),
            __LONGFILE__);

END_WINEVENTLOOP_CLEANUP:

  Log->Info(L"Complete teardown ia2poc", GetCurrentThreadId(), __LONGFILE__);

  if (logLoopThread == nullptr) {
    goto END_LOGLOOP_CLEANUP;
  }
  if (!SetEvent(logLoopCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  WaitForSingleObject(logLoopThread, INFINITE);
  SafeCloseHandle(&logLoopThread);
  SafeCloseHandle(&(logLoopCtx->QuitEvent));

  delete logLoopCtx;
  logLoopCtx = nullptr;

END_LOGLOOP_CLEANUP:

  isActive = false;
}
