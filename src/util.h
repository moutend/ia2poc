#pragma once

#include <UIAutomationClient.h>
#include <cstdint>
#include <windows.h>

#include "types.h"

template <class T> void SafeRelease(T **ppT) {
  if (ppT == nullptr) {
    return;
  }
  if (*ppT == nullptr) {
    return;
  }

  (*ppT)->Release();
  *ppT = nullptr;
}

void SafeCloseHandle(HANDLE *pHandle);
void SafeDelete(RawEvent **pRawEvent);
void SafeDelete(RawProcessInfo **pRawProcessInfo);
bool isEmptyIUIAutomationElement(IUIAutomationElement *pElement);
HRESULT GetProcessInfo(DWORD processId, RawProcessInfo **rawProcessInfo);
HRESULT GetProcessInfo(HWND hWindow, RawProcessInfo **rawProcessInfo);
