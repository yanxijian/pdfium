// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_BROTLI_BROTLI_DECODER_H_
#define CORE_FXCODEC_BROTLI_BROTLI_DECODER_H_

#include <stdint.h>

#include "core/fxcodec/data_and_bytes_consumed.h"
#include "core/fxcrt/span.h"

class CPDF_Dictionary;
typedef struct BrotliDecoderStateStruct BrotliDecoderState;

class BrotliDecoder {
 public:
  static DataAndBytesConsumed Decode(pdfium::span<const uint8_t> src_span,
                                     uint32_t estimated_decode_size);
<<<<<<< PATCH SET (a148402402b90963da33194055cb8dff9b060d27 Add public API to enable/disable Brotli stream decoding)
  static void SetBrotliEnabled(bool enabled);
  static bool GetBrotliEnabled();

||||||| BASE      (34a520439043b163979a359d3650fd440d0b8db8 Add BrotliDecode filter support for PDF 2.0 streams)
=======
>>>>>>> BASE      (85039431fb09ff95b0bf11c9c001c8da6534f18c Add BrotliDecode filter support for PDF 2.0 streams)
 private:
  struct BrotliDeleter {
    void operator()(BrotliDecoderState* ptr) const;
  };
};

#endif  // CORE_FXCODEC_BROTLI_BROTLI_DECODER_H_
