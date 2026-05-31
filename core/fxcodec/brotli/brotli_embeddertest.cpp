// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/embedder_test.h"
#include "testing/embedder_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

using BrotliEmbedderTest = EmbedderTest;

TEST_F(BrotliEmbedderTest, ManyRectanglesBrotli) {
  ASSERT_TRUE(OpenDocument("many_rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kManyRectanglesPng);
}

<<<<<<< PATCH SET (eab86f20ec9e0e9a93d1d28de8d463fbb767afaf Add BrotliDecode filter support for PDF 2.0 streams)
TEST_F(BrotliEmbedderTest, SimpleBrotliWithText) {
||||||| BASE      (7017a356eaebea415f46187440c8bc6a0c5bba60 Add simple tests for Brotli compression)
TEST_F(BrotliEmbedderTest, DISABLED_SimpleBrotliWithText) {
TEST_F(BrotliEmbedderTest, SimpleBrotliWithText) {
=======
TEST_F(BrotliEmbedderTest, DISABLED_SimpleBrotliWithText) {
>>>>>>> BASE      (b3ef8584354aa39e011a4cd590f460eaa99464e7 Add simple tests for Brotli compression)
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

<<<<<<< PATCH SET (eab86f20ec9e0e9a93d1d28de8d463fbb767afaf Add BrotliDecode filter support for PDF 2.0 streams)
TEST_F(BrotliEmbedderTest, BrotliRectangles) {
||||||| BASE      (7017a356eaebea415f46187440c8bc6a0c5bba60 Add simple tests for Brotli compression)
TEST_F(BrotliEmbedderTest, DISABLED_BrotliRectangles) {
TEST_F(BrotliEmbedderTest, BrotliRectangles) {
=======
TEST_F(BrotliEmbedderTest, DISABLED_BrotliRectangles) {
>>>>>>> BASE      (b3ef8584354aa39e011a4cd590f460eaa99464e7 Add simple tests for Brotli compression)
  ASSERT_TRUE(OpenDocument("rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kRectanglesPng);
}

<<<<<<< PATCH SET (eab86f20ec9e0e9a93d1d28de8d463fbb767afaf Add BrotliDecode filter support for PDF 2.0 streams)
TEST_F(BrotliEmbedderTest, BrotliWithLength1Argument) {
||||||| BASE      (7017a356eaebea415f46187440c8bc6a0c5bba60 Add simple tests for Brotli compression)
TEST_F(BrotliEmbedderTest, DISABLED_BrotliWithLength1Argument) {
TEST_F(BrotliEmbedderTest, BrotliWithLength1Argument) {
=======
TEST_F(BrotliEmbedderTest, DISABLED_BrotliWithLength1Argument) {
>>>>>>> BASE      (b3ef8584354aa39e011a4cd590f460eaa99464e7 Add simple tests for Brotli compression)
  ASSERT_TRUE(OpenDocument("hello_world_brotli_with_length1.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}
