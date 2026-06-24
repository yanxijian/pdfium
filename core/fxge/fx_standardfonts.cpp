// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/fx_standardfonts.h"

#include <algorithm>
#include <array>
#include <iterator>

#include "core/fxcrt/containers/contains.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxge/fontdata/chromefontdata/chromefontdata.h"

static_assert(static_cast<int>(StandardFont::kLast) + 1 == FX_kNumStandardFonts,
              "StandardFont enum count mismatch");

namespace {

constexpr std::array<const char*, FX_kNumStandardFonts> kBase14FontNames = {{
    "Courier",
    "Courier-Bold",
    "Courier-BoldOblique",
    "Courier-Oblique",
    "Helvetica",
    "Helvetica-Bold",
    "Helvetica-BoldOblique",
    "Helvetica-Oblique",
    "Times-Roman",
    "Times-Bold",
    "Times-BoldItalic",
    "Times-Italic",
    "Symbol",
    "ZapfDingbats",
}};

struct AltFontName {
  const char* name_;  // Raw, POD struct.
  StandardFont index_;
};

constexpr AltFontName kAltFontNames[] = {
    {"Arial", StandardFont::kHelvetica},
    {"Arial,Bold", StandardFont::kHelveticaBold},
    {"Arial,BoldItalic", StandardFont::kHelveticaBoldOblique},
    {"Arial,Italic", StandardFont::kHelveticaOblique},
    {"Arial-Bold", StandardFont::kHelveticaBold},
    {"Arial-BoldItalic", StandardFont::kHelveticaBoldOblique},
    {"Arial-BoldItalicMT", StandardFont::kHelveticaBoldOblique},
    {"Arial-BoldMT", StandardFont::kHelveticaBold},
    {"Arial-Italic", StandardFont::kHelveticaOblique},
    {"Arial-ItalicMT", StandardFont::kHelveticaOblique},
    {"ArialBold", StandardFont::kHelveticaBold},
    {"ArialBoldItalic", StandardFont::kHelveticaBoldOblique},
    {"ArialItalic", StandardFont::kHelveticaOblique},
    {"ArialMT", StandardFont::kHelvetica},
    {"ArialMT,Bold", StandardFont::kHelveticaBold},
    {"ArialMT,BoldItalic", StandardFont::kHelveticaBoldOblique},
    {"ArialMT,Italic", StandardFont::kHelveticaOblique},
    {"ArialRoundedMTBold", StandardFont::kHelveticaBold},
    {"Courier", StandardFont::kCourier},
    {"Courier,Bold", StandardFont::kCourierBold},
    {"Courier,BoldItalic", StandardFont::kCourierBoldOblique},
    {"Courier,Italic", StandardFont::kCourierOblique},
    {"Courier-Bold", StandardFont::kCourierBold},
    {"Courier-BoldOblique", StandardFont::kCourierBoldOblique},
    {"Courier-Oblique", StandardFont::kCourierOblique},
    {"CourierBold", StandardFont::kCourierBold},
    {"CourierBoldItalic", StandardFont::kCourierBoldOblique},
    {"CourierItalic", StandardFont::kCourierOblique},
    {"CourierNew", StandardFont::kCourier},
    {"CourierNew,Bold", StandardFont::kCourierBold},
    {"CourierNew,BoldItalic", StandardFont::kCourierBoldOblique},
    {"CourierNew,Italic", StandardFont::kCourierOblique},
    {"CourierNew-Bold", StandardFont::kCourierBold},
    {"CourierNew-BoldItalic", StandardFont::kCourierBoldOblique},
    {"CourierNew-Italic", StandardFont::kCourierOblique},
    {"CourierNewBold", StandardFont::kCourierBold},
    {"CourierNewBoldItalic", StandardFont::kCourierBoldOblique},
    {"CourierNewItalic", StandardFont::kCourierOblique},
    {"CourierNewPS-BoldItalicMT", StandardFont::kCourierBoldOblique},
    {"CourierNewPS-BoldMT", StandardFont::kCourierBold},
    {"CourierNewPS-ItalicMT", StandardFont::kCourierOblique},
    {"CourierNewPSMT", StandardFont::kCourier},
    {"CourierStd", StandardFont::kCourier},
    {"CourierStd-Bold", StandardFont::kCourierBold},
    {"CourierStd-BoldOblique", StandardFont::kCourierBoldOblique},
    {"CourierStd-Oblique", StandardFont::kCourierOblique},
    {"Helvetica", StandardFont::kHelvetica},
    {"Helvetica,Bold", StandardFont::kHelveticaBold},
    {"Helvetica,BoldItalic", StandardFont::kHelveticaBoldOblique},
    {"Helvetica,Italic", StandardFont::kHelveticaOblique},
    {"Helvetica-Bold", StandardFont::kHelveticaBold},
    {"Helvetica-BoldItalic", StandardFont::kHelveticaBoldOblique},
    {"Helvetica-BoldOblique", StandardFont::kHelveticaBoldOblique},
    {"Helvetica-Italic", StandardFont::kHelveticaOblique},
    {"Helvetica-Oblique", StandardFont::kHelveticaOblique},
    {"HelveticaBold", StandardFont::kHelveticaBold},
    {"HelveticaBoldItalic", StandardFont::kHelveticaBoldOblique},
    {"HelveticaItalic", StandardFont::kHelveticaOblique},
    {"Symbol", StandardFont::kSymbol},
    {"SymbolMT", StandardFont::kSymbol},
    {"Times-Bold", StandardFont::kTimesBold},
    {"Times-BoldItalic", StandardFont::kTimesBoldOblique},
    {"Times-Italic", StandardFont::kTimesOblique},
    {"Times-Roman", StandardFont::kTimes},
    {"TimesBold", StandardFont::kTimesBold},
    {"TimesBoldItalic", StandardFont::kTimesBoldOblique},
    {"TimesItalic", StandardFont::kTimesOblique},
    {"TimesNewRoman", StandardFont::kTimes},
    {"TimesNewRoman,Bold", StandardFont::kTimesBold},
    {"TimesNewRoman,BoldItalic", StandardFont::kTimesBoldOblique},
    {"TimesNewRoman,Italic", StandardFont::kTimesOblique},
    {"TimesNewRoman-Bold", StandardFont::kTimesBold},
    {"TimesNewRoman-BoldItalic", StandardFont::kTimesBoldOblique},
    {"TimesNewRoman-Italic", StandardFont::kTimesOblique},
    {"TimesNewRomanBold", StandardFont::kTimesBold},
    {"TimesNewRomanBoldItalic", StandardFont::kTimesBoldOblique},
    {"TimesNewRomanItalic", StandardFont::kTimesOblique},
    {"TimesNewRomanPS", StandardFont::kTimes},
    {"TimesNewRomanPS-Bold", StandardFont::kTimesBold},
    {"TimesNewRomanPS-BoldItalic", StandardFont::kTimesBoldOblique},
    {"TimesNewRomanPS-BoldItalicMT", StandardFont::kTimesBoldOblique},
    {"TimesNewRomanPS-BoldMT", StandardFont::kTimesBold},
    {"TimesNewRomanPS-Italic", StandardFont::kTimesOblique},
    {"TimesNewRomanPS-ItalicMT", StandardFont::kTimesOblique},
    {"TimesNewRomanPSMT", StandardFont::kTimes},
    {"TimesNewRomanPSMT,Bold", StandardFont::kTimesBold},
    {"TimesNewRomanPSMT,BoldItalic", StandardFont::kTimesBoldOblique},
    {"TimesNewRomanPSMT,Italic", StandardFont::kTimesOblique},
    {"ZapfDingbats", StandardFont::kDingbats},
};

constexpr std::array<pdfium::span<const uint8_t>, FX_kNumStandardFonts>
    kFoxitFonts = {{
        kFoxitFixedFontData,
        kFoxitFixedBoldFontData,
        kFoxitFixedBoldItalicFontData,
        kFoxitFixedItalicFontData,
        kFoxitSansFontData,
        kFoxitSansBoldFontData,
        kFoxitSansBoldItalicFontData,
        kFoxitSansItalicFontData,
        kFoxitSerifFontData,
        kFoxitSerifBoldFontData,
        kFoxitSerifBoldItalicFontData,
        kFoxitSerifItalicFontData,
        kFoxitSymbolFontData,
        kFoxitDingbatsFontData,
    }};

constexpr pdfium::span<const uint8_t> kGenericSansFont = kFoxitSansMMFontData;
constexpr pdfium::span<const uint8_t> kGenericSerifFont = kFoxitSerifMMFontData;

}  // namespace

