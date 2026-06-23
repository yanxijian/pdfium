// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_CFX_STANDARDFONTS_H_
#define CORE_FXGE_CFX_STANDARDFONTS_H_

#include <stddef.h>
#include <stdint.h>

#include <optional>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/span.h"

namespace fxge {

enum class StandardFont : uint8_t {
  kCourier = 0,
  kCourierBold,
  kCourierBoldOblique,
  kCourierOblique,
  kHelvetica,
  kHelveticaBold,
  kHelveticaBoldOblique,
  kHelveticaOblique,
  kTimes,
  kTimesBold,
  kTimesBoldOblique,
  kTimesOblique,
  kSymbol,
  kDingbats,
  kLast = kDingbats
};
constexpr int kNumStandardFonts = 14;

std::optional<StandardFont> GetStandardFontName(ByteString* name);
bool IsStandardFontName(const ByteString& name);
bool IsSymbolicFont(StandardFont font);
bool IsFixedFont(StandardFont font);

std::optional<StandardFont> GetStandardFontIndex(const ByteString& name);
ByteString GetCanonicalFontName(StandardFont font);

pdfium::span<const uint8_t> GetStandardFont(StandardFont font);
pdfium::span<const uint8_t> GetGenericSansFont();
pdfium::span<const uint8_t> GetGenericSerifFont();

}  // namespace fxge

#endif  // CORE_FXGE_CFX_STANDARDFONTS_H_
