#include <UIAutomationClient.h>
#include <cpplogger/cpplogger.h>
#include <iostream>
#include <oaidl.h>

#include "context.h"
#include "uiahandler.h"
#include "uialoop.h"
#include "util.h"

extern Logger::Logger *Log;

static IUIAutomation *pUIAutomation{};
static IUIAutomationTreeWalker *pWindowTreeWalker{};
static IUIAutomationTreeWalker *pBaseTreeWalker{};
static IUIAutomationCacheRequest *pWindowCacheRequest{};
static IUIAutomationCacheRequest *pBaseCacheRequest{};
static IUIAutomationElement *pRootElement{};

static PROPERTYID itemIndexPropertyId{};
static PROPERTYID itemCountPropertyId{};

HRESULT registerPropertyId(const std::wstring &propertyGUID,
                           const std::wstring &propertyName,
                           UIAutomationType propertyType,
                           PROPERTYID *propertyId) {
  HRESULT hr{};
  GUID guid{};

  hr = CLSIDFromString(propertyGUID.c_str(), &guid);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call CLSIDFromString", GetCurrentThreadId(),
              __LONGFILE__);
    return hr;
  }

  IUIAutomationRegistrar *pRegistrar{nullptr};

  hr = CoCreateInstance(CLSID_CUIAutomationRegistrar, nullptr,
                        CLSCTX_INPROC_SERVER, IID_IUIAutomationRegistrar,
                        reinterpret_cast<void **>(&pRegistrar));

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call CoCreateInstance", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  UIAutomationPropertyInfo info = {guid, propertyName.c_str(), propertyType};

  hr = pRegistrar->RegisterProperty(&info, propertyId);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call RegisterProperty", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

CLEANUP:
  SafeRelease(&pRegistrar);

  return S_OK;
}

