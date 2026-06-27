// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_CFX_SUBSTFONT_H_
#define CORE_FXGE_CFX_SUBSTFONT_H_

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxge/freetype/fx_freetype.h"

// Represents variations to apply on top of an existing font/face to
// convert it to render as if it were an alternative font.
class CFX_SubstFont {
 public:
  CFX_SubstFont();
  ~CFX_SubstFont();

#if defined(PDF_USE_SKIA)
  int GetOriginalWeight() const;
#endif
  void UseChromeSerif();
  bool IsActualFontLoaded(const ByteString& base_font_name) const;

  // Returns negative values on failure.
  int GetWeightLevel(size_t index) const;

  // Clamps index to size of table.
  int GetWeightLevelForLoad(size_t index) const;

  int GetSkew() const;
  int GetSkewCJK() const;

  // Applies skew transformation to `matrix` based on orientation and style.
  void ApplySkew(FT_Matrix* matrix, bool font_style, bool is_vertical) const;

  // Returns the effective font weight based on style.
  int GetEffectiveWeight(bool font_style) const;

  // Returns emboldening level for rendering, or negative on failure.
  int GetEmboldenLevelForRender(bool font_style, const FT_Matrix& matrix) const;

  // Returns emboldening level for path loading.
  int GetEmboldenLevelForLoad(bool font_style) const;

  // Returns estimated stem width.
  int GetEstimatedStemV() const;

  int GetItalicAngle() const { return italic_angle_; }
  FX_Charset GetCharset() const { return charset_; }
  bool IsSymbolic() const { return charset_ == FX_Charset::kSymbol; }
  bool IsMediumWeight() const { return weight_ >= 500 && weight_ <= 600; }
  bool IsForceBold() const;

  void SetIsBuiltInGenericFont() { flag_mm_ = true; }
  bool IsBuiltInGenericFont() const { return flag_mm_; }

  ByteString family_;
  FX_Charset charset_ = FX_Charset::kANSI;
  int weight_ = 0;
  int italic_angle_ = 0;
  int weight_cjk_ = 0;
  bool subst_cjk_ = false;
  bool italic_cjk_ = false;

 private:
  bool flag_mm_ = false;
};

#endif  // CORE_FXGE_CFX_SUBSTFONT_H_
