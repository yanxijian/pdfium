// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_substfont.h"

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxge/fx_font.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CFXSubstFontTest, EffectiveSkew) {
  CFX_SubstFont font;
  font.italic_angle_ = -12;
  EXPECT_EQ(-21, font.GetEffectiveSkew(/*font_style=*/false));

  font.subst_cjk_ = true;
  font.italic_cjk_ = true;
  EXPECT_EQ(-27, font.GetEffectiveSkew(/*font_style=*/true));
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

  CFX_Matrix matrix(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
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
