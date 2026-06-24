// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_standardfont.h"

#include <optional>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/span.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CFXStandardFontTest, IsStandardFontName) {
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Courier"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Courier-Bold"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Courier-BoldOblique"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Courier-Oblique"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Helvetica"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Helvetica-Bold"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Helvetica-BoldOblique"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Helvetica-Oblique"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Times-Roman"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Times-Bold"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Times-BoldItalic"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Times-Italic"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("Symbol"));
  EXPECT_TRUE(CFX_StandardFont::IsStandardFontName("ZapfDingbats"));

  EXPECT_FALSE(CFX_StandardFont::IsStandardFontName("Courie"));
  EXPECT_FALSE(CFX_StandardFont::IsStandardFontName("Courier-"));
  EXPECT_FALSE(CFX_StandardFont::IsStandardFontName("Helvetica+Bold"));
  EXPECT_FALSE(CFX_StandardFont::IsStandardFontName("YapfDingbats"));
}

TEST(CFXStandardFontTest, CanonicalizeStandardFontName) {
  // Test exact matches (should not change name, just return ID)
  {
    ByteString name = "Courier";
    auto id = CFX_StandardFont::CanonicalizeStandardFontName(&name);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(CFX_StandardFont::kCourier, id.value());
    EXPECT_EQ("Courier", name);
  }
  {
    ByteString name = "Times-Roman";
    auto id = CFX_StandardFont::CanonicalizeStandardFontName(&name);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(CFX_StandardFont::kTimes, id.value());
    EXPECT_EQ("Times-Roman", name);
  }

  // Test alternates (should change name to canonical and return ID)
  {
    ByteString name = "Arial";
    auto id = CFX_StandardFont::CanonicalizeStandardFontName(&name);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(CFX_StandardFont::kHelvetica, id.value());
    EXPECT_EQ("Helvetica", name);
  }
  {
    ByteString name = "CourierNew";
    auto id = CFX_StandardFont::CanonicalizeStandardFontName(&name);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(CFX_StandardFont::kCourier, id.value());
    EXPECT_EQ("Courier", name);
  }
  {
    ByteString name = "TimesNewRoman,Bold";
    auto id = CFX_StandardFont::CanonicalizeStandardFontName(&name);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(CFX_StandardFont::kTimesBold, id.value());
    EXPECT_EQ("Times-Bold", name);
  }

  // Test case insensitivity for alternates
  {
    ByteString name = "arial";
    auto id = CFX_StandardFont::CanonicalizeStandardFontName(&name);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(CFX_StandardFont::kHelvetica, id.value());
    EXPECT_EQ("Helvetica", name);
  }

  // Test non-standard fonts
  {
    ByteString name = "ComicSans";
    auto id = CFX_StandardFont::CanonicalizeStandardFontName(&name);
    EXPECT_FALSE(id.has_value());
    EXPECT_EQ("ComicSans", name);  // Should not change
  }
}

TEST(CFXStandardFontTest, IsSymbolicFont) {
  EXPECT_TRUE(CFX_StandardFont::IsSymbolicFont(CFX_StandardFont::kSymbol));
  EXPECT_TRUE(CFX_StandardFont::IsSymbolicFont(CFX_StandardFont::kDingbats));

  EXPECT_FALSE(CFX_StandardFont::IsSymbolicFont(CFX_StandardFont::kCourier));
  EXPECT_FALSE(CFX_StandardFont::IsSymbolicFont(CFX_StandardFont::kHelvetica));
  EXPECT_FALSE(CFX_StandardFont::IsSymbolicFont(CFX_StandardFont::kTimes));
}

TEST(CFXStandardFontTest, IsFixedFont) {
  EXPECT_TRUE(CFX_StandardFont::IsFixedFont(CFX_StandardFont::kCourier));
  EXPECT_TRUE(CFX_StandardFont::IsFixedFont(CFX_StandardFont::kCourierBold));
  EXPECT_TRUE(
      CFX_StandardFont::IsFixedFont(CFX_StandardFont::kCourierBoldOblique));
  EXPECT_TRUE(CFX_StandardFont::IsFixedFont(CFX_StandardFont::kCourierOblique));

  EXPECT_FALSE(CFX_StandardFont::IsFixedFont(CFX_StandardFont::kHelvetica));
  EXPECT_FALSE(CFX_StandardFont::IsFixedFont(CFX_StandardFont::kTimes));
  EXPECT_FALSE(CFX_StandardFont::IsFixedFont(CFX_StandardFont::kSymbol));
}

TEST(CFXStandardFontTest, GetStandardFontID) {
  EXPECT_EQ(CFX_StandardFont::kCourier,
            CFX_StandardFont::GetStandardFontID("Courier"));
  EXPECT_EQ(CFX_StandardFont::kTimesBold,
            CFX_StandardFont::GetStandardFontID("Times-Bold"));
  EXPECT_EQ(CFX_StandardFont::kDingbats,
            CFX_StandardFont::GetStandardFontID("ZapfDingbats"));

  // Should only match canonical names, not alternates
  EXPECT_FALSE(CFX_StandardFont::GetStandardFontID("Arial").has_value());
  EXPECT_FALSE(CFX_StandardFont::GetStandardFontID("CourierNew").has_value());
}

TEST(CFXStandardFontTest, GetCanonicalFontName) {
  EXPECT_EQ("Courier",
            CFX_StandardFont::GetCanonicalFontName(CFX_StandardFont::kCourier));
  EXPECT_EQ("Helvetica-Bold", CFX_StandardFont::GetCanonicalFontName(
                                  CFX_StandardFont::kHelveticaBold));
  EXPECT_EQ("ZapfDingbats", CFX_StandardFont::GetCanonicalFontName(
                                CFX_StandardFont::kDingbats));
}

TEST(CFXStandardFontTest, GetStandardFont) {
  for (int i = 0; i < CFX_StandardFont::kNumStandardFonts; ++i) {
    auto id = static_cast<CFX_StandardFont::ID>(i);
    pdfium::span<const uint8_t> span = CFX_StandardFont::GetStandardFont(id);
    EXPECT_FALSE(span.empty());
  }
}

TEST(CFXStandardFontTest, GenericFonts) {
  EXPECT_FALSE(CFX_StandardFont::GetGenericSansFont().empty());
  EXPECT_FALSE(CFX_StandardFont::GetGenericSerifFont().empty());
}
