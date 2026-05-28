// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/embedder_test.h"
#include "testing/gtest/include/gtest/gtest.h"

using BrotliEmbedderTest = EmbedderTest;

// These Documents are samples from the PDF Association
// TODO(crbug.com/475855993): Enable for all files when BrotliDecode support is
// added.
TEST_F(BrotliEmbedderTest, PrototypeA) {
  ASSERT_TRUE(OpenDocument("brotli_prototype_a.pdf"));
}

TEST_F(BrotliEmbedderTest, PrototypeB) {
  ASSERT_TRUE(OpenDocument("brotli_prototype_b.pdf"));
}

TEST_F(BrotliEmbedderTest, PrototypeC) {
  ASSERT_TRUE(OpenDocument("brotli_prototype_c.pdf"));
}
