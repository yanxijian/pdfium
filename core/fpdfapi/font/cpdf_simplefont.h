// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_FONT_CPDF_SIMPLEFONT_H_
#define CORE_FPDFAPI_FONT_CPDF_SIMPLEFONT_H_

#include <stdint.h>

#include <vector>

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/font/cpdf_fontencoding.h"
#include "core/fxcrt/fx_string.h"

// 8-bit fonts, supporting at most 256 characters mapped via /Encoding.
class CPDF_SimpleFont : public CPDF_Font {
 public:
  ~CPDF_SimpleFont() override;

  // CPDF_Font
  bool IsUnicodeCompatible() const override;

  static constexpr char kNotDef[] = ".notdef";
  static constexpr char kSpace[] = "space";

 protected:
  static constexpr size_t kInternalTableSize = 256;

  CPDF_SimpleFont(CPDF_Document* document,
                  RetainPtr<CPDF_Dictionary> font_dict);

  virtual void LoadGlyphMap() = 0;

  void LoadDifferences(const CPDF_Dictionary* encoding);
  void LoadPDFEncoding(bool bEmbedded, bool bTrueType);

  FontEncoding base_encoding_ = FontEncoding::kBuiltin;
  std::vector<ByteString> char_names_;
};

#endif  // CORE_FPDFAPI_FONT_CPDF_SIMPLEFONT_H_
