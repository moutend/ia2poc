#pragma once

#include <windows.h>

#include "types.h"

struct LogLoopContext {
  HANDLE QuitEvent = nullptr;
};

struct UIALoopContext {
  HANDLE QuitEvent = nullptr;
  EventHandler HandleFunc = nullptr;
};

struct WinEventLoopContext {
  HANDLE QuitEvent = nullptr;
  EventHandler HandleFunc = nullptr;
  bool IsActive = true;
};
