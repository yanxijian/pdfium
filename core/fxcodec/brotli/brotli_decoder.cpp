// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/brotli/brotli_decoder.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "core/fxcrt/check.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "third_party/brotli/include/brotli/decode.h"

namespace {
constexpr size_t kMaxDecodeBytes = 64 * 1024 * 1024;
}  // namespace

<<<<<<< PATCH SET (2b607a983e7408e7a143be488255170cac145b92 Add public API to enable/disable Brotli stream decoding)
bool BrotliDecoder::enabled_ = false;
||||||| BASE      (9564f610d1e36fa62a836eb9aa36e87422b06b59 Add BrotliDecode filter support for PDF 2.0 streams)
=======
void BrotliDecoder::BrotliDeleter::operator()(BrotliDecoderState* ptr) const {
  BrotliDecoderDestroyInstance(ptr);
}
>>>>>>> BASE      (34a520439043b163979a359d3650fd440d0b8db8 Add BrotliDecode filter support for PDF 2.0 streams)

DataAndBytesConsumed BrotliDecoder::Decode(pdfium::span<const uint8_t> src_span,
                                           uint32_t estimated_decode_size) {
  CHECK(enabled_);
  if (src_span.empty()) {
    return {DataVector<uint8_t>(), 0u};
  }
  if (estimated_decode_size == 0) {
    estimated_decode_size = pdfium::checked_cast<uint32_t>(src_span.size());
  }
  if (estimated_decode_size > kMaxDecodeBytes) {
    return {DataVector<uint8_t>(), 0u};
  }
  std::unique_ptr<BrotliDecoderState, BrotliDeleter> state(
      BrotliDecoderCreateInstance(nullptr, nullptr, nullptr));
  if (!state) {
    return {DataVector<uint8_t>(), 0u};
  }

  DataVector<uint8_t> decoded_buffer(estimated_decode_size);
  size_t available_in = src_span.size();
  const uint8_t* next_in = src_span.data();
  size_t total_out = 0;

  while (true) {
    size_t available_out = decoded_buffer.size() - total_out;
    uint8_t* next_out = pdfium::span(decoded_buffer).subspan(total_out).data();
    BrotliDecoderResult result =
        BrotliDecoderDecompressStream(state.get(), &available_in, &next_in,
                                      &available_out, &next_out, &total_out);
    if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT &&
        decoded_buffer.size() <= kMaxDecodeBytes / 2) {
      uint32_t new_decode_size =
          std::min(static_cast<uint32_t>(decoded_buffer.size()) * 2,
                   static_cast<uint32_t>(kMaxDecodeBytes));
      decoded_buffer.resize(new_decode_size);
      continue;
    }
    if (result == BROTLI_DECODER_RESULT_SUCCESS) {
      decoded_buffer.resize(total_out);
      return {std::move(decoded_buffer),
              static_cast<uint32_t>(src_span.subspan(available_in).size())};
    }
    return {DataVector<uint8_t>(), 0u};
  }
}
