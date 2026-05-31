// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/brotli/brotli_decoder.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "core/fxcrt/check.h"
#include "core/fxcrt/data_vector.h"
#include "third_party/brotli/include/brotli/decode.h"

namespace {
constexpr size_t kMaxDecodeBytes = 64 * 1024 * 1024;
}  // namespace

DataAndBytesConsumed BrotliDecoder::Decode(pdfium::span<const uint8_t> src_span,
                                           uint32_t estimated_decode_size) {
  if (src_span.empty()) {
    return {DataVector<uint8_t>(), 0u};
  }
  if (estimated_decode_size == 0) {
    estimated_decode_size = static_cast<uint32_t>(src_span.size());
  }
  if (estimated_decode_size > kMaxDecodeBytes) {
    return {DataVector<uint8_t>(), 0u};
  }
  using ScopedBrotliDecoderState = std::unique_ptr<BrotliDecoderState, decltype(&BrotliDecoderDestroyInstance)>;
  ScopedBrotliDecoderState state(BrotliDecoderCreateInstance(nullptr, nullptr, nullptr), &BrotliDecoderDestroyInstance);
  if (!state) {
    return {DataVector<uint8_t>(), 0u};
  }

  DataVector<uint8_t> decoded_buffer(estimated_decode_size);
  size_t available_in = src_span.size();
  const uint8_t* next_in = src_span.data();
  size_t total_out = 0;

  while (true) {
    size_t available_out = decoded_buffer.size() - total_out;
    uint8_t* next_out = UNSAFE_BUFFERS(decoded_buffer.data() + total_out);
    BrotliDecoderResult result =
        BrotliDecoderDecompressStream(state.get(), &available_in, &next_in,
                                      &available_out, &next_out, &total_out);
    if ((result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) &&
        (decoded_buffer.size() <= kMaxDecodeBytes)) {
      uint32_t new_decode_size =
          std::min(static_cast<uint32_t>(decoded_buffer.size()) * 2,
                   static_cast<uint32_t>(kMaxDecodeBytes));
      decoded_buffer.resize(new_decode_size);
    } else {
      if (result == BROTLI_DECODER_RESULT_SUCCESS) {
        decoded_buffer.resize(total_out);
        return {std::move(decoded_buffer),
                static_cast<uint32_t>(src_span.size() - available_in)};
      } else {
        return {DataVector<uint8_t>(), 0u};
      }
    }
  }
}
