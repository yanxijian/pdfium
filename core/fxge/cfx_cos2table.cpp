// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_cos2table.h"

#include "core/fxcrt/byteorder.h"

CFX_COS2Table::CFX_COS2Table() = default;

CFX_COS2Table::CFX_COS2Table(pdfium::span<const uint8_t> data) {
  if (data.size() < 42u) {
    return;
  }
  panose_ = {data[32], data[33]};

  if (data.size() < 58u) {
    return;
  }
  usb_[0] = fxcrt::GetUInt32MSBFirst(data.subspan<42u, 4u>());
  usb_[1] = fxcrt::GetUInt32MSBFirst(data.subspan<46u, 4u>());
  usb_[2] = fxcrt::GetUInt32MSBFirst(data.subspan<50u, 4u>());
  usb_[3] = fxcrt::GetUInt32MSBFirst(data.subspan<54u, 4u>());

  if (data.size() < 86u) {
    return;
  }
  csb_[0] = fxcrt::GetUInt32MSBFirst(data.subspan<78u, 4u>());
  csb_[1] = fxcrt::GetUInt32MSBFirst(data.subspan<82u, 4u>());
}

CFX_COS2Table::CFX_COS2Table(std::array<uint32_t, 4> usb,
                             std::array<uint32_t, 2> csb,
                             std::array<uint8_t, 2> panose)
    : usb_(usb), csb_(csb), panose_(panose) {}

CFX_COS2Table::~CFX_COS2Table() = default;
