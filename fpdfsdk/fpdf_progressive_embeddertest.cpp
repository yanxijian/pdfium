// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/page/cpdf_page.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "fpdfsdk/fpdf_progressive.h"
#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_progressive.h"
#include "testing/embedder_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

FPDF_BOOL NeverPause(IFSDK_PAUSE*) {
  return false;
}

}  // namespace

class FPDFProgressiveEmbedderTest : public EmbedderTest {};

TEST_F(FPDFProgressiveEmbedderTest, FailedStartClearsRenderContext) {
  ASSERT_TRUE(OpenDocument("hello_world.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap(FPDFBitmap_Create(1, 1, /*alpha=*/0));
  ASSERT_TRUE(bitmap);

  CPDF_Page* cpdf_page = CPDFPageFromFPDFPage(page.get());
  ASSERT_TRUE(cpdf_page);
  ASSERT_EQ(nullptr, cpdf_page->GetRenderContext());

  IFSDK_PAUSE pause = {};
  pause.version = 1;
  pause.NeedToPauseNow = NeverPause;

  FailNextProgressiveRenderDeviceCreationForTesting();
  EXPECT_EQ(FPDF_RENDER_FAILED,
            FPDF_RenderPageBitmap_Start(bitmap.get(), page.get(), 0, 0, 1, 1, 0,
                                        0, &pause));
  EXPECT_EQ(nullptr, cpdf_page->GetRenderContext());
  FPDF_RenderPage_Close(page.get());
}
