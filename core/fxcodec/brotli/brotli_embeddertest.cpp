// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/embedder_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

using BrotliEmbedderTest = EmbedderTest;

TEST_F(BrotliEmbedderTest, ManyRectanglesBrotli) {
  FPDF_SetBrotliDecodeEnabled(true);
  ASSERT_TRUE(FPDF_IsBrotliDecodeEnabled());
  ASSERT_TRUE(OpenDocument("many_rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kManyRectanglesPng);
}

TEST_F(BrotliEmbedderTest, SimpleBrotliWithText) {
  FPDF_SetBrotliDecodeEnabled(true);
  ASSERT_TRUE(FPDF_IsBrotliDecodeEnabled());
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  // TODO (crbug.com/475855993): Find out why this needs to be fuzzy.
  CompareBitmapWithFuzzyExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

TEST_F(BrotliEmbedderTest, BrotliRectangles) {
  FPDF_SetBrotliDecodeEnabled(true);
  ASSERT_TRUE(FPDF_IsBrotliDecodeEnabled());
  ASSERT_TRUE(OpenDocument("rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kRectanglesPng);
}

TEST_F(BrotliEmbedderTest, BrotliWithLength1Argument) {
  FPDF_SetBrotliDecodeEnabled(true);
  ASSERT_TRUE(FPDF_IsBrotliDecodeEnabled());
  ASSERT_TRUE(OpenDocument("hello_world_brotli_with_length1.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  // TODO (crbug.com/475855993): Find out why this needs to be fuzzy.
  CompareBitmapWithFuzzyExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

TEST_F(BrotliEmbedderTest, BrotliDecodeDisabled) {
  FPDF_SetBrotliDecodeEnabled(false);
  ASSERT_FALSE(FPDF_IsBrotliDecodeEnabled());

  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_FALSE(page);
}
