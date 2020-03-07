#pragma once

#include <cstdint>

#include "types.h"

#define export __declspec(dllexport)

extern "C" {
export void __stdcall Setup(int32_t *code, int32_t logLevel,
                            EventHandler handleFunc);
export void __stdcall Teardown(int32_t *code);
}
