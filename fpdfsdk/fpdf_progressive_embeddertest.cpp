// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fxge/cfx_renderdevice.h"
#include "core/fxge/dib/cfx_dibitmap.h"
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

std::unique_ptr<CFX_RenderDevice> FailRenderDeviceCreation(
    RetainPtr<CFX_DIBitmap>,
    bool) {
  return nullptr;
}

}  // namespace

class FPDFProgressiveEmbedderTest : public EmbedderTest {};

TEST_F(FPDFProgressiveEmbedderTest,
       DeviceCreationFailureLeavesNoRenderContext) {
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

  EXPECT_EQ(FPDF_RENDER_FAILED,
            StartProgressiveRenderWithDeviceFactory(
                bitmap.get(), page.get(), 0, 0, 1, 1, 0, 0,
                /*color_scheme=*/nullptr, &pause, &FailRenderDeviceCreation));
  EXPECT_EQ(nullptr, cpdf_page->GetRenderContext());
  FPDF_RenderPage_Close(page.get());
}
