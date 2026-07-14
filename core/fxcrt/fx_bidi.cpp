// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/fx_bidi.h"

#include <unicode/ubidi.h>

#include <algorithm>
#include <limits>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_unicode.h"
#include "core/fxcrt/numerics/safe_math.h"

CFX_BidiChar::CFX_BidiChar()
    : current_segment_({0, 0, Direction::kNeutral}),
      last_segment_({0, 0, Direction::kNeutral}) {}

bool CFX_BidiChar::AppendChar(wchar_t wch) {
  Direction direction;
  switch (pdfium::unicode::GetBidiClass(wch)) {
    case FX_BIDICLASS::kL:
      direction = Direction::kLeft;
      break;
    case FX_BIDICLASS::kAN:
    case FX_BIDICLASS::kEN:
    case FX_BIDICLASS::kNSM:
    case FX_BIDICLASS::kCS:
    case FX_BIDICLASS::kES:
    case FX_BIDICLASS::kET:
    case FX_BIDICLASS::kBN:
      direction = Direction::kLeftWeak;
      break;
    case FX_BIDICLASS::kR:
    case FX_BIDICLASS::kAL:
      direction = Direction::kRight;
      break;
    default:
      direction = Direction::kNeutral;
      break;
  }

  bool bChangeDirection = (direction != current_segment_.direction);
  if (bChangeDirection) {
    StartNewSegment(direction);
  }

  current_segment_.count++;
  return bChangeDirection;
}

bool CFX_BidiChar::EndChar() {
  StartNewSegment(Direction::kNeutral);
  return last_segment_.count > 0;
}

void CFX_BidiChar::StartNewSegment(CFX_BidiChar::Direction direction) {
  last_segment_ = current_segment_;
  current_segment_.start += current_segment_.count;
  current_segment_.count = 0;
  current_segment_.direction = direction;
}

CFX_BidiString::CFX_BidiString(const WideString& str) : str_(str) {
  CFX_BidiChar bidi;
  for (wchar_t c : str_) {
    if (bidi.AppendChar(c)) {
      order_.push_back(bidi.GetSegmentInfo());
    }
  }
  if (bidi.EndChar()) {
    order_.push_back(bidi.GetSegmentInfo());
  }

  size_t nR2L = std::count_if(
      order_.begin(), order_.end(), [](const CFX_BidiChar::Segment& seg) {
        return seg.direction == CFX_BidiChar::Direction::kRight;
      });

  size_t nL2R = std::count_if(
      order_.begin(), order_.end(), [](const CFX_BidiChar::Segment& seg) {
        return seg.direction == CFX_BidiChar::Direction::kLeft;
      });

  if (nR2L > 0 && nR2L >= nL2R) {
    SetOverallDirectionRight();
  }
}

CFX_BidiString::~CFX_BidiString() = default;

CFX_BidiChar::Direction CFX_BidiString::OverallDirection() const {
  DCHECK_NE(overall_direction_, CFX_BidiChar::Direction::kNeutral);
  DCHECK_NE(overall_direction_, CFX_BidiChar::Direction::kLeftWeak);
  return overall_direction_;
}

void CFX_BidiString::SetOverallDirectionLeft() {
  if (overall_direction_ != CFX_BidiChar::Direction::kLeft) {
    std::ranges::reverse(order_);
    overall_direction_ = CFX_BidiChar::Direction::kLeft;
  }
}

void CFX_BidiString::SetOverallDirectionRight() {
  if (overall_direction_ != CFX_BidiChar::Direction::kRight) {
    std::ranges::reverse(order_);
    overall_direction_ = CFX_BidiChar::Direction::kRight;
  }
}

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

  status = U_ZERO_ERROR;
  ubidi_setPara(paragraph_bidi_.get(), utf16_text_.data(),
                static_cast<int32_t>(utf16_text_.size()), para_level, nullptr,
                &status);
  if (U_FAILURE(status)) {
    paragraph_bidi_.reset();
  }
}

CFX_BidiResolver::~CFX_BidiResolver() = default;

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

  ubidi_setLine(paragraph_bidi_.get(), line_start,
                pdfium::CheckAdd(line_start, line_length).ValueOrDefault(-1),
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
