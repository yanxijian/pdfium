// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is Chromium's base/rand_util_win.cc, adapted to work with PDFium's
// codebase with the following modifications:
// - Replaced namespace base with namespace pdfium.
// - Kept only the span<uint8_t> RandBytes() overload.
// - Self-contained dynamic loading of ProcessPrng from bcryptprimitives.dll.

#include "core/fxcrt/rand_util.h"

#include <windows.h>

#include "core/fxcrt/check.h"
#include "core/fxcrt/compiler_specific.h"

extern "C" {
using ProcessPrngFn = BOOL(WINAPI*)(PBYTE pbData, SIZE_T cbData);
}

namespace pdfium {

void RandBytes(pdfium::span<uint8_t> output) {
  static ProcessPrngFn process_prng_fn = []() {
    HMODULE hmod = LoadLibraryW(L"bcryptprimitives.dll");
    CHECK(hmod);
    ProcessPrngFn fn =
        reinterpret_cast<ProcessPrngFn>(GetProcAddress(hmod, "ProcessPrng"));
    CHECK(fn);
    return fn;
  }();
  BOOL success = process_prng_fn(output.data(), output.size());
  CHECK(success);
  MSAN_UNPOISON(output.data(), output.size());
}

}  // namespace pdfium
