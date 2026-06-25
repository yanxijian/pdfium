// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Based on https://github.com/googlefonts/fontations/pull/1820

#include "core/fxge/skrifa/src/outlines.h"

#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "core/fxge/skrifa/src/main.rs.h"

void skrifa::run(rust::Str font_path) {
  std::string path_str(font_path.data(), font_path.size());
  std::ifstream input(path_str, std::ios::binary);
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                             (std::istreambuf_iterator<char>()));
  input.close();

  rust::Slice<const uint8_t> slice(bytes);

  auto font = skrifa::new_font(slice, 0);
  if (!font->is_ok()) {
    return;
  }

  std::string name(font->postscript_name());
  auto family_name = std::string(font->family_name());

  assert(font->encoding() == PsEncodingKind::Standard);

  auto gid_from_code = font->code_to_gid(120);
  auto gid = font->unicode_to_gid('x');
  assert(gid_from_code == gid);

  skrifa::Outline outline;
  font->unscaled_outline(gid, outline);
  font->scaled_outline(gid, 16.0, outline);

  uint32_t period_unicode;
  assert(skrifa::agl_name_to_unicode("period", period_unicode));
  assert(period_unicode == '.');

  uint8_t period_name[64] = {};
  assert(
      skrifa::agl_unicode_to_name('.', rust::Slice<uint8_t>(period_name, 40)));
  assert(std::string_view(reinterpret_cast<const char*>(&period_name[0])) ==
         "period");
}
