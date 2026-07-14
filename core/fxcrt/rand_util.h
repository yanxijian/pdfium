// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_RAND_UTIL_H_
#define CORE_FXCRT_RAND_UTIL_H_

#include <stdint.h>

#include "core/fxcrt/span.h"

namespace pdfium {

// Fills `output` with cryptographically secure random data.
void RandBytes(pdfium::span<uint8_t> output);

}  // namespace pdfium

#endif  // CORE_FXCRT_RAND_UTIL_H_
