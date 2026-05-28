// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/embedder_test.h"
#include "testing/gtest/include/gtest/gtest.h"

using BrotliEmbedderTest = EmbedderTest;

// TODO(crbug.com/475855993): Enable for all files when BrotliDecode support is
// added.
TEST_F(BrotliEmbedderTest, DISABLED_BrotliWithAnnots) {
  ASSERT_TRUE(OpenDocument("annots_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);
}

TEST_F(BrotliEmbedderTest, DISABLED_SimpleBrotliWithText) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);
}

TEST_F(BrotliEmbedderTest, DISABLED_BrotliRectangles) {
  ASSERT_TRUE(OpenDocument("rectangles_brotli.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);
}

TEST_F(BrotliEmbedderTest, DISABLED_BrotliWithLength1Argument) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli_with_length1.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);
}
