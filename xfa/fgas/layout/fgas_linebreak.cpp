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

namespace {

// Short aliases used to keep the kLineBreakPairTable rows below readable.
constexpr FX_LINEBREAKTYPE kLbUn = FX_LINEBREAKTYPE::kUNKNOWN;
constexpr FX_LINEBREAKTYPE kLbDb = FX_LINEBREAKTYPE::kDIRECT_BRK;
constexpr FX_LINEBREAKTYPE kLbIb = FX_LINEBREAKTYPE::kINDIRECT_BRK;
constexpr FX_LINEBREAKTYPE kLbCb = FX_LINEBREAKTYPE::kCOM_INDIRECT_BRK;
constexpr FX_LINEBREAKTYPE kLbCp = FX_LINEBREAKTYPE::kCOM_PROHIBITED_BRK;
constexpr FX_LINEBREAKTYPE kLbPb = FX_LINEBREAKTYPE::kPROHIBITED_BRK;

using LineBreakPairRow = std::array<const FX_LINEBREAKTYPE, 38>;
constexpr std::array<const LineBreakPairRow, 38> kLineBreakPairTable = {{
    {kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbPb,
     kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbCp,
     kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbPb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbPb, kLbIb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbPb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbIb, kLbIb,
     kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbPb, kLbCb,
     kLbPb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbIb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbIb, kLbIb,
     kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbPb, kLbCb,
     kLbPb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbDb, kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbDb, kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbIb, kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbIb, kLbIb, kLbDb, kLbDb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbIb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbIb, kLbIb, kLbIb, kLbDb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbIb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbIb, kLbIb, kLbDb, kLbDb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbIb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbIb, kLbIb,
     kLbIb, kLbIb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbIb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbIb, kLbIb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbDb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbIb, kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbDb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbDb, kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbIb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbIb, kLbIb,
     kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbPb, kLbCb,
     kLbPb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbDb, kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbDb, kLbPb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb,
     kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbPb, kLbDb,
     kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbIb, kLbIb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbIb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbIb, kLbIb,
     kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbPb, kLbCb,
     kLbPb, kLbIb, kLbIb, kLbIb, kLbIb, kLbIb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbIb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbIb, kLbIb, kLbIb, kLbIb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbIb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbIb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbDb, kLbPb, kLbPb, kLbPb, kLbDb, kLbPb, kLbDb, kLbIb,
     kLbDb, kLbDb, kLbDb, kLbPb, kLbPb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbPb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
    {kLbDb, kLbPb, kLbIb, kLbDb, kLbIb, kLbPb, kLbPb, kLbPb, kLbDb, kLbDb,
     kLbDb, kLbDb, kLbDb, kLbDb, kLbIb, kLbIb, kLbDb, kLbDb, kLbPb, kLbCb,
     kLbPb, kLbDb, kLbDb, kLbDb, kLbDb, kLbDb, kLbUn, kLbUn, kLbUn, kLbUn,
     kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn, kLbUn},
}};

}  // namespace

FX_LINEBREAKTYPE GetLineBreakTypeFromPair(FX_BREAKPROPERTY curr_char,
                                          FX_BREAKPROPERTY next_char) {
  const size_t row = static_cast<size_t>(curr_char);
  const size_t col = static_cast<size_t>(next_char);
  return kLineBreakPairTable[row][col];
}
