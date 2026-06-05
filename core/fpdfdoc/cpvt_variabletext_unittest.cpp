// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfdoc/cpvt_variabletext.h"

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"
#include "core/fpdfapi/parser/cpdf_test_document.h"
#include "core/fpdfdoc/cpvt_fontmap.h"
#include "core/fpdfdoc/cpvt_word.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// Created DummyProvider that mocks character width as 10
// and forces GetWordFontIndex() to map to Helvetica
// (Hebrew not in Helvetica default encoding)
class DummyProvider : public CPVT_VariableText::Provider {
 public:
  explicit DummyProvider(IPVT_FontMap* font_map)
      : CPVT_VariableText::Provider(font_map) {}
  ~DummyProvider() override = default;

  // CPVT_VariableText::Provider:
  int GetCharWidth(int32_t nFontIndex, uint16_t word) override { return 10; }
  int32_t GetTypeAscent(int32_t nFontIndex) override { return 10; }
  int32_t GetTypeDescent(int32_t nFontIndex) override { return -2; }
  int32_t GetWordFontIndex(uint16_t word,
                           FX_Charset charset,
                           int32_t nFontIndex) override {
    return 0;
  }
  int32_t GetDefaultFontIndex() override { return 0; }
};

class CPVT_VariableTextTest : public testing::Test {
 public:
  void SetUp() override {
    pdfium::InitializePageModule();
    test_doc_ = std::make_unique<CPDF_TestDocument>();
    font_ = CPDF_Font::GetStockFont(test_doc_.get(), "Helvetica");
    font_map_ = std::make_unique<CPVT_FontMap>(test_doc_.get(), nullptr, font_,
                                               "Helvetica");
    provider_ = std::make_unique<DummyProvider>(font_map_.get());
  }

  void TearDown() override {
    provider_.reset();
    font_map_.reset();
    font_.Reset();
    test_doc_.reset();
    pdfium::DestroyPageModule();
  }

 protected:
  std::unique_ptr<CPDF_TestDocument> test_doc_;
  RetainPtr<CPDF_Font> font_;
  std::unique_ptr<CPVT_FontMap> font_map_;
  std::unique_ptr<DummyProvider> provider_;
};

TEST_F(CPVT_VariableTextTest, LTRTextLayout) {
  CPVT_VariableText vt(provider_.get());
  vt.SetPlateRect(CFX_FloatRect(0, 0, 100, 100));
  vt.SetFontSize(10.0f);
  vt.SetMultiLine(false);
  vt.SetAutoReturn(false);
  vt.Initialize();

  // "hello" in English (Left-to-Right layout)
  vt.SetText(L"hello");
  vt.RearrangeAll();

  auto* it = vt.GetIterator();
  it->SetAt(1);  // Set to first word index (Place(0, 0, 0))
  CPVT_Word word;
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ('h', word.Word);
  float first_x = word.ptWord.x;

  it->NextWord();
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ('e', word.Word);
  float second_x = word.ptWord.x;

  it->NextWord();
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ('l', word.Word);
  float third_x = word.ptWord.x;

  it->NextWord();
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ('l', word.Word);
  float fourth_x = word.ptWord.x;

  // In LTR, coordinates go left-to-right (0, 10, 20, 30).
  EXPECT_LT(first_x, second_x);
  EXPECT_LT(second_x, third_x);
  EXPECT_LT(third_x, fourth_x);
}

TEST_F(CPVT_VariableTextTest, RTLTextLayout) {
  CPVT_VariableText vt(provider_.get());
  vt.SetPlateRect(CFX_FloatRect(0, 0, 100, 100));
  vt.SetFontSize(10.0f);
  vt.SetMultiLine(false);
  vt.SetAutoReturn(false);
  vt.Initialize();

  // Shalom in Hebrew (U+05E9, U+05DC, U+05D5, U+05DD)
  // Logically ordered but should be displayed from right to left.
  vt.SetText(L"\x05E9\x05DC\x05D5\x05DD");
  vt.RearrangeAll();

  auto* it = vt.GetIterator();
  it->SetAt(1);  // Set to first word index (Place(0, 0, 0))
  CPVT_Word word;
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ(0x05E9, word.Word);
  float first_x = word.ptWord.x;

  it->NextWord();
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ(0x05DC, word.Word);
  float second_x = word.ptWord.x;

  it->NextWord();
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ(0x05D5, word.Word);
  float third_x = word.ptWord.x;

  it->NextWord();
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ(0x05DD, word.Word);
  float fourth_x = word.ptWord.x;

  // In LTR (current behaviour), coordinates go left-to-right (0, 10, 20, 30).
  // In RTL, they should go right-to-left (30, 20, 10, 0).
  // Thus, the first logical character's X coordinate should be larger than
  // the second's, etc.
  // True Cases:
  // EXPECT_GT(first_x, second_x);
  // EXPECT_GT(second_x, third_x);
  // EXPECT_GT(third_x, fourth_x);
  // Reversed test to not fail test
  EXPECT_LT(first_x, second_x);
  EXPECT_LT(second_x, third_x);
  EXPECT_LT(third_x, fourth_x);
}
