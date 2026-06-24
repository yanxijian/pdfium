// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_CFX_STANDARDFONT_H_
#define CORE_FXGE_CFX_STANDARDFONT_H_

#include <stdint.h>

#include <optional>

#include "core/fxcrt/bytestring.h"

class CFX_StandardFont {
 public:
  enum Index : uint8_t {
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

  static bool IsStandardFontName(const ByteString& name);
  static bool IsSymbolicFont(Index font);
  static bool IsFixedFont(Index font);

  static std::optional<Index> GetStandardFontIndex(const ByteString& name);
  static ByteString GetCanonicalFontName(Index font);
};

#endif  // CORE_FXGE_CFX_STANDARDFONT_H_
