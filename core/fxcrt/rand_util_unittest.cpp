// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/rand_util.h"

#include <array>

#include "testing/gtest/include/gtest/gtest.h"

TEST(RandUtilTest, RandBytes) {
  std::array<uint8_t, 16> buffer1;
  std::array<uint8_t, 16> buffer2;
  pdfium::RandBytes(buffer1);
  pdfium::RandBytes(buffer2);
  EXPECT_NE(buffer1, buffer2);
}
