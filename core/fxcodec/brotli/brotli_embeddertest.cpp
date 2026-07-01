// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/embedder_test_constants.h"
#include "testing/embedder_test_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
bool old_brotli_;
int old_version_;

void SetUpBrotli(bool brotli = true, int version = 6) {
  auto* env = EmbedderTestEnvironment::GetInstance();
  old_brotli_ = env->GetBrotli();
  old_version_ = env->GetVersion();
  env->TearDown();
  env->SetBrotli(brotli);
  env->SetVersion(version);
  env->SetUp();
}
}  // namespace

class BrotliEnabledEmbedderTest : public EmbedderTest {
  void SetUp() override { SetUpBrotli(); }
  void TearDown() override {
    EmbedderTest::TearDown();
    auto* env = EmbedderTestEnvironment::GetInstance();
    env->TearDown();
    env->SetBrotli(old_brotli_);
    env->SetVersion(old_version_);
    env->SetUp();
    EmbedderTest::SetUp();
  }
};

class BrotliDisabledEmbedderTest : public BrotliEnabledEmbedderTest {
  void SetUp() override { SetUpBrotli(/*brotli=*/false, /*version=*/6); }
};

class BrotliV5EmbedderTest : public BrotliEnabledEmbedderTest {
  void SetUp() override { SetUpBrotli(/*brotli=*/true, /*version=*/5); }
};

TEST_F(BrotliEnabledEmbedderTest, ManyRectanglesBrotli) {
  ASSERT_TRUE(OpenDocument("many_rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kManyRectanglesPng);
}

TEST_F(BrotliEnabledEmbedderTest, SimpleBrotliWithText) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

TEST_F(BrotliEnabledEmbedderTest, BrotliRectangles) {
  ASSERT_TRUE(OpenDocument("rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kRectanglesPng);
}

TEST_F(BrotliEnabledEmbedderTest, BrotliWithLength1Argument) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli_with_length1.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmapWithExpectationSuffix(bitmap.get(), pdfium::kHelloWorldPng);
}

TEST_F(BrotliDisabledEmbedderTest, BrotliDecodeDisabled) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmap(bitmap.get(), "hello_world_brotli_disabled");
}

TEST_F(BrotliV5EmbedderTest, WrongBrotliVersion) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmap(bitmap.get(), "hello_world_brotli_disabled");
}
