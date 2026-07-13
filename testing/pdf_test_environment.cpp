// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/pdf_test_environment.h"

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/span.h"
#include "core/fxge/cfx_gemodule.h"

PDFTestEnvironment::PDFTestEnvironment() = default;

PDFTestEnvironment::~PDFTestEnvironment() = default;

// testing::Environment:
void PDFTestEnvironment::SetUp() {
  const char** font_paths = test_fonts_.font_paths();
  std::optional<pdfium::span<const char* const>> user_font_paths_opt;
  if (font_paths) {
    size_t count = 0;
    UNSAFE_BUFFERS({
      while (font_paths[count]) {
        ++count;
      }
      user_font_paths_opt = pdfium::span(font_paths, count);
    });
  }
  CFX_GEModule::Create(user_font_paths_opt,
                       CFX_GEModule::RendererType::kDefault,
                       CFX_FontMgr::FontBackend::kFreeType);
}

void PDFTestEnvironment::TearDown() {
  CFX_GEModule::Destroy();
}
