// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <iostream>
#include "core/fxcrt/to_underlying.h"
#include "core/fxge/cfx_fontmgr.h"
#include "core/fxge/cfx_gemodule.h"
#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/embedder_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

using BrotliEmbedderTest = EmbedderTest;
namespace {
void SetUpBrotli(bool brotli = true, int version = 6) {
  CFX_GEModule* module = CFX_GEModule::Get();
  const char** paths = module->GetUserFontPaths();
  std::cout << "ptr=" << static_cast<const void*>(paths) << "\n";
  if (paths) {
    for (int i = 0; UNSAFE_BUFFERS(paths[i]) != nullptr; ++i) {
      std::cout << "  [" << i << "] = " << UNSAFE_BUFFERS(paths[i]) << "\n";
    }
  }
  FPDF_LIBRARY_CONFIG config = {
      .version = version,
      .m_pUserFontPaths = module->GetUserFontPaths(),
      .m_pIsolate = nullptr,
      .m_v8EmbedderSlot = 0,
      .m_pPlatform = nullptr,
#ifdef PDF_USE_SKIA
      .m_RendererType = module->UseSkiaRenderer() ? FPDF_RENDERERTYPE_SKIA
                                                  : FPDF_RENDERERTYPE_AGG,
#else
      .m_RendererType = FPDF_RENDERERTYPE_AGG,
#endif
      .m_FontLibraryType = (module->GetFontMgr()->GetFontBackend() ==
                            CFX_FontMgr::FontBackend::kFontations)
                               ? FPDF_FONTBACKENDTYPE_FONTATIONS
                               : FPDF_FONTBACKENDTYPE_FREETYPE,
      .m_BrotliEnabled = brotli,
  };
  FPDF_DestroyLibrary();
  FPDF_InitLibraryWithConfig(&config);
  paths = module->GetUserFontPaths();
  std::cout << "ptr=" << static_cast<const void*>(paths) << "\n";
  if (paths) {
    for (int i = 0; UNSAFE_BUFFERS(paths[i]) != nullptr; ++i) {
      std::cout << "  [" << i << "] = " << UNSAFE_BUFFERS(paths[i]) << "\n";
    }
  }
}
}  // namespace

TEST_F(BrotliEmbedderTest, ManyRectanglesBrotli) {
  SetUpBrotli();
  ASSERT_TRUE(OpenDocument("many_rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kManyRectanglesPng);
}

TEST_F(BrotliEmbedderTest, SimpleBrotliWithText) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));
  SetUpBrotli();
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

TEST_F(BrotliEmbedderTest, BrotliRectangles) {
  SetUpBrotli();
  ASSERT_TRUE(OpenDocument("rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kRectanglesPng);
}

TEST_F(BrotliEmbedderTest, BrotliWithLength1Argument) {
  SetUpBrotli();
  ASSERT_TRUE(OpenDocument("hello_world_brotli_with_length1.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

TEST_F(BrotliEmbedderTest, BrotliDecodeDisabled) {
  SetUpBrotli(/*brotli=*/false);
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmap(bitmap.get(), "hello_world_brotli_disabled");
}

TEST_F(BrotliEmbedderTest, WrongBrotliVersion) {
  SetUpBrotli(/*brotli=*/true, /*version=*/5);
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmap(bitmap.get(), "hello_world_brotli_disabled");
}
