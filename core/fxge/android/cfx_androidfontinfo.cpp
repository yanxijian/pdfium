// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/android/cfx_androidfontinfo.h"

#include <algorithm>
#include <functional>
#include <utility>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/span.h"

namespace {

constexpr char kRoboto[] = "Roboto";
constexpr char kDroidSans[] = "Droid Sans";
constexpr char kDroidSerif[] = "Droid Serif";
constexpr char kDroidSansMono[] = "Droid Sans Mono";
constexpr char kDroidSansFallback[] = "Droid Sans Fallback";

struct SkiaFontMap {
  uint32_t family;
  const char* subst;
};

const SkiaFontMap kSkiaFontmap[] = {
    {0x058c5083, kRoboto},             // Arial
    {0x05dfade2, kDroidSerif},         // Unknown
    {0x0684317d, kDroidSerif},         // Serif
    {0x14ee2d13, kRoboto},             // Verdana
    {0x3918fe2d, kDroidSansMono},      // Courier
    {0x3b98b31c, kDroidSerif},         // Palatino
    {0x3d49f40e, kDroidSerif},         // Baskerville
    {0x432c41c5, kDroidSerif},         // Unknown
    {0x491b6ad0, kDroidSerif},         // Unknown
    {0x5612cab1, kDroidSansFallback},  // Unknown
    {0x779ce19d, kRoboto},             // Unknown
    {0x7cc9510b, kDroidSansFallback},  // Unknown
    {0x83746053, kDroidSansMono},      // Courier New
    {0xaaa60c03, kDroidSansMono},      // Monospace
    {0xbf85ff26, kDroidSerif},         // Unknown
    {0xc04fe601, kDroidSansMono},      // Monaco
    {0xca3812d5, kDroidSansFallback},  // SimHei
    {0xca383e15, kDroidSansFallback},  // SimSun
    {0xcad5eaf6, kDroidSansFallback},  // Unknown
    {0xcb7a04c8, kRoboto},             // Tahoma
    {0xfb4ce0de, kDroidSerif},         // Georgia
};

const SkiaFontMap kSkiaSansFontMap[] = {
    {0x058c5083, kDroidSans},  // Arial
    {0x14ee2d13, kDroidSans},  // Verdana
    {0x779ce19d, kDroidSans},  // Unknown
    {0xcb7a04c8, kDroidSans},  // Tahoma
    {0xfb4ce0de, kDroidSans},  // Georgia
};

const char* SkiaGetSubstFont(uint32_t hash,
                             pdfium::span<const SkiaFontMap> font_map) {
  const SkiaFontMap* it = std::ranges::lower_bound(
      font_map, hash, std::less<>{}, &SkiaFontMap::family);

  if (it != font_map.end() && it->family == hash) {
    return it->subst;
  }
  return nullptr;
}

uint32_t SkiaNormalizeFontName(ByteStringView family) {
  uint32_t hash_code = 0;
  for (unsigned char ch : family) {
    if (ch == ' ' || ch == '-' || ch == ',') {
      continue;
    }
    hash_code = 31 * hash_code + tolower(ch);
  }
  return hash_code;
}

}  // namespace

CFX_AndroidFontInfo::CFX_AndroidFontInfo() = default;

CFX_AndroidFontInfo::~CFX_AndroidFontInfo() = default;

bool CFX_AndroidFontInfo::Init(const char** user_paths) {
  AddPath("/system/fonts");
  if (user_paths) {
    UNSAFE_BUFFERS({
      for (const char** path = user_paths; *path; ++path) {
        AddPath(*path);
      }
    });
  }
  return true;
}

void* CFX_AndroidFontInfo::MapFont(int weight,
                                   bool bItalic,
                                   FX_Charset charset,
                                   int pitch_family,
                                   const ByteString& face) {
  const uint32_t face_name_hash = SkiaNormalizeFontName(face.AsStringView());
  const char* subst_name = SkiaGetSubstFont(face_name_hash, kSkiaFontmap);
  if (subst_name) {
    void* font = FindFont(weight, bItalic, charset, pitch_family, subst_name,
                          /*bMatchName=*/true);
    if (font) {
      return font;
    }
  }

  const char* subst_sans_name =
      SkiaGetSubstFont(face_name_hash, kSkiaSansFontMap);
  if (subst_sans_name) {
    void* font = FindFont(weight, bItalic, charset, pitch_family,
                          subst_sans_name, /*bMatchName=*/true);
    if (font) {
      return font;
    }
  }

  void* font = GetSubstFont(face);
  if (font) {
    return font;
  }

  return FindFont(weight, bItalic, charset, pitch_family, face,
                  /*bMatchName=*/false);
}
