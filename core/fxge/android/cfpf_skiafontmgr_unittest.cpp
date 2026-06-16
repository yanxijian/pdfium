// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the cross-platform font mapping and selection logic of
// CFPF_SkiaFontMgr. It is compiled and run on Linux host tests as the
// logic does not depend on Android-specific system APIs.

#include "core/fxge/android/cfpf_skiafontmgr.h"

#include <string>
#include <utility>
#include <vector>

#include "core/fxcrt/cfx_read_only_span_stream.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxge/android/cfpf_skiafont.h"
#include "core/fxge/cfx_face.h"
#include "core/fxge/fx_font.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/file_util.h"
#include "testing/utils/path_service.h"

class TestSkiaFontMgr : public CFPF_SkiaFontMgr {
 public:
  TestSkiaFontMgr() = default;

  RetainPtr<CFX_Face> GetFontFace(const ByteString& path,
                                  int32_t face_index) override {
    last_requested_path_ = path;
    return dummy_face_;
  }

  ByteString last_requested_path_;
  RetainPtr<CFX_Face> dummy_face_;
};

class CFPF_SkiaFontMgrTest : public ::testing::Test {
 public:
  CFPF_SkiaFontMgrTest() = default;

  void SetUp() override {
    std::string font_path = PathService::GetThirdPartyFilePath(
        "NotoSansCJK/NotoSansSC-Regular.subset.otf");
    ASSERT_FALSE(font_path.empty());
    font_data_ = GetFileContents(font_path.c_str());
    ASSERT_FALSE(font_data_.empty());

    auto stream = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(font_data_);
    dummy_face_ = CFX_Face::New(nullptr, std::move(stream), 0);
    ASSERT_TRUE(dummy_face_);

    font_mgr_.dummy_face_ = dummy_face_;
  }

  CFPF_SkiaFont* CreateFont(ByteStringView family_name,
                            FX_Charset charset,
                            uint32_t style) {
    font_mgr_.last_requested_path_.clear();
    return font_mgr_.CreateFont(family_name, charset, style);
  }

  ByteString GetLastRequestedPath() { return font_mgr_.last_requested_path_; }

  void AddDummyFont(const char* font_name,
                    const char* path,
                    FX_CharsetFlag charset,
                    uint32_t glyph_count = 0,
                    uint32_t style = 0) {
    auto entry = std::make_unique<CFPF_SkiaFontMgr::Entry>();
    entry->path = path;
    entry->family = font_name;
    entry->style = style;
    entry->face_index = 0;
    entry->charsets = charset;
    entry->glyph_num = glyph_count;
    font_mgr_.font_faces_.push_back(std::move(entry));
  }

 protected:
  std::vector<uint8_t> font_data_;
  RetainPtr<CFX_Face> dummy_face_;
  TestSkiaFontMgr font_mgr_;
};