DWORD WINAPI uiaLoop(LPVOID context) {
  Log->Info(L"Start UI Automation loop", GetCurrentThreadId(), __LONGFILE__);

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call CoInitializeEx", GetCurrentThreadId(),
              __LONGFILE__);
    return hr;
  }

  UIALoopContext *ctx = static_cast<UIALoopContext *>(context);

  if (ctx == nullptr) {
    Log->Fail(L"Failed to obtain context", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  hr = CoCreateInstance(__uuidof(CUIAutomation), nullptr, CLSCTX_INPROC_SERVER,
                        __uuidof(IUIAutomation),
                        reinterpret_cast<void **>(&pUIAutomation));

  if (FAILED(hr)) {
    Log->Fail(L"Failed to create an instance of IUIAutomation",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  VARIANT vEmpty{};
  vEmpty.vt = VT_I4;
  vEmpty.lVal = 0;

  IUIAutomationCondition *pCondition{};
  IUIAutomationCondition *pWindowCondition{};

  hr = pUIAutomation->CreatePropertyCondition(UIA_NativeWindowHandlePropertyId,
                                              vEmpty, &pCondition);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to create an instance of IUIAutomationCondition",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  hr = pUIAutomation->CreateNotCondition(pCondition, &pWindowCondition);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to create an instance of IUIAutomationCondition",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  hr = pUIAutomation->CreateTreeWalker(pWindowCondition, &pWindowTreeWalker);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to create an instance of IUIAutomationTreeWalker",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  hr = pUIAutomation->CreateCacheRequest(&pWindowCacheRequest);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to create an instance of IUIAutomationCacheRequest",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  hr = pWindowCacheRequest->AddProperty(UIA_NativeWindowHandlePropertyId);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IUIAutomationCacheRequest::AddProperty",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  hr = pUIAutomation->get_RawViewWalker(&pBaseTreeWalker);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IUIAutomation::get_RawViewWalker",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  hr = pWindowCacheRequest->Clone(&pBaseCacheRequest);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IUIAutomationCacheRequest::Clone",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  const PROPERTYID automationElementProperties[10] = {
      UIA_NativeWindowHandlePropertyId, UIA_FrameworkIdPropertyId,
      UIA_AutomationIdPropertyId,       UIA_ClassNamePropertyId,
      UIA_ControlTypePropertyId,        UIA_ProviderDescriptionPropertyId,
      UIA_ProcessIdPropertyId,          UIA_IsTextPatternAvailablePropertyId,
      UIA_IsContentElementPropertyId,   UIA_IsControlElementPropertyId};

  for (int i = 0; i < 10; i++) {
    hr = pBaseCacheRequest->AddProperty(automationElementProperties[i]);

    if (FAILED(hr)) {
      Log->Fail(L"Failed to call IUIAutomationCacheRequest::AddProperty",
                GetCurrentThreadId(), __LONGFILE__);
      goto CLEANUP;
    }
  }

  hr = pBaseCacheRequest->AddPattern(UIA_TextPatternId);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IUIAutomationCacheRequest::AddPattern",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  hr =
      pUIAutomation->GetRootElementBuildCache(pBaseCacheRequest, &pRootElement);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IUIAutomation::GetRootElementBuildCache",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  FocusChangeEventHandler *pFocusChangeEventHandler =
      new FocusChangeEventHandler(ctx->HandleFunc);

  hr = pUIAutomation->AddFocusChangedEventHandler(pBaseCacheRequest,
                                                  pFocusChangeEventHandler);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IUIAutomation::AddFocusChangedEventHandler",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  PropertyChangeEventHandler *pPropertyChangeEventHandler =
      new PropertyChangeEventHandler(ctx->HandleFunc);

  SAFEARRAYBOUND saBound;
  saBound.lLbound = 0;
  saBound.cElements = 9;
  SAFEARRAY *pProperties = SafeArrayCreate(VT_I4, 1, &saBound);

  if (pProperties == nullptr) {
    Log->Fail(L"Failed to create SAFEARRAY", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  hr = SafeArrayLock(pProperties);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call SafeArrayLock()", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  PROPERTYID *pData = static_cast<PROPERTYID *>(pProperties->pvData);

  const PROPERTYID changeProperties[9] = {
      UIA_NamePropertyId,
      UIA_HelpTextPropertyId,
      UIA_ExpandCollapseExpandCollapseStatePropertyId,
      UIA_ToggleToggleStatePropertyId,
      UIA_IsEnabledPropertyId,
      UIA_ValueValuePropertyId,
      UIA_RangeValueValuePropertyId,
      UIA_ControllerForPropertyId,
      UIA_ItemStatusPropertyId};

  for (int i = 0; i < 9; i++) {
    pData[i] = changeProperties[i];
  }

  hr = SafeArrayUnlock(pProperties);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call SafeArrayUnlock()", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  hr = pUIAutomation->AddPropertyChangedEventHandler(
      pRootElement, TreeScope_Subtree, pBaseCacheRequest,
      pPropertyChangeEventHandler, pProperties);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to call IUIAutomation::AddPropertyChangedEventHandler",
              GetCurrentThreadId(), __LONGFILE__);
    goto CLEANUP;
  }

  AutomationEventHandler *pAutomationEventHandler =
      new AutomationEventHandler(ctx->HandleFunc);

  const PROPERTYID eventProperties[8] = {
      UIA_LiveRegionChangedEventId,
      UIA_SelectionItem_ElementSelectedEventId,
      UIA_MenuOpenedEventId,
      UIA_SelectionItem_ElementAddedToSelectionEventId,
      UIA_SelectionItem_ElementRemovedFromSelectionEventId,
      UIA_ToolTipOpenedEventId,
      UIA_Window_WindowOpenedEventId,
      UIA_SystemAlertEventId};

  for (int i = 0; i < 8; i++) {
    hr = pUIAutomation->AddAutomationEventHandler(
        eventProperties[i], pRootElement, TreeScope_Subtree, pBaseCacheRequest,
        pAutomationEventHandler);

    if (FAILED(hr)) {

      Log->Fail(L"Failed to call IUIAutomation::AddAutomationEventHandler ",
                GetCurrentThreadId(), __LONGFILE__);
      goto CLEANUP;
    }
  }

  hr = registerPropertyId(L"{92A053DA-2969-4021-BF27-514CFC2E4A69}",
                          L"ItemIndex", UIAutomationType_Int,
                          &itemIndexPropertyId);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to register ItemIndex property", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  hr = registerPropertyId(L"{ABBF5C45-5CCC-47b7-BB4E-87CB87BBD162}",
                          L"ItemCount", UIAutomationType_Int,
                          &itemCountPropertyId);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to register ItemCount property", GetCurrentThreadId(),
              __LONGFILE__);
    goto CLEANUP;
  }

  Log->Info(L"Wait for quit event", GetCurrentThreadId(), __LONGFILE__);

  HANDLE waitArray[1] = {ctx->QuitEvent};
  DWORD waitResult = WaitForMultipleObjects(1, waitArray, FALSE, INFINITE);

  switch (waitResult) {
  case WAIT_OBJECT_0 + 0: // ctx->QuitEvent
    break;
  }

CLEANUP:

  Log->Info(L"Cleanup", GetCurrentThreadId(), __LONGFILE__);

  SafeRelease(&pWindowCondition);
  SafeRelease(&pCondition);
  SafeRelease(&pRootElement);
  SafeRelease(&pWindowTreeWalker);
  SafeRelease(&pBaseTreeWalker);
  SafeRelease(&pWindowCacheRequest);
  SafeRelease(&pBaseCacheRequest);
  SafeRelease(&pUIAutomation);

  if (pProperties != nullptr) {
    SafeArrayDestroy(pProperties);
    pProperties = nullptr;
  }

  CoUninitialize();

  Log->Info(L"End UI Automation loop thread", GetCurrentThreadId(),
            __LONGFILE__);

  return S_OK;
}
