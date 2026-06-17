// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_CFX_FACE_SKRIFA_INTERNAL_H_
#define CORE_FXGE_CFX_FACE_SKRIFA_INTERNAL_H_

#if defined(PDF_ENABLE_FONTATIONS)
#include <memory>
#include <utility>

#include "core/fxge/skrifa/src/main.rs.h"

class CFX_Path;

struct SkrifaPsFontHolder {
  explicit SkrifaPsFontHolder(rust::Box<skrifa::PsFont> f) : font(std::move(f)) {}
  rust::Box<skrifa::PsFont> font;
};

struct SkrifaOpenTypeFontHolder {
  explicit SkrifaOpenTypeFontHolder(rust::Box<skrifa::BridgeOpenTypeFont> f)
      : font(std::move(f)) {}
  rust::Box<skrifa::BridgeOpenTypeFont> font;
};

std::unique_ptr<CFX_Path> ConvertOutline(const skrifa::Outline& outline);
#endif

#endif  // CORE_FXGE_CFX_FACE_SKRIFA_INTERNAL_H_