std::optional<StandardFont> FX_GetStandardFontName(ByteString* name) {
  const auto* end = std::end(kAltFontNames);
  const auto* found =
      std::lower_bound(std::begin(kAltFontNames), end, name->c_str(),
                       [](const AltFontName& element, const char* name) {
                         return FXSYS_stricmp(element.name_, name) < 0;
                       });
  if (found == end || FXSYS_stricmp(found->name_, name->c_str())) {
    return std::nullopt;
  }

  *name = kBase14FontNames[static_cast<size_t>(found->index_)];
  return found->index_;
}

bool FX_IsStandardFontName(const ByteString& name) {
  return pdfium::Contains(kBase14FontNames, name);
}

bool FX_IsSymbolicFont(StandardFont font) {
  return font == StandardFont::kSymbol || font == StandardFont::kDingbats;
}

bool FX_IsFixedFont(StandardFont font) {
  return font == StandardFont::kCourier || font == StandardFont::kCourierBold ||
         font == StandardFont::kCourierBoldOblique ||
         font == StandardFont::kCourierOblique;
}

pdfium::span<const uint8_t> FX_GetStandardFont(StandardFont font) {
  return kFoxitFonts[static_cast<size_t>(font)];
}

pdfium::span<const uint8_t> FX_GetGenericSansFont() {
  return kGenericSansFont;
}

pdfium::span<const uint8_t> FX_GetGenericSerifFont() {
  return kGenericSerifFont;
}

std::optional<StandardFont> FX_GetStandardFontIndex(const ByteString& name) {
  auto it = std::find(kBase14FontNames.begin(), kBase14FontNames.end(), name);
  if (it == kBase14FontNames.end()) {
    return std::nullopt;
  }
  return static_cast<StandardFont>(std::distance(kBase14FontNames.begin(), it));
}

ByteString FX_GetCanonicalFontName(StandardFont font) {
  return kBase14FontNames[static_cast<size_t>(font)];
}
