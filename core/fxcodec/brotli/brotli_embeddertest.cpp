// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/embedder_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

using BrotliEmbedderTest = EmbedderTest;

namespace {
const FPDF_LIBRARY_CONFIG kBrotliEnabledConfig = {
    .version = 6,
    .m_BrotliEnabled = true,
};
const FPDF_LIBRARY_CONFIG kWrongVersionConfig = {
    .version = 5,
    .m_BrotliEnabled = true,
};
const FPDF_LIBRARY_CONFIG kBrotliDisabledConfig = {
    .version = 6,
    .m_BrotliEnabled = false,
};
}  // namespace

TEST_F(BrotliEmbedderTest, ManyRectanglesBrotli) {
  FPDF_DestroyLibrary();
  FPDF_InitLibraryWithConfig(&kBrotliEnabledConfig);
  ASSERT_TRUE(OpenDocument("many_rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kManyRectanglesPng);
}

TEST_F(BrotliEmbedderTest, SimpleBrotliWithText) {
  FPDF_DestroyLibrary();
  FPDF_InitLibraryWithConfig(&kBrotliEnabledConfig);
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

TEST_F(BrotliEmbedderTest, BrotliRectangles) {
  FPDF_DestroyLibrary();
  FPDF_InitLibraryWithConfig(&kBrotliEnabledConfig);
  ASSERT_TRUE(OpenDocument("rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kRectanglesPng);
}

TEST_F(BrotliEmbedderTest, BrotliWithLength1Argument) {
  FPDF_DestroyLibrary();
  FPDF_InitLibraryWithConfig(&kBrotliEnabledConfig);
  ASSERT_TRUE(OpenDocument("hello_world_brotli_with_length1.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

TEST_F(BrotliEmbedderTest, BrotliDecodeDisabled) {
  FPDF_DestroyLibrary();
  FPDF_InitLibraryWithConfig(&kBrotliDisabledConfig);
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmap(bitmap.get(), "hello_world_brotli_disabled");
}

TEST_F(BrotliEmbedderTest, BrotliWithWrongVersion) {
  FPDF_DestroyLibrary();
  FPDF_InitLibraryWithConfig(&kWrongVersionConfig);
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmap(bitmap.get(), "hello_world_brotli_disabled");
}
