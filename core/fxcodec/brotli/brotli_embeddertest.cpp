// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/embedder_test.h"
#include "testing/embedder_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

using BrotliEmbedderTest = EmbedderTest;

<<<<<<< PATCH SET (0b4c8d01bbb064c54a712663db5d1b101c26e6b4 Add BrotliDecode filter support to PDFium)
TEST_F(BrotliEmbedderTest, ManyRectanglesBrotli) {
||||||| BASE      (503f646f87592e3b58792eac4393e1c6d50d65d4 Add simple tests for Brotli compression)
// TODO(crbug.com/475855993): Enable for all files when BrotliDecode support is
// added.
TEST_F(BrotliEmbedderTest, DISABLED_BrotliWithAnnots) {
  ASSERT_TRUE(OpenDocument("annots_brotli.pdf"));
TEST_F(BrotliEmbedderTest, ManyRectanglesBrotli) {
=======
// TODO(crbug.com/475855993): Enable for all files when BrotliDecode support is
// added.
TEST_F(BrotliEmbedderTest, DISABLED_ManyRectanglesBrotli) {
>>>>>>> BASE      (e748e4066a269a48bb86debcc79d3d7b28684c00 Add simple tests for Brotli compression)
  ASSERT_TRUE(OpenDocument("many_rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kManyRectanglesPng);
}

TEST_F(BrotliEmbedderTest, SimpleBrotliWithText) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

TEST_F(BrotliEmbedderTest, BrotliRectangles) {
  ASSERT_TRUE(OpenDocument("rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kRectanglesPng);
}

TEST_F(BrotliEmbedderTest, BrotliWithLength1Argument) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli_with_length1.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}
