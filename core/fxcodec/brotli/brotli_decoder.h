// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_BROTLI_BROTLI_DECODER_H_
#define CORE_FXCODEC_BROTLI_BROTLI_DECODER_H_

#include <stdint.h>

#include "core/fxcodec/data_and_bytes_consumed.h"
#include "core/fxcrt/span.h"

class CPDF_Dictionary;
struct BrotliDecoderStateStruct;

struct BrotliDecoderStateDeleter {
  void operator()(struct BrotliDecoderStateStruct* ptr) const;
};

class BrotliDecoder {
 public:
  static DataAndBytesConsumed Decode(pdfium::span<const uint8_t> src_span,
                                     uint32_t estimated_decode_size);
<<<<<<< PATCH SET (c5db61f591fd127fb33e8773c8ee84510d47c2ee Add public API to enable/disable Brotli stream decoding)
  static void SetBrotliEnabled(bool enabled);
  static bool GetBrotliEnabled();
 private:
  struct BrotliDeleter {
    void operator()(BrotliDecoderState* ptr) const;
  };
||||||| BASE      (85039431fb09ff95b0bf11c9c001c8da6534f18c Add BrotliDecode filter support for PDF 2.0 streams)
 private:
  struct BrotliDeleter {
    void operator()(BrotliDecoderState* ptr) const;
  };
=======
>>>>>>> BASE      (baca7d2d708d7dff0a6e16b5d8d7dd2851a1cbaf Add BrotliDecode filter support for PDF 2.0 streams)
};

#endif  // CORE_FXCODEC_BROTLI_BROTLI_DECODER_H_
