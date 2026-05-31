// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <utility>

#include "core/fxcodec/brotli/brotli_decoder.h"
#include "third_party/brotli/include/brotli/decode.h"

#include "core/fpdfapi/page/cpdf_dib.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/data_vector.h"

namespace {
constexpr size_t kMaxDecodeBytes = 64 << 20;
}  // namespace

DataAndBytesConsumed BrotliDecoder::Decode(pdfium::span<const uint8_t> src_span,
                                           const CPDF_Dictionary* params,
                                           uint32_t estimated_decode_size) {
  if (src_span.empty()) {
    return {DataVector<uint8_t>(), 0u};
  }
  uint32_t length1 = 0;
  if (params) {
    length1 = params->GetIntegerFor("Length1", 0);
    estimated_decode_size = std::max(length1, estimated_decode_size);
  }
  if (estimated_decode_size == 0) {
    estimated_decode_size = static_cast<uint32_t>(src_span.size());
  }
  if (estimated_decode_size > kMaxDecodeBytes) {
    return {DataVector<uint8_t>(), 0u};
  }

  DataVector<uint8_t> decoded_buffer(estimated_decode_size);
  size_t available_in = src_span.size();
  const uint8_t* next_in = src_span.data();
  size_t available_out = estimated_decode_size;
  uint8_t* next_out = decoded_buffer.data();
  size_t total_out = 0;

  BrotliDecoderState* state =
      BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
  BrotliDecoderResult result = BrotliDecoderDecompressStream(
      state, &available_in, &next_in, &available_out, &next_out, &total_out);
  BrotliDecoderDestroyInstance(state);

  switch (result) {
    case BROTLI_DECODER_RESULT_SUCCESS:
      decoded_buffer.resize(total_out);
      return {std::move(decoded_buffer),
              static_cast<uint32_t>(src_span.size() - available_in)};
    case BROTLI_DECODER_RESULT_ERROR:
    case BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT:
      return {DataVector<uint8_t>(), 0u};
    case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT:
      if (estimated_decode_size != kMaxDecodeBytes) {
        uint32_t new_decode_size = std::min(
            estimated_decode_size * 2, static_cast<uint32_t>(kMaxDecodeBytes));
        // Pass nullptr for params in recursive calls since the estimated size
        // is now based on the previous attempt's output needs, not Length1.
        return Decode(src_span, nullptr, new_decode_size);
      }
      return {DataVector<uint8_t>(), 0u};
  }
}
