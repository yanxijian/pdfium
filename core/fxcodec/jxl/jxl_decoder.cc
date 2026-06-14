// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/jxl/jxl_decoder.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "core/fxcrt/check.h"
#include "core/fxcrt/compiler_specific.h"

#if defined(PDF_ENABLE_RUST_JXL)
#include "third_party/rust/jxl/v0_4/wrapper/lib.rs.h"
#endif

namespace pdfium::jxl {

namespace {

// Conservative cap against decompression bombs.
//
// Keep in sync with the mental model from Blink: the jxl-rs wrapper counts
// pixels * channels (samples). 1B samples supports JPEG XL codestream level 5
// (~268M pixels) even for RGBA.
constexpr uint64_t kMaxDecodedSamples = 1024ULL * 1024 * 1024;

}  // namespace

std::optional<Info> ParseInfo(pdfium::span<const uint8_t> data) {
#if !defined(PDF_ENABLE_RUST_JXL)
  return std::nullopt;
#else
  if (data.empty()) {
    return std::nullopt;
  }

  // Use unpremultiplied alpha output.
  rust::Box<blink::jxl_rs::JxlRsDecoder> decoder =
      blink::jxl_rs::jxl_rs_decoder_create(kMaxDecodedSamples,
                                           /*premultiply_alpha=*/false);

  // jxl-rs is a streaming decoder: advance it with an empty output buffer
  // until the image header has been parsed. Each `process()` call reports
  // `Success` at a stage boundary; the first one yields the basic info.
  size_t offset = 0;
  while (!decoder->has_basic_info()) {
    pdfium::span<const uint8_t> input = data.subspan(offset);
    blink::jxl_rs::JxlRsProcessResult result = decoder->process(
        rust::Slice<const uint8_t>(input.data(), input.size()),
        rust::Slice<uint8_t>(), /*width=*/0, /*height=*/0, /*row_stride=*/0);
    if (result.status == blink::jxl_rs::JxlRsStatus::Error) {
      return std::nullopt;
    }
    offset += result.bytes_consumed;
    if (result.bytes_consumed == 0) {
      // No forward progress means truncated or corrupt input.
      return std::nullopt;
    }
  }

  blink::jxl_rs::JxlRsBasicInfo info = decoder->get_basic_info();
  if (info.width == 0 || info.height == 0) {
    return std::nullopt;
  }

  Info out;
  out.width = info.width;
  out.height = info.height;
  out.has_alpha = info.has_alpha;
  out.have_animation = info.have_animation;
  return out;
#endif
}

bool DecodeFrame0ToBgra(pdfium::span<const uint8_t> data,
                        uint8_t* dest_bgra,
                        uint32_t dest_stride,
                        uint32_t width,
                        uint32_t height) {
#if !defined(PDF_ENABLE_RUST_JXL)
  return false;
#else
  if (!dest_bgra || width == 0 || height == 0) {
    return false;
  }

  // We only support BGRA8 output for the initial integration.
  const uint32_t min_stride = width * 4u;
  if (dest_stride < min_stride) {
    return false;
  }

  rust::Box<blink::jxl_rs::JxlRsDecoder> decoder =
      blink::jxl_rs::jxl_rs_decoder_create(kMaxDecodedSamples,
                                           /*premultiply_alpha=*/false);

  // jxl-rs reports `Success` at each stage boundary (basic info, frame header,
  // decoded frame). Drive it with a small state machine, handing it the output
  // buffer only once the basic info is known, and stop after frame 0 decodes.
  enum class State { kInitial, kHaveBasicInfo, kHaveFrameHeader };
  State state = State::kInitial;
  const rust::Slice<uint8_t> output_slice(
      dest_bgra,
      static_cast<size_t>(dest_stride) * static_cast<size_t>(height));
  size_t offset = 0;

  while (true) {
    pdfium::span<const uint8_t> input = data.subspan(offset);
    rust::Slice<uint8_t> output =
        state == State::kInitial ? rust::Slice<uint8_t>() : output_slice;
    blink::jxl_rs::JxlRsProcessResult result = decoder->process(
        rust::Slice<const uint8_t>(input.data(), input.size()), output, width,
        height, static_cast<size_t>(dest_stride));
    if (result.status == blink::jxl_rs::JxlRsStatus::Error) {
      return false;
    }
    offset += result.bytes_consumed;
    if (result.status == blink::jxl_rs::JxlRsStatus::NeedMoreInput) {
      if (result.bytes_consumed > 0 && offset < data.size()) {
        continue;
      }
      // All data fed but the decoder still wants more: truncated input.
      return false;
    }

    // status == Success: advance past the stage that just completed.
    switch (state) {
      case State::kInitial: {
        blink::jxl_rs::JxlRsBasicInfo info = decoder->get_basic_info();
        if (info.width != width || info.height != height) {
          // Caller must pass dimensions matching the JXL payload.
          return false;
        }
        decoder->set_pixel_format(blink::jxl_rs::JxlRsPixelFormat::Bgra8,
                                  info.num_extra_channels);
        state = State::kHaveBasicInfo;
        break;
      }
      case State::kHaveBasicInfo:
        state = State::kHaveFrameHeader;
        break;
      case State::kHaveFrameHeader:
        // Frame 0 has been decoded into `dest_bgra`.
        return true;
    }
  }
#endif
}

}  // namespace pdfium::jxl
