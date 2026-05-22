// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "core/fxge/skrifa/src/main.rs.h"
#include "core/fxge/skrifa/src/outlines.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/path_service.h"

TEST(FxSkrifaTest, TestFoxitFixedCff) {
  std::string font_path = PathService::GetTestFilePath("fonts/foxit_fixed.cff");
  skrifa::run(rust::Str(font_path));
}

TEST(FxSkrifaTest, TestGetOs2CodePageRange) {
  std::string font_path = PathService::GetTestFilePath("fonts/foxit_fixed.cff");
  std::ifstream input(font_path, std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(input)),
                          (std::istreambuf_iterator<char>()));
  input.close();

  rust::Slice<const uint8_t> slice(
      reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
  skrifa::CodePageRange range;
  EXPECT_TRUE(skrifa::get_os2_code_page_range(slice, range));
}

TEST(FxSkrifaTest, TestGetOs2Panose) {
  std::string font_path = PathService::GetTestFilePath("fonts/foxit_fixed.cff");
  std::ifstream input(font_path, std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(input)),
                          (std::istreambuf_iterator<char>()));
  input.close();

  rust::Slice<const uint8_t> slice(
      reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
  skrifa::Os2Panose panose;
  EXPECT_TRUE(skrifa::get_os2_panose(slice, panose));
}
