#include "core/fpdfdoc/cpvt_variabletext.h"

#include <memory>

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"
#include "core/fpdfapi/parser/cpdf_test_document.h"
#include "core/fpdfdoc/cpvt_fontmap.h"
#include "core/fpdfdoc/cpvt_word.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"


//Created DummyProvider that mocks character width as 10.0f 
//and forces GetWordFontIndex to map to Helvetiva (Hebrew not in Helvetica default encoding)
class DummyProvider : public CPVT_VariableText::Provider {
 public:
  explicit DummyProvider(IPVT_FontMap* font_map)
      : CPVT_VariableText::Provider(font_map) {}
  ~DummyProvider() override = default;

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

TEST(CPVT_VariableTextTest, RTLTextLayout) {
  pdfium::InitializePageModule();

  {
    CPDF_TestDocument test_doc;
    RetainPtr<CPDF_Font> font = CPDF_Font::GetStockFont(&test_doc, "Helvetica");
    CPVT_FontMap font_map(&test_doc, nullptr, font, "Helvetica");
    DummyProvider provider(&font_map);

    CPVT_VariableText vt(&provider);
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
    it->SetAt(1); // Set to first word index (Place(0, 0, 0))
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
  // Thus, the first logical character's X coordinate should be larger than the second's, etc.
    EXPECT_GT(first_x, second_x);
    EXPECT_GT(second_x, third_x);
    EXPECT_GT(third_x, fourth_x);
  }

  pdfium::DestroyPageModule();
}