// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_CFX_TYPE1FACE_H_
#define CORE_FXGE_CFX_TYPE1FACE_H_

#include <memory>

#include "core/fxcrt/retain_ptr.h"
#include "core/fxge/cfx_face.h"

class CFX_Type1Face final : public CFX_Face {
 public:
  static RetainPtr<CFX_Type1Face> Create(
      RetainPtr<Retainable> cache_entry,
      RetainPtr<CFX_ReadOnlySpanStream> font_stream,
      uint32_t face_index,
      std::unique_ptr<SkrifaFontHolder> skrifa_font);

  CFX_Type1Face(RetainPtr<Retainable> cache_entry,
                RetainPtr<CFX_ReadOnlySpanStream> font_stream,
                FT_FaceRec* rec,
                std::unique_ptr<SkrifaFontHolder> skrifa_font);
  ~CFX_Type1Face() override;

  // CFX_Face overrides:
  CFX_Type1Face* AsType1Face() override;
  int GetCharIndex(uint32_t code) override;
  std::unique_ptr<CFX_Path> LoadGlyphPath(
      uint32_t glyph_index,
      int dest_width,
      bool is_vertical,
      const CFX_SubstFont* subst_font) override;

 private:
#if defined(PDF_ENABLE_FONTATIONS)
  std::unique_ptr<SkrifaFontHolder> const skrifa_font_;
#endif
};

#endif  // CORE_FXGE_CFX_TYPE1FACE_H_
