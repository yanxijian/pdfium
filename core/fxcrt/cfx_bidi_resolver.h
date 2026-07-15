// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_CFX_BIDI_RESOLVER_H_
#define CORE_FXCRT_CFX_BIDI_RESOLVER_H_

#include <memory>
#include <string>
#include <vector>

#include "core/fxcrt/widestring.h"

struct UBiDi;

struct UBiDiDeleter {
  void operator()(UBiDi* bidi) const;
};
using ScopedUBiDi = std::unique_ptr<UBiDi, UBiDiDeleter>;

// Wraps ICU's UBiDi engine to extract visual text runs from a logical
// paragraph. On failure to initialize ICU or process text, operations will
// safely fallback to returning empty results.
class CFX_BidiResolver {
 public:
  enum class BaseDirection {
    kAuto,
    kLeftToRight,
    kRightToLeft
  };

  struct ResolvedRun {
    int start;
    int length;
    bool is_rtl;
  };

  CFX_BidiResolver(const WideString& paragraph_text, BaseDirection direction);
  ~CFX_BidiResolver();

  bool IsValid() const;

  // Extracts visual runs for a subset of the paragraph. Returns an empty
  // vector if the line limits are invalid or if ICU initialization failed.
  std::vector<ResolvedRun> GetVisualRunsForLine(int line_start,
                                                int line_length) const;

 private:
  std::u16string utf16_text_;
  std::unique_ptr<UBiDi, UBiDiDeleter> paragraph_bidi_;
};

#endif  // CORE_FXCRT_CFX_BIDI_RESOLVER_H_
