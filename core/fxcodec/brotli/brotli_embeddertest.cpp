// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/embedder_test_constants.h"
#include "testing/embedder_test_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

using BrotliEmbedderTest = EmbedderTest;

namespace {
EmbedderTestEnvironment* env;
}  // namespace

TEST_F(BrotliEmbedderTest, ManyRectanglesBrotli) {
  env = EmbedderTestEnvironment::GetInstance();
  FPDF_DestroyLibrary();
  env->SetUpBrotli(/*brotli=*/true);
  ASSERT_TRUE(OpenDocument("many_rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kManyRectanglesPng);
}

TEST_F(BrotliEmbedderTest, SimpleBrotliWithText) {
  env = EmbedderTestEnvironment::GetInstance();
  FPDF_DestroyLibrary();
  env->SetUpBrotli(/*brotli=*/true);
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

TEST_F(BrotliEmbedderTest, BrotliRectangles) {
  env = EmbedderTestEnvironment::GetInstance();
  FPDF_DestroyLibrary();
  env->SetUpBrotli(/*brotli=*/true);
  ASSERT_TRUE(OpenDocument("rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kRectanglesPng);
}

TEST_F(BrotliEmbedderTest, BrotliWithLength1Argument) {
  env = EmbedderTestEnvironment::GetInstance();
  FPDF_DestroyLibrary();
  env->SetUpBrotli(/*brotli=*/true);
  ASSERT_TRUE(OpenDocument("hello_world_brotli_with_length1.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

TEST_F(BrotliEmbedderTest, BrotliDecodeDisabled) {
  env = EmbedderTestEnvironment::GetInstance();
  FPDF_DestroyLibrary();
  env->SetUpBrotli(/*brotli=*/false);
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmap(bitmap.get(), "hello_world_brotli_disabled");
}
