// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <string>
#include <vector>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxge/cfx_font.h"
#include "core/fxge/skrifa/src/main.rs.h"
#include "core/fxge/skrifa/src/outlines.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/file_util.h"
#include "testing/utils/path_service.h"

TEST(FxSkrifaTest, TestFoxitFixedCff) {
  std::string font_path = PathService::GetTestFilePath("fonts/foxit_fixed.cff");
  skrifa::run(rust::Str(font_path));
}

TEST(FxSkrifaTest, TestGetOs2CodePageRange) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  skrifa::CodePageRange range;
  EXPECT_TRUE(font->get_os2_code_page_range(range));
  EXPECT_EQ(range.range1, 0u);
  EXPECT_EQ(range.range2, 0u);
}

TEST(FxSkrifaTest, TestGetOs2Panose) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  skrifa::Os2Panose panose;
  EXPECT_TRUE(font->get_os2_panose(panose));
  EXPECT_EQ(panose.b0, 2);
  EXPECT_EQ(panose.b1, 0);
}

TEST(FxSkrifaTest, TestGetOs2FsType) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  uint16_t fs_type = 0x1234;  // Show that is is updated.
  EXPECT_TRUE(font->get_os2_fs_type(fs_type));
  EXPECT_EQ(fs_type, 0u);
}

TEST(FxSkrifaTest, TestGetCharCodesAndIndices) {
  std::string font_path = PathService::GetTestFilePath("fonts/ahem/Ahem.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  auto results = font->get_char_codes_and_indices(0xFFFF);
  ASSERT_EQ(278u, results.size());
  EXPECT_EQ(skrifa::CharCodeAndIndex(32, 3), results[0]);
  EXPECT_EQ(skrifa::CharCodeAndIndex(33, 4), results[1]);
  EXPECT_EQ(skrifa::CharCodeAndIndex(65279, 245), results[277]);
}

TEST(FxSkrifaTest, TestIsTricky) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  EXPECT_FALSE(font->is_tricky());
}

TEST(FxSkrifaTest, TestGetNumFaces) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  EXPECT_EQ(skrifa::get_num_faces(rust::Slice<const uint8_t>(bytes)), 1u);
}

TEST(FxSkrifaTest, TestHasOutline) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  EXPECT_TRUE(font->has_outline(0));
}