TEST_F(CFPF_SkiaFontMgrTest, CreateFontPreferOriginalNonCJK) {
  // Install both original (Arial) and substitution (Roboto).
  AddDummyFont("Arial", "path/to/Arial", FX_CharsetFlag::kANSI);
  AddDummyFont("Roboto", "path/to/Roboto", FX_CharsetFlag::kANSI);

  // Map "Arial" -> should prefer "Arial" over "Roboto" (subst).
  // Style 0.
  CFPF_SkiaFont* font = CreateFont("Arial", FX_Charset::kANSI, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/Arial");
}

TEST_F(CFPF_SkiaFontMgrTest, CreateFontFallbackToSubstNonCJK) {
  // Install only substitution (Roboto).
  AddDummyFont("Roboto", "path/to/Roboto", FX_CharsetFlag::kANSI);

  // Map "Arial" -> should fallback to "Roboto" (subst for Arial).
  CFPF_SkiaFont* font = CreateFont("Arial", FX_Charset::kANSI, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/Roboto");
}

TEST_F(CFPF_SkiaFontMgrTest, CreateFontPreferOriginalCJK) {
  // Install both original (SimSun) and substitution (Droid Sans Fallback).
  // Droid Sans Fallback might have more glyphs, but we should prefer original?
  // Actually, in old code CJK logic:
  //   if (matches_name || font->glyph_num > best_glyph_num)
  // Both match name (SimSun matches original, Droid Sans Fallback matches
  // subst). So both have matches_name = true. The last one iterated wins (in
  // old code). We added them in order: SimSun, then Droid Sans Fallback.
  // CFPF_SkiaFontMgr iterates in reverse order:
  //   for (const std::unique_ptr<Entry>& font : pdfium::Reversed(font_faces_))
  // So it will see Droid Sans Fallback first, then SimSun.
  // 1. Droid Sans Fallback: matches_name = true. Selected. best_glyph_num =
  // 2000.
  // 2. SimSun: matches_name = true. Selected (overwriting Droid Sans Fallback
  // because matches_name is true). So SimSun should win because it is iterated
  // last (first in original vector, but reversed iteration). Let's verify this
  // order-dependent behavior.

  AddDummyFont("SimSun", "path/to/SimSun", FX_CharsetFlag::kChineseSimplified,
               /*glyph_count=*/1000);
  AddDummyFont("Droid Sans Fallback", "path/to/DroidSansFallback",
               FX_CharsetFlag::kChineseSimplified, /*glyph_count=*/2000);

  CFPF_SkiaFont* font = CreateFont("SimSun", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/SimSun");
}

TEST_F(CFPF_SkiaFontMgrTest, CreateFontCJKOrderDependency1) {
  // Add Droid Bold first, then SimSun Bold.
  // Reversed: SimSun Bold, then Droid Bold.
  // Droid Bold wins because both match name, no early exit (not perfect score),
  // and Droid Bold is scanned last.
  AddDummyFont("Droid Sans Fallback", "path/to/DroidSansFallback",
               FX_CharsetFlag::kChineseSimplified, 2000,
               pdfium::kFontStyleForceBold);
  AddDummyFont("SimSun", "path/to/SimSun", FX_CharsetFlag::kChineseSimplified,
               1000, pdfium::kFontStyleForceBold);

  CFPF_SkiaFont* font = CreateFont("SimSun", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/DroidSansFallback");
}

TEST_F(CFPF_SkiaFontMgrTest, CreateFontCJKOrderDependency2) {
  // Add SimSun Bold first, then Droid Bold.
  // Reversed: Droid Bold, then SimSun Bold.
  // SimSun Bold wins because both match name, no early exit, and SimSun Bold
  // is scanned last.
  AddDummyFont("SimSun", "path/to/SimSun", FX_CharsetFlag::kChineseSimplified,
               1000, pdfium::kFontStyleForceBold);
  AddDummyFont("Droid Sans Fallback", "path/to/DroidSansFallback",
               FX_CharsetFlag::kChineseSimplified, 2000,
               pdfium::kFontStyleForceBold);

  CFPF_SkiaFont* font = CreateFont("SimSun", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/SimSun");
}

TEST_F(CFPF_SkiaFontMgrTest, CreateFontFallbackToSubstCJK) {
  // Install only substitution (Droid Sans Fallback).
  AddDummyFont("Droid Sans Fallback", "path/to/DroidSansFallback",
               FX_CharsetFlag::kChineseSimplified, /*glyph_count=*/2000);

  // Map "SimSun" -> should fallback to "Droid Sans Fallback".
  CFPF_SkiaFont* font = CreateFont("SimSun", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/DroidSansFallback");
}

TEST_F(CFPF_SkiaFontMgrTest, CreateFontCJKFallbackNoMatch) {
  // Install some other CJK font (e.g. Droid Sans Fallback).
  AddDummyFont("Droid Sans Fallback", "path/to/DroidSansFallback",
               FX_CharsetFlag::kChineseSimplified, /*glyph_count=*/2000);

  // Map some random CJK name that has no subst rule (e.g. "MyCJKFont").
  // It should find Droid Sans Fallback because it has more glyphs (2000 > 0).
  CFPF_SkiaFont* font =
      CreateFont("MyCJKFont", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/DroidSansFallback");
}

TEST_F(CFPF_SkiaFontMgrTest, CreateFontCJKDifferentGlyphCountsNoNameMatch) {
  // Install two non-matching CJK fonts with different glyph counts.
  AddDummyFont("CJK Font 1", "path/to/CJKFont1",
               FX_CharsetFlag::kChineseSimplified, /*glyph_count=*/1000);
  AddDummyFont("CJK Font 2", "path/to/CJKFont2",
               FX_CharsetFlag::kChineseSimplified, /*glyph_count=*/2000);

  // Map "MyCJKFont" (no match) -> should prefer CJK Font 2 (more glyphs).
  // Reversed iteration sees CJK Font 2 first (best_glyph_num = 2000).
  // Then CJK Font 1 (glyph_num = 1000 < 2000) -> not selected.
  CFPF_SkiaFont* font =
      CreateFont("MyCJKFont", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/CJKFont2");

  // If we added them in opposite order.
  // CJK Font 1 first, then CJK Font 2.
  // Reversed iteration sees CJK Font 1 first (best_glyph_num = 1000).
  // Then CJK Font 2 (glyph_num = 2000 > 1000) -> selected.
  // So CJK Font 2 still wins.
}
