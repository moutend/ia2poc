#include <cpplogger/cpplogger.h>

#include "msaa.h"
#include "types.h"
#include "util.h"

extern Logger::Logger *Log;

HRESULT fetchAllElements(std::vector<RawElement *> &v,
                         RawProcessInfo **pRawProcessInfo) {
  HWND hWindow = GetForegroundWindow();

  if (hWindow == nullptr) {
    return E_FAIL;
  }
  if (FAILED(GetProcessInfo(hWindow, pRawProcessInfo))) {
    return E_FAIL;
  }

  HRESULT hr{};
  IAccessible *pAcc{};

  hr = AccessibleObjectFromWindow(hWindow, OBJID_CLIENT, IID_IAccessible,
                                  reinterpret_cast<void **>(&pAcc));

  if (FAILED(hr)) {
    Log->Warn(L"Failed to get accessible from window", GetCurrentThreadId(),
              __LONGFILE__);
    return hr;
  }

  hr = walkIAccessible(pAcc, 0, 0, v);

  return hr;
}

HRESULT walkIAccessible(IAccessible *pAcc, int depth, int index,
                        std::vector<RawElement *> &v) {
  if (pAcc == nullptr) {
    return S_OK;
  }

  HRESULT hr{};
  RawElement *rawElement{};

  // hr = RawElementFromIAccessible(pAcc, &rawElement);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to convert element", GetCurrentThreadId(), __LONGFILE__);
    return hr;
  }

  // rawElement->Index = index;
  // rawElement->Depth = depth;
  // v.push_back(rawElement);

  long childCount{};

  hr = pAcc->get_accChildCount(&childCount);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to get accessible children count", GetCurrentThreadId(),
              __LONGFILE__);
    return hr;
  }
  if (childCount == 0) {
    return S_OK;
  }

  long returnCount{};
  VARIANT *pArray = new VARIANT[childCount];

  hr = AccessibleChildren(pAcc, 0, childCount, pArray, &returnCount);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to get accessible children", GetCurrentThreadId(),
              __LONGFILE__);
    return hr;
  }
  for (int x = 0; x < returnCount; x++) {
    VARIANT vtChild = pArray[x];

    if (vtChild.vt != VT_DISPATCH) {
      continue;
    }

    IDispatch *pDisp = vtChild.pdispVal;
    IAccessible *pChild{};

    hr = pDisp->QueryInterface(IID_IAccessible,
                               reinterpret_cast<void **>(&pChild));

    if (SUCCEEDED(hr)) {
      walkIAccessible(pChild, depth + 1, x, v);
    }

    SafeRelease(&pDisp);
    SafeRelease(&pChild);
  }

  delete[] pArray;
  pArray = nullptr;

  return S_OK;
}