TEST(FxSkrifaTest, TestRobotoGlyph167Bounds) {
  std::string font_path = PathService::GetTestFilePath(
      "../../third_party/harfbuzz/src/perf/fonts/Roboto-Regular.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());
  ASSERT_FALSE(bytes.empty());

  CFX_Font font;
  ASSERT_TRUE(font.LoadFaceZeroFromSpan(bytes, /*force_vertical=*/false,
                                        /*object_tag=*/0));
  auto face = font.GetFace();
  ASSERT_TRUE(face);

  std::cerr << "FT: Is variable="
            << (FT_HAS_MULTIPLE_MASTERS(face->GetFTFaceForTesting()) ? "yes"
                                                                     : "no")
            << std::endl;

  FX_RECT ft_bbox = face->GetGlyphBBox(167);

  FT_Face ft_face = face->GetFTFaceForTesting();
  FT_GlyphSlot glyph = ft_face->glyph;
  std::cerr << "FT Metrics for GID 167:"
            << "\n  metrics.horiBearingX=" << glyph->metrics.horiBearingX
            << "\n  metrics.width=" << glyph->metrics.width
            << "\n  metrics.horiAdvance=" << glyph->metrics.horiAdvance;

  FT_BBox cbox;
  FT_Outline_Get_CBox(&glyph->outline, &cbox);
  std::cerr << "\nFT CBox for GID 167:"
            << "\n  xMin=" << cbox.xMin << ", xMax=" << cbox.xMax
            << ", yMin=" << cbox.yMin << ", yMax=" << cbox.yMax;

  UNSAFE_BUFFERS({
    if (glyph->outline.n_points > 0) {
      FT_Pos x_min = glyph->outline.points[0].x;
      FT_Pos x_max = glyph->outline.points[0].x;
      for (int i = 1; i < glyph->outline.n_points; ++i) {
        x_min = std::min(x_min, glyph->outline.points[i].x);
        x_max = std::max(x_max, glyph->outline.points[i].x);
      }
      std::cerr << "\nFT Points range for GID 167:"
                << "\n  x_min=" << x_min << ", x_max=" << x_max;
    } else {
      std::cerr << "\nFT Outline has no points";
    }
  });
  std::cerr << std::endl;

  auto skrifa_font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(skrifa_font->is_ok());
  skrifa::BoundingBox skrifa_bbox = skrifa_font->glyph_bounds(167);

  skrifa::Outline skrifa_outline;
  ASSERT_TRUE(skrifa_font->unscaled_outline(167, skrifa_outline));
  if (!skrifa_outline.points.empty()) {
    float sk_x_min = skrifa_outline.points[0].x;
    float sk_x_max = skrifa_outline.points[0].x;
    for (const auto& p : skrifa_outline.points) {
      sk_x_min = std::min(sk_x_min, p.x);
      sk_x_max = std::max(sk_x_max, p.x);
    }
    std::cerr << "Skrifa Outline Points range for GID 167:"
              << "\n  x_min=" << sk_x_min << ", x_max=" << sk_x_max
              << std::endl;
  } else {
    std::cerr << "Skrifa Outline has no points" << std::endl;
  }

  uint16_t upem = face->GetUnitsPerEm();
  EXPECT_EQ(upem, 2048);

  FX_RECT skrifa_rect(NormalizeFontMetric(skrifa_bbox.x_min, upem),
                      NormalizeFontMetric(skrifa_bbox.y_max, upem),
                      NormalizeFontMetric(skrifa_bbox.x_max, upem),
                      NormalizeFontMetric(skrifa_bbox.y_min, upem));

  if (ft_bbox != skrifa_rect) {
    std::cerr << "TestRobotoGlyph167Bounds mismatch!"
              << "\n  FT      normalized=[" << ft_bbox.left << ","
              << ft_bbox.top << "," << ft_bbox.right << "," << ft_bbox.bottom
              << "]"
              << "\n  Skrifa  normalized=[" << skrifa_rect.left << ","
              << skrifa_rect.top << "," << skrifa_rect.right << ","
              << skrifa_rect.bottom << "]"
              << "\n  Skrifa  raw=[" << skrifa_bbox.x_min << ","
              << skrifa_bbox.y_max << "," << skrifa_bbox.x_max << ","
              << skrifa_bbox.y_min << "]" << std::endl;
  }

  EXPECT_EQ(ft_bbox.left, skrifa_rect.left);
  EXPECT_EQ(ft_bbox.top, skrifa_rect.top);
  EXPECT_EQ(ft_bbox.right, skrifa_rect.right);
  EXPECT_EQ(ft_bbox.bottom, skrifa_rect.bottom);
}

TEST(FxSkrifaTest, TestMinionCff) {
  std::string font_path = PathService::GetTestFilePath("fonts/minion.cff");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());
  ASSERT_FALSE(bytes.empty());

  CFX_Font font;
  ASSERT_TRUE(font.LoadFaceZeroFromSpan(bytes, /*force_vertical=*/false,
                                        /*object_tag=*/0));
  auto face = font.GetFace();
  ASSERT_TRUE(face);
  std::cerr << "FT: num_glyphs=" << face->GetFTFaceForTesting()->num_glyphs
            << std::endl;

  auto skrifa_font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(skrifa_font->is_ok());

  auto family_name = skrifa_font->family_name();
  auto ps_name = skrifa_font->postscript_name();
  std::cerr << "Skrifa: num_glyphs=" << skrifa_font->num_glyphs() << std::endl;
  std::cerr << "Skrifa: family_name='"
            << std::string(family_name.data(), family_name.size()) << "'"
            << std::endl;
  std::cerr << "Skrifa: postscript_name='"
            << std::string(ps_name.data(), ps_name.size()) << "'" << std::endl;

  std::cerr << "Skrifa: has_outline(55)=" << skrifa_font->has_outline(55)
            << std::endl;
  std::cerr << "FT: has_outline(55)="
            << (FT_Load_Glyph(face->GetFTFaceForTesting(), 55,
                              FT_LOAD_NO_SCALE) == 0)
            << std::endl;
}

