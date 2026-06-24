// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_FX_STANDARDFONTS_H_
#define CORE_FXGE_FX_STANDARDFONTS_H_

#include <stddef.h>
#include <stdint.h>

#include <optional>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/span.h"

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
constexpr int FX_kNumStandardFonts = 14;

std::optional<StandardFont> FX_GetStandardFontName(ByteString* name);
bool FX_IsStandardFontName(const ByteString& name);
bool FX_IsSymbolicFont(StandardFont font);
bool FX_IsFixedFont(StandardFont font);

std::optional<StandardFont> FX_GetStandardFontIndex(const ByteString& name);
ByteString FX_GetCanonicalFontName(StandardFont font);

pdfium::span<const uint8_t> FX_GetStandardFont(StandardFont font);
pdfium::span<const uint8_t> FX_GetGenericSansFont();
pdfium::span<const uint8_t> FX_GetGenericSerifFont();

#endif  // CORE_FXGE_FX_STANDARDFONTS_H_
