// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_CFX_COS2TABLE_H_
#define CORE_FXGE_CFX_COS2TABLE_H_

#include <stdint.h>

#include <array>

#include "core/fxcrt/span.h"

class CFX_COS2Table {
 public:
  CFX_COS2Table();
  explicit CFX_COS2Table(pdfium::span<const uint8_t> data);
  CFX_COS2Table(std::array<uint32_t, 4> usb,
                std::array<uint32_t, 2> csb,
                std::array<uint8_t, 2> panose);
  ~CFX_COS2Table();

  pdfium::span<const uint32_t> GetUsb() const { return usb_; }
  pdfium::span<const uint32_t> GetCsb() const { return csb_; }
  pdfium::span<const uint8_t> GetPanose() const { return panose_; }

  friend bool operator==(const CFX_COS2Table&, const CFX_COS2Table&) = default;

 private:
  std::array<uint32_t, 4> usb_ = {};
  std::array<uint32_t, 2> csb_ = {};
  std::array<uint8_t, 2> panose_ = {};
};

#endif  // CORE_FXGE_CFX_COS2TABLE_H_
