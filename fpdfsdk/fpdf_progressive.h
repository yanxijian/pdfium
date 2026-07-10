// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FPDFSDK_FPDF_PROGRESSIVE_H_
#define FPDFSDK_FPDF_PROGRESSIVE_H_

#include <memory>

#include "core/fxcrt/retain_ptr.h"
#include "public/fpdf_progressive.h"

class CFX_DIBitmap;
class CFX_RenderDevice;

using ProgressiveRenderDeviceFactory =
    std::unique_ptr<CFX_RenderDevice> (*)(RetainPtr<CFX_DIBitmap>, bool);

// Internal entry point exposed for deterministic device-failure testing.
// `device_factory` must be non-null.
int StartProgressiveRenderWithDeviceFactory(
    FPDF_BITMAP bitmap,
    FPDF_PAGE page,
    int start_x,
    int start_y,
    int size_x,
    int size_y,
    int rotate,
    int flags,
    const FPDF_COLORSCHEME* color_scheme,
    IFSDK_PAUSE* pause,
    ProgressiveRenderDeviceFactory device_factory);

#endif  // FPDFSDK_FPDF_PROGRESSIVE_H_
