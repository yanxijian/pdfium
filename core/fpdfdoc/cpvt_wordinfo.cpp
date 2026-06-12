// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfdoc/cpvt_wordinfo.h"

#include "core/fpdfdoc/cpvt_word.h"
#include "core/fxcrt/fx_codepage.h"

CPVT_WordInfo::CPVT_WordInfo()
    : Word(0),
      nCharset(FX_Charset::kANSI),
      fWordX(0.0f),
      fWordY(0.0f),
      fWordTail(0.0f),
      nFontIndex(-1),
      nDirection(CPVT_WordDirection::kLeftToRight) {}

CPVT_WordInfo::CPVT_WordInfo(uint16_t word,
                             FX_Charset charset,
                             int32_t fontIndex)
    : Word(word),
      nCharset(charset),
      fWordX(0.0f),
      fWordY(0.0f),
      fWordTail(0.0f),
      nFontIndex(fontIndex),
      nDirection(CPVT_WordDirection::kLeftToRight) {}

CPVT_WordInfo::CPVT_WordInfo(const CPVT_WordInfo& word)
    : Word(0),
      nCharset(FX_Charset::kANSI),
      fWordX(0.0f),
      fWordY(0.0f),
      fWordTail(0.0f),
      nFontIndex(-1),
      nDirection(CPVT_WordDirection::kLeftToRight) {
  operator=(word);
}

CPVT_WordInfo::~CPVT_WordInfo() = default;

CPVT_WordInfo& CPVT_WordInfo::operator=(const CPVT_WordInfo& word) {
  if (this == &word) {
    return *this;
  }

  Word = word.Word;
  nCharset = word.nCharset;
  nFontIndex = word.nFontIndex;
  fWordX = word.fWordX;
  fWordY = word.fWordY;
  fWordTail = word.fWordTail;
  nDirection = word.nDirection;
  return *this;
}

CPVT_Word::CPVT_Word() = default;
CPVT_Word::~CPVT_Word() = default;
CPVT_Word::CPVT_Word(const CPVT_Word&) = default;
CPVT_Word& CPVT_Word::operator=(const CPVT_Word&) = default;

float CPVT_Word::GetCaretX() const {
  return nDirection == CPVT_WordDirection::kRightToLeft ? ptWord.x
                                                        : ptWord.x + fWidth;
}