TEST(FxSkrifaTest, TestTimesBoldGlyph104Bounds) {
  std::string font_path = PathService::GetTestFilePath("fonts/times_bold.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());
  ASSERT_FALSE(bytes.empty());

  CFX_Font font;
  ASSERT_TRUE(font.LoadFaceZeroFromSpan(bytes, /*force_vertical=*/false,
                                        /*object_tag=*/0));
  auto face = font.GetFace();
  ASSERT_TRUE(face);

  FT_Face ft_face = face->GetFTFaceForTesting();
  ASSERT_EQ(FT_Load_Glyph(ft_face, 104, FT_LOAD_NO_SCALE), 0);
  FT_Glyph_Metrics ft_metrics = ft_face->glyph->metrics;

  int ft_x_min = ft_metrics.horiBearingX;
  int ft_y_max = ft_metrics.horiBearingY;
  int ft_x_max = ft_metrics.horiBearingX + ft_metrics.width;
  int ft_y_min = ft_metrics.horiBearingY - ft_metrics.height;

  std::cerr << "FT metrics BBox: [" << ft_x_min << ", " << ft_y_min << ", "
            << ft_x_max << ", " << ft_y_max << "]" << std::endl;
  std::cerr << "FT metrics: lsb=" << ft_metrics.horiBearingX
            << ", width=" << ft_metrics.width
            << ", advance=" << ft_metrics.horiAdvance << std::endl;

  auto skrifa_font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(skrifa_font->is_ok());

  skrifa::BoundingBox skrifa_bbox = skrifa_font->glyph_bounds(104);
  std::cerr << "Skrifa BBox: [" << skrifa_bbox.x_min << ", "
            << skrifa_bbox.y_min << ", " << skrifa_bbox.x_max << ", "
            << skrifa_bbox.y_max << "]" << std::endl;
}

TEST(FxSkrifaTest, TestRobotoGlyph2344Bounds) {
  std::string font_path = PathService::GetTestFilePath("fonts/roboto.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());
  ASSERT_FALSE(bytes.empty());

  CFX_Font font;
  ASSERT_TRUE(font.LoadFaceZeroFromSpan(bytes, /*force_vertical=*/false,
                                        /*object_tag=*/0));
  auto face = font.GetFace();
  ASSERT_TRUE(face);

  FT_Face ft_face = face->GetFTFaceForTesting();
  ASSERT_EQ(FT_Load_Glyph(ft_face, 2344, FT_LOAD_NO_SCALE), 0);
  FT_Glyph_Metrics ft_metrics = ft_face->glyph->metrics;

  int ft_x_min = ft_metrics.horiBearingX;
  int ft_y_max = ft_metrics.horiBearingY;
  int ft_x_max = ft_metrics.horiBearingX + ft_metrics.width;
  int ft_y_min = ft_metrics.horiBearingY - ft_metrics.height;

  std::cerr << "FT metrics BBox: [" << ft_x_min << ", " << ft_y_min << ", "
            << ft_x_max << ", " << ft_y_max << "]" << std::endl;
  std::cerr << "FT metrics: lsb=" << ft_metrics.horiBearingX
            << ", width=" << ft_metrics.width
            << ", advance=" << ft_metrics.horiAdvance << std::endl;

  auto skrifa_font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(skrifa_font->is_ok());

  // Print GID 46 metrics (component 0)
  ASSERT_EQ(FT_Load_Glyph(ft_face, 46, FT_LOAD_NO_SCALE), 0);
  FT_Glyph_Metrics ft_metrics_46 = ft_face->glyph->metrics;
  std::cerr << "FT GID 46: lsb=" << ft_metrics_46.horiBearingX
            << ", width=" << ft_metrics_46.width
            << ", advance=" << ft_metrics_46.horiAdvance << std::endl;
  skrifa::BoundingBox skrifa_bbox_46 = skrifa_font->glyph_bounds(46);
  std::cerr << "Skrifa GID 46 BBox: [" << skrifa_bbox_46.x_min << ", "
            << skrifa_bbox_46.y_min << ", " << skrifa_bbox_46.x_max << ", "
            << skrifa_bbox_46.y_max << "]" << std::endl;

  // Print GID 2344 original LSB (from hmtx) if we can bypass the override?
  // We can't bypass it easily unless we look at the raw hmtx, but we saw it was
  // -74 in the rust print.

  skrifa::BoundingBox skrifa_bbox = skrifa_font->glyph_bounds(2344);
  std::cerr << "Skrifa GID 2344 BBox: [" << skrifa_bbox.x_min << ", "
            << skrifa_bbox.y_min << ", " << skrifa_bbox.x_max << ", "
            << skrifa_bbox.y_max << "]" << std::endl;
}
