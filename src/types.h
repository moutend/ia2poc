#pragma once

#include <cstdint>
#include <windows.h>

#include <UIAutomationClient.h>
#include <oleacc.h>

typedef struct {
  int32_t Top;
  int32_t Left;
  int32_t Width;
  int32_t Height;
  int32_t Index;
  int32_t Depth;
  int32_t ControlTypeId;
  int32_t Role;
  int32_t NameLength;
  int32_t ClassNameLength;
  int32_t AriaRoleNameLength;
  wchar_t *NameData;
  wchar_t *ClassNameData;
  wchar_t *AriaRoleNameData;
} RawElement;

typedef struct {
  int32_t EventId;
  RawElement *Element;
} RawEvent;

typedef struct {
  int32_t ProcessNameLength;
  wchar_t *ProcessNameData;
} RawProcessInfo;

typedef int64_t(__stdcall *EventHandler)(RawEvent *rawEvent,
                                         RawProcessInfo *rawProcessInfo);

typedef int64_t(__stdcall *BulkFetchHandler)(RawElement **rawElements,
                                             int32_t rawElementsLength,
                                             RawProcessInfo *rawProcessInfo);

HRESULT RawElementFromIUIAutomationElement(IUIAutomationElement *pElement,
                                           RawElement **pRawElement);

HRESULT RawEventFromIUIAutomationElement(int32_t eventId,
                                         IUIAutomationElement *pElement,
                                         RawEvent **pRawEvent);

HRESULT RawElementFromIAccessible(IAccessible *pAcc, RawElement **pRawElement);

HRESULT RawEventFromIAccessible(int32_t eventId, IAccessible *pAcc,
                                RawEvent **pRawEvent);
