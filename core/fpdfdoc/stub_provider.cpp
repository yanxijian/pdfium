// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfdoc/stub_provider.h"

#include "core/fxcrt/fx_codepage.h"

StubProvider::StubProvider(IPVT_FontMap* font_map)
    : CPVT_VariableText::Provider(font_map) {}

StubProvider::~StubProvider() = default;

int StubProvider::GetCharWidth(int32_t nFontIndex, uint16_t word) {
  return 10;
}

int32_t StubProvider::GetTypeAscent(int32_t nFontIndex) {
  return 10;
}

int32_t StubProvider::GetTypeDescent(int32_t nFontIndex) {
  return -2;
}

int32_t StubProvider::GetWordFontIndex(uint16_t word,
                                       FX_Charset charset,
                                       int32_t nFontIndex) {
  return 0;
}

int32_t StubProvider::GetDefaultFontIndex() {
  return 0;
}
