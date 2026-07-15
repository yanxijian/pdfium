// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/cfx_bidi_resolver.h"

#include <limits>
#include <string>

#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/fx_string.h"
#include "third_party/icu/source/common/unicode/ubidi.h"

void UBiDiDeleter::operator()(UBiDi* bidi) const {
  ubidi_close(bidi);
}

CFX_BidiResolver::CFX_BidiResolver(const WideString& paragraph_text,
                                   BaseDirection direction) {
  if (paragraph_text.IsEmpty()) {
    return;
  }

  // A workaround for integer overflow in ICU. See crbug.com/504629701.
  // Can remove this after fixing the ICU issue, and rolling out the ICU
  // update.
  constexpr size_t kIcuRunSize = sizeof(int32_t) * 3;
  CHECK_LE(paragraph_text.GetLength(),
           std::numeric_limits<int32_t>::max() / kIcuRunSize);

  UErrorCode status = U_ZERO_ERROR;
  paragraph_bidi_.reset(
      ubidi_openSized(static_cast<int32_t>(paragraph_text.GetLength()), 0,
                      &status));
  if (U_FAILURE(status)) {
    paragraph_bidi_.reset();
    return;
  }

  utf16_text_ = FX_UTF16Encode(paragraph_text.AsStringView());

  UBiDiLevel para_level = UBIDI_DEFAULT_LTR;
  if (direction == BaseDirection::kLeftToRight) {
    para_level = UBIDI_LTR;
  } else if (direction == BaseDirection::kRightToLeft) {
    para_level = UBIDI_RTL;
  }

  ubidi_setPara(paragraph_bidi_.get(), utf16_text_.data(),
                static_cast<int32_t>(utf16_text_.size()), para_level, nullptr,
                &status);
  if (U_FAILURE(status)) {
    paragraph_bidi_.reset();
  }
}

CFX_BidiResolver::~CFX_BidiResolver() = default;

bool CFX_BidiResolver::IsValid() const {
  return paragraph_bidi_ != nullptr;
}

std::vector<CFX_BidiResolver::ResolvedRun>
CFX_BidiResolver::GetVisualRunsForLine(int line_start,
                                       int line_length) const {
  std::vector<ResolvedRun> runs;
  if (!paragraph_bidi_ || line_length <= 0) {
    return runs;
  }

  UErrorCode status = U_ZERO_ERROR;
  ScopedUBiDi line_bidi(ubidi_openSized(line_length, 0, &status));
  if (U_FAILURE(status)) {
    return runs;
  }

  FX_SAFE_INT32 safe_line_end = pdfium::CheckAdd(line_start, line_length);
  if (!safe_line_end.IsValid()) {
    return runs;
  }

  ubidi_setLine(paragraph_bidi_.get(), line_start, safe_line_end.ValueOrDie(),
                line_bidi.get(), &status);
  if (U_FAILURE(status)) {
    return runs;
  }

  int32_t run_count = ubidi_countRuns(line_bidi.get(), &status);
  if (U_FAILURE(status)) {
    return runs;
  }

  for (int32_t i = 0; i < run_count; ++i) {
    int32_t logical_start = 0;
    int32_t length = 0;
    UBiDiDirection dir =
        ubidi_getVisualRun(line_bidi.get(), i, &logical_start, &length);
    runs.push_back({logical_start + line_start, length, dir == UBIDI_RTL});
  }

  return runs;
}
