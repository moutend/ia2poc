#pragma once

#include <UIAutomationClient.h>
#include <cstdint>
#include <oleauto.h>
#include <windows.h>

#include "types.h"

class FocusChangeEventHandler : public IUIAutomationFocusChangedEventHandler {
public:
  FocusChangeEventHandler(EventHandler eventHandler);

  // IUnknown methods
  ULONG STDMETHODCALLTYPE AddRef();
  ULONG STDMETHODCALLTYPE Release();
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppInterface);

  // IUIAutomationFocusChangedEventHandler methods
  HRESULT STDMETHODCALLTYPE
  HandleFocusChangedEvent(IUIAutomationElement *pSender);

private:
  LONG mRefCount;
  EventHandler mEventHandler = nullptr;
  HANDLE mNotifyEvent = nullptr;
};

class PropertyChangeEventHandler
    : public IUIAutomationPropertyChangedEventHandler {
public:
  PropertyChangeEventHandler(EventHandler eventHandler);

  // IUnknown methods
  ULONG STDMETHODCALLTYPE AddRef();
  ULONG STDMETHODCALLTYPE Release();
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppInterface);

  // IUIAutomationPropertyChangedEventHandler methods
  HRESULT STDMETHODCALLTYPE HandlePropertyChangedEvent(
      IUIAutomationElement *sender, PROPERTYID propertyId, VARIANT newValue);

private:
  LONG mRefCount;
  EventHandler mEventHandler = nullptr;
  HANDLE mNotifyEvent = nullptr;
};

class AutomationEventHandler : public IUIAutomationEventHandler {
public:
  AutomationEventHandler(EventHandler eventHandler);

  // IUnknown methods
  ULONG STDMETHODCALLTYPE AddRef();
  ULONG STDMETHODCALLTYPE Release();
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppInterface);

  // IUIAutomationEventHandler methods
  HRESULT STDMETHODCALLTYPE HandleAutomationEvent(IUIAutomationElement *sender,
                                                  EVENTID eventId);

private:
  LONG mRefCount;
  EventHandler mEventHandler = nullptr;
  HANDLE mNotifyEvent = nullptr;
};
