// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_substfont.h"

#include "core/fxge/fx_font.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CFXSubstFontTest, ApplySkewHorizontal) {
  CFX_SubstFont font;
  font.italic_angle_ = -12;

  FT_Matrix matrix = {65536, 0, 0, 65536};
  font.ApplySkew(&matrix, /*font_style=*/false, /*is_vertical=*/false);
  EXPECT_EQ(65536, matrix.xx);
  EXPECT_EQ(13762, matrix.xy);  // - (65536 * (-21) / 100) = 13762
  EXPECT_EQ(0, matrix.yx);
  EXPECT_EQ(65536, matrix.yy);
}

TEST(CFXSubstFontTest, ApplySkewVertical) {
  CFX_SubstFont font;
  font.italic_angle_ = -12;

  FT_Matrix matrix = {65536, 0, 0, 65536};
  font.ApplySkew(&matrix, /*font_style=*/false, /*is_vertical=*/true);
  EXPECT_EQ(65536, matrix.xx);
  EXPECT_EQ(0, matrix.xy);
  EXPECT_EQ(-13762, matrix.yx);  // + (65536 * (-21) / 100) = -13762
  EXPECT_EQ(65536, matrix.yy);
}

TEST(CFXSubstFontTest, ApplySkewCJK) {
  CFX_SubstFont font;
  font.subst_cjk_ = true;
  font.italic_cjk_ = true;

  FT_Matrix matrix = {65536, 0, 0, 65536};
  font.ApplySkew(&matrix, /*font_style=*/true, /*is_vertical=*/false);
  EXPECT_EQ(65536, matrix.xx);
  EXPECT_EQ(
      17694,
      matrix.xy);  // -15 deg -> -27 skew -> - (65536 * (-27) / 100) = 17694
  EXPECT_EQ(0, matrix.yx);
  EXPECT_EQ(65536, matrix.yy);
}

TEST(CFXSubstFontTest, EffectiveWeight) {
  CFX_SubstFont font;
  font.weight_ = 400;
  font.weight_cjk_ = 700;
  font.subst_cjk_ = true;

  EXPECT_EQ(400, font.GetEffectiveWeight(/*font_style=*/false));
  EXPECT_EQ(700, font.GetEffectiveWeight(/*font_style=*/true));
}

TEST(CFXSubstFontTest, EmboldenLevels) {
  CFX_SubstFont font;
  font.weight_ = 700;  // index = (700-400)/10 = 30

  FT_Matrix matrix = {65536, 0, 0, 65536};
  EXPECT_GT(font.GetEmboldenLevelForRender(/*font_style=*/false, matrix), 0);
  EXPECT_GT(font.GetEmboldenLevelForLoad(/*font_style=*/false), 0);

  font.SetIsBuiltInGenericFont();
  EXPECT_EQ(0, font.GetEmboldenLevelForRender(/*font_style=*/false, matrix));
  EXPECT_EQ(0, font.GetEmboldenLevelForLoad(/*font_style=*/false));
}

TEST(CFXSubstFontTest, EstimatedStemV) {
  CFX_SubstFont font;
  font.weight_ = 700;
  EXPECT_EQ(140, font.GetEstimatedStemV());
}

TEST(CFXSubstFontTest, IsActualFontLoaded) {
  CFX_SubstFont font;
  font.family_ = "Times New Roman";

  EXPECT_TRUE(font.IsActualFontLoaded("timesnewroman,bold"));
  EXPECT_TRUE(font.IsActualFontLoaded("timesnewromanps-bold"));
  EXPECT_FALSE(font.IsActualFontLoaded("arial,bold"));
}
