// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/cfx_bidi_resolver.h"

#include "core/fxcrt/widestring.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(fxcrt, BidiResolverPureLTR) {
  WideString text = L"abc";
  CFX_BidiResolver resolver(text,
                            CFX_BidiResolver::BaseDirection::kLeftToRight);
  EXPECT_TRUE(resolver.IsValid());
  auto runs = resolver.GetVisualRunsForLine(0, 3);
  ASSERT_EQ(1u, runs.size());
  EXPECT_EQ(0, runs[0].start);
  EXPECT_EQ(3, runs[0].length);
  EXPECT_FALSE(runs[0].is_rtl);
}

TEST(fxcrt, BidiResolverPureRTL) {
  // Hebrew characters Aleph, Bet, Gimel (RTL characters).
  WideString text = L"\x05D0\x05D1\x05D2";
  CFX_BidiResolver resolver(text,
                            CFX_BidiResolver::BaseDirection::kRightToLeft);
  EXPECT_TRUE(resolver.IsValid());
  auto runs = resolver.GetVisualRunsForLine(0, 3);
  ASSERT_EQ(1u, runs.size());
  EXPECT_EQ(0, runs[0].start);
  EXPECT_EQ(3, runs[0].length);
  EXPECT_TRUE(runs[0].is_rtl);
}

TEST(fxcrt, BidiResolverMixed) {
  WideString text = L"ab \x05D0\x05D1 cd";
  CFX_BidiResolver resolver(text,
                            CFX_BidiResolver::BaseDirection::kLeftToRight);
  EXPECT_TRUE(resolver.IsValid());
  auto runs = resolver.GetVisualRunsForLine(0, 8);
  ASSERT_EQ(3u, runs.size());
  // "ab " (LTR) -> "Aleph Bet" (RTL) -> " cd" (LTR)
  EXPECT_EQ(0, runs[0].start);
  EXPECT_EQ(3, runs[0].length);
  EXPECT_FALSE(runs[0].is_rtl);

  EXPECT_EQ(3, runs[1].start);
  EXPECT_EQ(2, runs[1].length);
  EXPECT_TRUE(runs[1].is_rtl);

  EXPECT_EQ(5, runs[2].start);
  EXPECT_EQ(3, runs[2].length);
  EXPECT_FALSE(runs[2].is_rtl);
}

TEST(fxcrt, BidiResolverLineWrap) {
  WideString text = L"abc def ghi";
  CFX_BidiResolver resolver(text,
                            CFX_BidiResolver::BaseDirection::kLeftToRight);
  // Get runs for the middle word "def "
  auto runs = resolver.GetVisualRunsForLine(4, 4);
  ASSERT_EQ(1u, runs.size());
  EXPECT_EQ(4, runs[0].start);
  EXPECT_EQ(4, runs[0].length);
  EXPECT_FALSE(runs[0].is_rtl);
}

TEST(fxcrt, BidiResolverEmpty) {
  CFX_BidiResolver resolver(WideString(),
                            CFX_BidiResolver::BaseDirection::kAuto);
  // Just proving it doesn't crash or leak memory.
  EXPECT_FALSE(resolver.IsValid());
  auto runs = resolver.GetVisualRunsForLine(0, 0);
  EXPECT_TRUE(runs.empty());
}
