// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fgas/layout/fgas_linebreak.h"

#include <stddef.h>

#include <array>
#include <iterator>

#include "core/fxcrt/check.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_unicode.h"

namespace pdfium {

namespace {

constexpr LineBreakType kLBUN = LineBreakType::kUnknown;
constexpr LineBreakType kLBDB = LineBreakType::kDirectBreak;
constexpr LineBreakType kLBIB = LineBreakType::kIndirectBreak;
constexpr LineBreakType kLBCB = LineBreakType::kCommonIndirectBreak;
constexpr LineBreakType kLBCP = LineBreakType::kCommonProhibitedBreak;
constexpr LineBreakType kLBPB = LineBreakType::kProhibitedBreak;

using LineBreakPairRow = std::array<const LineBreakType, 38>;
constexpr std::array<const LineBreakPairRow, 38> kLineBreakPairTable = {{
    {kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBPB,
     kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBCP,
     kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBPB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBPB, kLBIB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBPB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBIB, kLBIB,
     kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBPB, kLBCB,
     kLBPB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBIB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBIB, kLBIB,
     kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBPB, kLBCB,
     kLBPB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBDB, kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBDB, kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBIB, kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBIB, kLBIB, kLBDB, kLBDB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBIB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBIB, kLBIB, kLBIB, kLBDB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBIB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBIB, kLBIB, kLBDB, kLBDB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBIB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBIB, kLBIB,
     kLBIB, kLBIB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBIB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBIB, kLBIB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBDB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBIB, kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBDB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBDB, kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBIB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBIB, kLBIB,
     kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBPB, kLBCB,
     kLBPB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBDB, kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBDB, kLBPB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB,
     kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBPB, kLBDB,
     kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBIB, kLBIB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBIB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBIB, kLBIB,
     kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBPB, kLBCB,
     kLBPB, kLBIB, kLBIB, kLBIB, kLBIB, kLBIB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBIB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBIB, kLBIB, kLBIB, kLBIB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBIB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBIB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBDB, kLBPB, kLBPB, kLBPB, kLBDB, kLBPB, kLBDB, kLBIB,
     kLBDB, kLBDB, kLBDB, kLBPB, kLBPB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBPB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
    {kLBDB, kLBPB, kLBIB, kLBDB, kLBIB, kLBPB, kLBPB, kLBPB, kLBDB, kLBDB,
     kLBDB, kLBDB, kLBDB, kLBDB, kLBIB, kLBIB, kLBDB, kLBDB, kLBPB, kLBCB,
     kLBPB, kLBDB, kLBDB, kLBDB, kLBDB, kLBDB, kLBUN, kLBUN, kLBUN, kLBUN,
     kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN, kLBUN},
}};

}  // namespace

LineBreakType GetLineBreakTypeFromPair(BreakProperty curr_char,
                                       BreakProperty next_char) {
  const size_t row = static_cast<size_t>(curr_char);
  const size_t col = static_cast<size_t>(next_char);
  return kLineBreakPairTable[row][col];
}

}  // namespace pdfium
