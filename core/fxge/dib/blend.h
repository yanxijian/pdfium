// Copyright 2023 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_DIB_BLEND_H_
#define CORE_FXGE_DIB_BLEND_H_

namespace fxge {

enum class BlendMode;

// Note that Blend() only handles separable blend modes.
int Blend(BlendMode blend_mode, int back_color, int src_color);

#if defined(PDF_USE_SKIA)
// Note that BlendPremul() only handles separable blend modes, except for
// kNormal.
int BlendPremul(BlendMode blend_mode,
                int back_color,
                int back_alpha,
                int src_color,
                int src_alpha);
#endif  // defined(PDF_USE_SKIA)

}  // namespace fxge

#endif  // CORE_FXGE_DIB_BLEND_H_
