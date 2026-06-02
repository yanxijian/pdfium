// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "core/fxcrt/check.h"
#include "core/fxge/cfx_folderfontinfo.h"
#include "core/fxge/cfx_fontmapper.h"
#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/fx_font.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/path_service.h"

TEST(FXFontTest, UnicodeFromAdobeName) {
  EXPECT_EQ(static_cast<wchar_t>(0x0000), UnicodeFromAdobeName("nonesuch"));
  EXPECT_EQ(static_cast<wchar_t>(0x0000), UnicodeFromAdobeName(""));
  EXPECT_EQ(static_cast<wchar_t>(0x00b6), UnicodeFromAdobeName("paragraph"));
  EXPECT_EQ(static_cast<wchar_t>(0x00d3), UnicodeFromAdobeName("Oacute"));
  EXPECT_EQ(static_cast<wchar_t>(0x00fe), UnicodeFromAdobeName("thorn"));
  EXPECT_EQ(static_cast<wchar_t>(0x0384), UnicodeFromAdobeName("tonos"));
  EXPECT_EQ(static_cast<wchar_t>(0x2022), UnicodeFromAdobeName("bullet"));
}

TEST(FXFontTest, AdobeNameFromUnicode) {
  EXPECT_EQ("", AdobeNameFromUnicode(0x0000));
  EXPECT_EQ("divide", AdobeNameFromUnicode(0x00f7));
  EXPECT_EQ("Lslash", AdobeNameFromUnicode(0x0141));
  EXPECT_EQ("tonos", AdobeNameFromUnicode(0x0384));
  EXPECT_EQ("afii57513", AdobeNameFromUnicode(0x0691));
  EXPECT_EQ("angkhankhuthai", AdobeNameFromUnicode(0x0e5a));
  EXPECT_EQ("Euro", AdobeNameFromUnicode(0x20ac));
}

TEST(FXFontTest, ReadFontNameFromMicrosoftEntries) {
  std::string test_data_dir = PathService::GetTestDataDir();
  ASSERT_FALSE(test_data_dir.empty());

  CFX_FontMapper font_mapper;

  {
    // |folder_font_info| has to be deallocated before the |font_mapper| or we
    // run into UnownedPtr class issues with ASAN.
    CFX_FolderFontInfo folder_font_info;
    folder_font_info.AddPath(
        (test_data_dir + PATH_SEPARATOR + "font_tests").c_str());

    font_mapper.SetSystemFontInfo(
        CFX_GEModule::Get()->GetPlatform()->CreateDefaultSystemFontInfo());
    folder_font_info.EnumFontList(&font_mapper);
  }

  ASSERT_EQ(1u, font_mapper.GetFaceSize());
  ASSERT_EQ("Test", font_mapper.GetFaceName(0));
}

TEST(FXFontTest, GetFontTable) {
  // Mock font data with 2 tables
  // Header: 0x00010000, 2 tables, searchRange=16, entrySelector=1, rangeShift=0
  // Table 1: 'head', checksum=0, offset=44, length=10
  // Table 2: 'name', checksum=0, offset=54, length=20
  // Total size = 12 + 2*16 + 10 + 20 = 74 bytes

  std::vector<uint8_t> mock_font = {
      // Header
      0x00, 0x01, 0x00, 0x00,  // scaler type
      0x00, 0x02,              // numTables = 2
      0x00, 0x10,              // searchRange = 16
      0x00, 0x01,              // entrySelector = 1
      0x00, 0x00,              // rangeShift = 0

      // Table Directory Entry 1 ('head')
      'h', 'e', 'a', 'd', 0x00, 0x00, 0x00, 0x00,  // checksum
      0x00, 0x00, 0x00, 0x2C,                      // offset = 44
      0x00, 0x00, 0x00, 0x0A,                      // length = 10

      // Table Directory Entry 2 ('name')
      'n', 'a', 'm', 'e', 0x00, 0x00, 0x00, 0x00,  // checksum
      0x00, 0x00, 0x00, 0x36,                      // offset = 54
      0x00, 0x00, 0x00, 0x14,                      // length = 20

      // Table 1 data ('head')
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,

      // Table 2 data ('name')
      0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C,
      0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24};

  pdfium::span<const uint8_t> font_span(mock_font);

  // Test finding 'head'
  uint32_t head_tag = CFX_FontMapper::MakeTag('h', 'e', 'a', 'd');
  auto head_table = GetFontTable(font_span, head_tag, 0);
  ASSERT_EQ(10u, head_table.size());
  EXPECT_EQ(0x01, head_table[0]);
  EXPECT_EQ(0x0A, head_table[9]);

  // Test finding 'name'
  uint32_t name_tag = CFX_FontMapper::MakeTag('n', 'a', 'm', 'e');
  auto name_table = GetFontTable(font_span, name_tag, 0);
  ASSERT_EQ(20u, name_table.size());
  EXPECT_EQ(0x11, name_table[0]);
  EXPECT_EQ(0x24, name_table[19]);

  // Test not finding 'gasp'
  uint32_t gasp_tag = CFX_FontMapper::MakeTag('g', 'a', 's', 'p');
  auto gasp_table = GetFontTable(font_span, gasp_tag, 0);
  EXPECT_TRUE(gasp_table.empty());

  // Test invalid data (too short)
  auto short_table = GetFontTable(font_span.first(10u), head_tag, 0);
  EXPECT_TRUE(short_table.empty());

  // Test invalid data (truncated directory)
  auto trunc_table = GetFontTable(font_span.first(20u), head_tag, 0);
  EXPECT_TRUE(trunc_table.empty());
}
