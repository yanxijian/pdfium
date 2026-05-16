// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdf_dict.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "fpdfsdk/cpdfsdk_helpers.h"

extern "C" {

FPDF_EXPORT FPDF_DICTIONARY FPDF_CALLCONV
FPDF_GetPageDictionary(FPDF_DOCUMENT doc, int page_index) {
  CPDF_Document* document = CPDFDocumentFromFPDFDocument(doc);
  if (!document) {
    return nullptr;
  }
  return FPDFDictFromCPDFDict(document->GetPageDictionary(page_index).Get());
}

}
