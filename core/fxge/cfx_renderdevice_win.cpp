// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_renderdevice.h"

#include <memory>
#include <utility>

#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/renderdevicedriver_iface.h"
#include "core/fxge/win32/cgdi_display_driver.h"
#include "core/fxge/win32/cgdi_printer_driver.h"
#include "core/fxge/win32/cps_printer_driver.h"
#include "core/fxge/win32/ctext_only_printer_driver.h"

WindowsPrintMode g_pdfium_print_mode = WindowsPrintMode::kEmf;

namespace {

std::unique_ptr<RenderDeviceDriverIface> CreateDriver(
    HDC hDC,
    CFX_PSFontTracker* ps_font_tracker,
    const EncoderIface* encoder_iface) {
  int device_type = GetDeviceCaps(hDC, TECHNOLOGY);
  int obj_type = GetObjectType(hDC);
  bool is_printer = obj_type == OBJ_METADC || obj_type == OBJ_ENHMETADC ||
                    device_type == DT_RASPRINTER || device_type == DT_PLOTTER;

  if (!is_printer) {
    return std::make_unique<CGdiDisplayDriver>(hDC);
  }

  if (g_pdfium_print_mode == WindowsPrintMode::kTextOnly) {
    return std::make_unique<CTextOnlyPrinterDriver>(hDC);
  }

  if (g_pdfium_print_mode == WindowsPrintMode::kPostScript2 ||
      g_pdfium_print_mode == WindowsPrintMode::kPostScript3 ||
      g_pdfium_print_mode == WindowsPrintMode::kPostScript3Type42) {
    CFX_PSRenderer::RenderingLevel level;
    if (g_pdfium_print_mode == WindowsPrintMode::kPostScript2) {
      level = CFX_PSRenderer::RenderingLevel::kLevel2;
    } else if (g_pdfium_print_mode == WindowsPrintMode::kPostScript3) {
      level = CFX_PSRenderer::RenderingLevel::kLevel3;
    } else {
      level = CFX_PSRenderer::RenderingLevel::kLevel3Type42;
    }
    return std::make_unique<CPSPrinterDriver>(hDC, level, ps_font_tracker,
                                              encoder_iface);
  }

  // TODO(thestig): Remove this mode.
  if (g_pdfium_print_mode == WindowsPrintMode::kPostScript2PassThrough ||
      g_pdfium_print_mode == WindowsPrintMode::kPostScript3PassThrough ||
      g_pdfium_print_mode == WindowsPrintMode::kPostScript3Type42PassThrough) {
    CFX_PSRenderer::RenderingLevel level;
    if (g_pdfium_print_mode == WindowsPrintMode::kPostScript2PassThrough) {
      level = CFX_PSRenderer::RenderingLevel::kLevel2;
    } else if (g_pdfium_print_mode ==
               WindowsPrintMode::kPostScript3PassThrough) {
      level = CFX_PSRenderer::RenderingLevel::kLevel3;
    } else {
      level = CFX_PSRenderer::RenderingLevel::kLevel3Type42;
    }
    return std::make_unique<CPSPrinterDriver>(hDC, level, ps_font_tracker,
                                              encoder_iface);
  }

  return std::make_unique<CGdiPrinterDriver>(hDC);
}

}  // namespace

bool CFX_RenderDevice::InitWithWindowsDevice(
    HDC hDC,
    CFX_PSFontTracker* ps_font_tracker) {
  const EncoderIface* encoder_iface = CFX_GEModule::Get()->GetEncoderIface();
  std::unique_ptr<RenderDeviceDriverIface> driver =
      CreateDriver(hDC, ps_font_tracker, encoder_iface);
  if (!driver) {
    return false;
  }
  SetDeviceDriver(std::move(driver));
  return true;
}
