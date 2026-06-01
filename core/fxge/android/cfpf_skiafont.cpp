// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/android/cfpf_skiafont.h"

#include <utility>

#include "core/fxcrt/check.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxge/fx_font.h"

CFPF_SkiaFont::CFPF_SkiaFont(RetainPtr<CFX_Face> face, FX_Charset uCharset)
    : face_(std::move(face)), charset_(uCharset) {
  CHECK(face_);
}

CFPF_SkiaFont::~CFPF_SkiaFont() = default;

ByteString CFPF_SkiaFont::GetFamilyName() {
  return face_->GetFamilyName();
}

uint32_t CFPF_SkiaFont::GetFontData(uint32_t dwTable,
                                    pdfium::span<uint8_t> pBuffer) {
  pdfium::span<const uint8_t> table_data =
      GetFontTable(face_->GetData(), dwTable, 0);
  if (table_data.empty()) {
    return 0;
  }
  if (pBuffer.size() < table_data.size()) {
    return pdfium::checked_cast<uint32_t>(table_data.size());
  }
  fxcrt::spancpy(pBuffer, table_data);
  return pdfium::checked_cast<uint32_t>(table_data.size());
}
