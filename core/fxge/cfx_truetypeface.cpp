// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_truetypeface.h"

#include <utility>

#include "core/fxcrt/check.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxge/cfx_fontmgr.h"
#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/cfx_path.h"

#if defined(PDF_ENABLE_FONTATIONS)
#include "core/fxge/cfx_face_skrifa_internal.h"
#endif

// static
RetainPtr<CFX_TrueTypeFace> CFX_TrueTypeFace::Create(
    RetainPtr<Retainable> cache_entry,
    RetainPtr<CFX_ReadOnlySpanStream> font_stream,
    uint32_t face_index,
    std::unique_ptr<SkrifaFontHolder> skrifa_font) {
  CFX_FontMgr* font_mgr = CFX_GEModule::Get()->GetFontMgr();
  pdfium::span<const uint8_t> data = font_stream->span();
  FT_FaceRec* face_rec = nullptr;
  if (FT_New_Memory_Face(font_mgr->GetFTLibrary(), data.data(),
                         pdfium::checked_cast<FT_Long>(data.size()),
                         pdfium::checked_cast<FT_Long>(face_index),
                         &face_rec) != 0) {
    return nullptr;
  }
  if (FT_Set_Pixel_Sizes(face_rec, 64, 64) != 0) {
    return nullptr;
  }
  return pdfium::WrapRetain(
      new CFX_TrueTypeFace(std::move(cache_entry), std::move(font_stream),
                           face_rec, std::move(skrifa_font)));
}

CFX_TrueTypeFace::CFX_TrueTypeFace(
    RetainPtr<Retainable> cache_entry,
    RetainPtr<CFX_ReadOnlySpanStream> font_stream,
    FT_FaceRec* rec,
    std::unique_ptr<SkrifaFontHolder> skrifa_font)
    : CFX_Face(std::move(cache_entry), std::move(font_stream), rec)
#if defined(PDF_ENABLE_FONTATIONS)
      ,
      skrifa_font_(std::move(skrifa_font))
#endif
{
}

CFX_TrueTypeFace::~CFX_TrueTypeFace() = default;

CFX_TrueTypeFace* CFX_TrueTypeFace::AsTrueTypeFace() {
  return this;
}

int CFX_TrueTypeFace::GetCharIndex(uint32_t code) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::Get()->GetFontMgr()->GetFontBackend() ==
      CFX_FontMgr::FontBackend::kFontations) {
    if (skrifa_font_ && skrifa_font_->font->is_ok()) {
      return skrifa_font_->font->unicode_to_gid(code);
    }
  }
#endif
  return CFX_Face::GetCharIndex(code);
}

std::unique_ptr<CFX_Path> CFX_TrueTypeFace::LoadGlyphPath(
    uint32_t glyph_index,
    int dest_width,
    bool is_vertical,
    const CFX_SubstFont* subst_font) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::Get()->GetFontMgr()->GetFontBackend() ==
      CFX_FontMgr::FontBackend::kFontations) {
    if (skrifa_font_ && skrifa_font_->font->is_ok()) {
      skrifa::Outline outline;
      if (skrifa_font_->font->unscaled_outline(glyph_index, outline)) {
        return ConvertOutline(outline);
      }
    }
  }
#endif
  return CFX_Face::LoadGlyphPath(glyph_index, dest_width, is_vertical,
                                 subst_font);
}
