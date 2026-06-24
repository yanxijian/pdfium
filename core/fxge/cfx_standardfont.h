// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_CFX_STANDARDFONT_H_
#define CORE_FXGE_CFX_STANDARDFONT_H_

#include <stddef.h>
#include <stdint.h>

#include <optional>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/span.h"

class CFX_StandardFont {
 public:
  enum ID : uint8_t {
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

  static constexpr int kNumStandardFonts = 14;

  static std::optional<ID> CanonicalizeStandardFontName(ByteString* name);
  static bool IsStandardFontName(const ByteString& name);
  static bool IsSymbolicFont(ID font);
  static bool IsFixedFont(ID font);

  static std::optional<ID> GetStandardFontID(const ByteString& name);
  static ByteString GetCanonicalFontName(ID font);

  static pdfium::span<const uint8_t> GetStandardFont(ID font);
  static pdfium::span<const uint8_t> GetGenericSansFont();
  static pdfium::span<const uint8_t> GetGenericSerifFont();
};

#endif  // CORE_FXGE_CFX_STANDARDFONT_H_
