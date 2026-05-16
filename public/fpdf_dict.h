// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PUBLIC_FPDF_DICT_H_
#define PUBLIC_FPDF_DICT_H_

#include "public/fpdfview.h"

typedef const struct fpdf_dictionary_t__* FPDF_DICTIONARY;

#ifdef __cplusplus
extern "C" {
#endif

// Experimental API.
// Function: FPDF_GetPageDictionary
//       Gets the page dictionary as an FPDF_DICTIONARY, which can be passed
//       to helper functions to retrieve values associated with specific keys.
// Parameters:
//       doc        - Handle to the document.
//       page_index - Zero-based index of the page.
// Return value:
//       Returns an FPDF_DICTIONARY handle, or nullptr on failure.
FPDF_EXPORT FPDF_DICTIONARY FPDF_CALLCONV
FPDF_GetPageDictionary(FPDF_DOCUMENT doc, int page_index);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // PUBLIC_FPDF_DICT_H_
