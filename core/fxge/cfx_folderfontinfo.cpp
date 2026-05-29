// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/cfx_folderfontinfo.h"

#include <array>
#include <iterator>
#include <limits>
#include <utility>

#include "build/build_config.h"
#include "core/fxcrt/byteorder.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/containers/contains.h"
#include "core/fxcrt/fixed_size_data_vector.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/fx_folder.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/mapped_data_bytes.h"
#include "core/fxge/cfx_fontmapper.h"
#include "core/fxge/fx_font.h"

namespace {

struct FontSubst {
  const char* name_;
  const char* subst_name_;
};

constexpr auto kBase14Substs = std::to_array<const FontSubst>({
    {"Courier", "Courier New"},
    {"Courier-Bold", "Courier New Bold"},
    {"Courier-BoldOblique", "Courier New Bold Italic"},
    {"Courier-Oblique", "Courier New Italic"},
    {"Helvetica", "Arial"},
    {"Helvetica-Bold", "Arial Bold"},
    {"Helvetica-BoldOblique", "Arial Bold Italic"},
    {"Helvetica-Oblique", "Arial Italic"},
    {"Times-Roman", "Times New Roman"},
    {"Times-Bold", "Times New Roman Bold"},
    {"Times-BoldItalic", "Times New Roman Bold Italic"},
    {"Times-Italic", "Times New Roman Italic"},
});

// Used with std::unique_ptr to automatically call fclose().
struct FxFileCloser {
  inline void operator()(FILE* h) const {
    if (h) {
      fclose(h);
    }
  }
};

bool FindFamilyNameMatch(ByteStringView family_name,
                         const ByteString& installed_font_name) {
  std::optional<size_t> result = installed_font_name.Find(family_name, 0);
  if (!result.has_value()) {
    return false;
  }

  size_t next_index = result.value() + family_name.GetLength();
  // Rule out the case that |family_name| is a substring of
  // |installed_font_name| but their family names are actually different words.
  // For example: "Univers" and "Universal" are not a match because they have
  // different family names, but "Univers" and "Univers Bold" are a match.
  if (installed_font_name.IsValidIndex(next_index) &&
      FXSYS_IsLowerASCII(installed_font_name[next_index])) {
    return false;
  }

  return true;
}

}  // namespace

CFX_FolderFontInfo::CFX_FolderFontInfo() = default;

CFX_FolderFontInfo::~CFX_FolderFontInfo() = default;

void CFX_FolderFontInfo::AddPath(const ByteString& path) {
  path_list_.push_back(path);
}

void CFX_FolderFontInfo::EnumFontList(CFX_FontMapper* pMapper) {
  mapper_ = pMapper;
  for (const auto& path : path_list_) {
    ScanPath(path);
  }
}

void CFX_FolderFontInfo::ScanPath(const ByteString& path) {
  std::unique_ptr<FX_Folder> handle = FX_Folder::OpenFolder(path);
  if (!handle) {
    return;
  }

  ByteString filename;
  bool bFolder;
  while (handle->GetNextFile(&filename, &bFolder)) {
    if (bFolder) {
      if (filename == "." || filename == "..") {
        continue;
      }
    } else {
      ByteString ext = filename.Last(4);
      ext.MakeLower();
      if (ext != ".ttf" && ext != ".ttc" && ext != ".otf") {
        continue;
      }
    }

    ByteString fullpath = path;
#if BUILDFLAG(IS_WIN)
    fullpath += "\\";
#else
    fullpath += "/";
#endif

    fullpath += filename;
    bFolder ? ScanPath(fullpath) : ScanFile(fullpath);
  }
}

void CFX_FolderFontInfo::ScanFile(const ByteString& path) {
  std::unique_ptr<FILE, FxFileCloser> pFile(fopen(path.c_str(), "rb"));
  if (!pFile) {
    return;
  }

  fseek(pFile.get(), 0, SEEK_END);
  FX_FILESIZE filesize = ftell(pFile.get());
  fseek(pFile.get(), 0, SEEK_SET);

  uint8_t buffer[12];
  if (UNSAFE_BUFFERS(fread(buffer, 12, 1, pFile.get())) != 1) {
    return;
  }

  uint32_t magic = fxcrt::GetUInt32MSBFirst(pdfium::span(buffer).first<4u>());
  if (magic != SystemFontInfoIface::kTableTTCF) {
    ReportFace(path, pFile.get(), filesize, 0);
    return;
  }

  uint32_t nFaces =
      fxcrt::GetUInt32MSBFirst(pdfium::span(buffer).subspan<8u, 4u>());
  auto offsets = FixedSizeDataVector<uint8_t>::Uninit(nFaces * 4);
  if (UNSAFE_BUFFERS(
          fread(offsets.span().data(), nFaces * 4, 1, pFile.get())) != 1) {
    return;
  }

  for (uint32_t i = 0; i < nFaces; i++) {
    uint32_t offset =
        fxcrt::GetUInt32MSBFirst(offsets.span().subspan(i * 4).first<4u>());
    ReportFace(path, pFile.get(), filesize, offset);
  }
}

void CFX_FolderFontInfo::ReportFace(const ByteString& path,
                                    FILE* pFile,
                                    FX_FILESIZE filesize,
                                    uint32_t offset) {
  if (fseek(pFile, offset, SEEK_SET) < 0) {
    return;
  }

  uint8_t buffer[12];
  if (UNSAFE_BUFFERS(fread(buffer, 12, 1, pFile)) != 1) {
    return;
  }

  uint16_t nTables =
      fxcrt::GetUInt16MSBFirst(pdfium::span(buffer).subspan<4, 2>());
  auto table_dir_data = FixedSizeDataVector<uint8_t>::Uninit(nTables * 16);
  if (UNSAFE_BUFFERS(
          fread(table_dir_data.span().data(), nTables * 16, 1, pFile)) != 1) {
    return;
  }

  uint32_t name_tag = CFX_FontMapper::MakeTag('n', 'a', 'm', 'e');
  auto loc = FindFontTable(table_dir_data.span(), name_tag);
  if (!loc) {
    return;
  }

  auto names_data = FixedSizeDataVector<uint8_t>::Uninit(loc->size);
  if (fseek(pFile, loc->offset, SEEK_SET) < 0 ||
      UNSAFE_BUFFERS(fread(names_data.span().data(), loc->size, 1, pFile)) !=
          1) {
    return;
  }

  ByteString facename = GetNameFromTT(names_data.span(), 1);
  if (facename.IsEmpty()) {
    return;
  }

  ByteString style = GetNameFromTT(names_data.span(), 2);
  if (style != "Regular") {
    facename += " " + style;
  }

  if (pdfium::Contains(font_list_, facename)) {
    return;
  }

  ByteString tables = UNSAFE_BUFFERS(
      ByteString(table_dir_data.span().data(), table_dir_data.span().size()));
  auto pInfo = std::make_unique<FontFaceInfo>(
      path, facename, tables, offset, pdfium::checked_cast<uint32_t>(filesize));

  uint32_t os2_tag = CFX_FontMapper::MakeTag('O', 'S', '/', '2');
  loc = FindFontTable(table_dir_data.span(), os2_tag);
  if (loc && loc->size >= 86) {
    auto os2_data = FixedSizeDataVector<uint8_t>::Uninit(loc->size);
    if (fseek(pFile, loc->offset, SEEK_SET) >= 0 &&
        UNSAFE_BUFFERS(fread(os2_data.span().data(), loc->size, 1, pFile)) ==
            1) {
      pdfium::span<const uint8_t> p = os2_data.span().subspan(78u);
      uint32_t codepages = fxcrt::GetUInt32MSBFirst(p.first<4u>());
      if (codepages & (1U << 17)) {
        mapper_->AddInstalledFont(facename, FX_Charset::kShiftJIS);
        pInfo->charsets_ |= FontFaceInfo::CharsetFlag::kShiftJis;
      }
      if (codepages & (1U << 18)) {
        mapper_->AddInstalledFont(facename, FX_Charset::kChineseSimplified);
        pInfo->charsets_ |= FontFaceInfo::CharsetFlag::kGb;
      }
      if (codepages & (1U << 20)) {
        mapper_->AddInstalledFont(facename, FX_Charset::kChineseTraditional);
        pInfo->charsets_ |= FontFaceInfo::CharsetFlag::kBig5;
      }
      if ((codepages & (1U << 19)) || (codepages & (1U << 21))) {
        mapper_->AddInstalledFont(facename, FX_Charset::kHangul);
        pInfo->charsets_ |= FontFaceInfo::CharsetFlag::kKorean;
      }
      if (codepages & (1U << 31)) {
        mapper_->AddInstalledFont(facename, FX_Charset::kSymbol);
        pInfo->charsets_ |= FontFaceInfo::CharsetFlag::kSymbol;
      }
    }
  }

  mapper_->AddInstalledFont(facename, FX_Charset::kANSI);
  pInfo->charsets_ |= FontFaceInfo::CharsetFlag::kAnsi;
  pInfo->styles_ = 0;
  if (style.Contains("Bold")) {
    pInfo->styles_ |= pdfium::kFontStyleForceBold;
  }
  if (style.Contains("Italic") || style.Contains("Oblique")) {
    pInfo->styles_ |= pdfium::kFontStyleItalic;
  }
  if (facename.Contains("Serif")) {
    pInfo->styles_ |= pdfium::kFontStyleSerif;
  }

  font_list_[facename] = std::move(pInfo);
}

void* CFX_FolderFontInfo::GetSubstFont(const ByteString& face) {
  for (size_t iBaseFont = 0; iBaseFont < std::size(kBase14Substs);
       iBaseFont++) {
    if (face == kBase14Substs[iBaseFont].name_) {
      return GetFont(kBase14Substs[iBaseFont].subst_name_);
    }
  }
  return nullptr;
}

CFX_FolderFontInfo::FontFaceInfo::CharsetFlag
CFX_FolderFontInfo::FontFaceInfo::GetCharset(FX_Charset charset) {
  switch (charset) {
    case FX_Charset::kShiftJIS:
      return CharsetFlag::kShiftJis;
    case FX_Charset::kChineseSimplified:
      return CharsetFlag::kGb;
    case FX_Charset::kChineseTraditional:
      return CharsetFlag::kBig5;
    case FX_Charset::kHangul:
      return CharsetFlag::kKorean;
    case FX_Charset::kSymbol:
      return CharsetFlag::kSymbol;
    case FX_Charset::kANSI:
      return CharsetFlag::kAnsi;
    default:
      return CharsetFlag::kNone;
  }
}

void* CFX_FolderFontInfo::FindFont(int weight,
                                   bool bItalic,
                                   FX_Charset charset,
                                   int pitch_family,
                                   const ByteString& family,
                                   bool bMatchName) {
  FontFaceInfo* pFind = nullptr;
  FontFaceInfo::CharsetFlag charset_flag = FontFaceInfo::GetCharset(charset);

  int32_t iBestSimilar = 0;
  if (bMatchName) {
    // Try a direct lookup for either a perfect score or to determine a
    // baseline similarity score.
    auto direct_it = font_list_.find(family);
    if (direct_it != font_list_.end()) {
      FontFaceInfo* font = direct_it->second.get();
      if (font->IsEligibleForFindFont(charset_flag, charset)) {
        iBestSimilar =
            font->SimilarityScore(weight, bItalic, pitch_family, bMatchName);
        if (iBestSimilar == FontFaceInfo::kSimilarityScoreMax) {
          return font;
        }
        pFind = font;
      }
    }
  }
  // Try and find a better match. Since FindFamilyNameMatch() is expensive,
  // avoid calling it unless there might be a better match.
  ByteStringView bsFamily = family.AsStringView();
  for (const auto& it : font_list_) {
    const ByteString& bsName = it.first;
    FontFaceInfo* font = it.second.get();
    if (!font->IsEligibleForFindFont(charset_flag, charset)) {
      continue;
    }
    int32_t iSimilarValue = font->SimilarityScore(
        weight, bItalic, pitch_family,
        bMatchName && bsFamily.GetLength() == bsName.GetLength());
    if (iSimilarValue > iBestSimilar) {
      if (bMatchName && !FindFamilyNameMatch(bsFamily, bsName)) {
        continue;
      }
      iBestSimilar = iSimilarValue;
      pFind = font;
    }
  }

  if (pFind) {
    return pFind;
  }

  if (charset == FX_Charset::kANSI && FontFamilyIsFixedPitch(pitch_family)) {
    auto* courier_new = GetFont("Courier New");
    if (courier_new) {
      return courier_new;
    }
  }

  return nullptr;
}

void* CFX_FolderFontInfo::MapFont(int weight,
                                  bool bItalic,
                                  FX_Charset charset,
                                  int pitch_family,
                                  const ByteString& face) {
  return nullptr;
}

void* CFX_FolderFontInfo::GetFont(const ByteString& face) {
  auto it = font_list_.find(face);
  return it != font_list_.end() ? it->second.get() : nullptr;
}

size_t CFX_FolderFontInfo::GetFontData(void* hFont,
                                       uint32_t table,
                                       pdfium::span<uint8_t> buffer) {
  if (!hFont) {
    return 0;
  }

  const FontFaceInfo* font = static_cast<FontFaceInfo*>(hFont);

  if (table == SystemFontInfoIface::kTableNone) {
    if (font->font_offset_) {
      return 0;
    }
    if (buffer.size() < font->file_size_) {
      return font->file_size_;
    }
    std::unique_ptr<FILE, FxFileCloser> pFile(
        fopen(font->file_path_.c_str(), "rb"));
    if (!pFile) {
      return 0;
    }
    if (UNSAFE_BUFFERS(
            fread(buffer.data(), font->file_size_, 1, pFile.get())) != 1) {
      return 0;
    }
    return font->file_size_;
  }

  if (table == SystemFontInfoIface::kTableTTCF) {
    if (!font->font_offset_) {
      return 0;
    }
    if (buffer.size() < font->file_size_) {
      return font->file_size_;
    }
    std::unique_ptr<FILE, FxFileCloser> pFile(
        fopen(font->file_path_.c_str(), "rb"));
    if (!pFile) {
      return 0;
    }
    if (UNSAFE_BUFFERS(
            fread(buffer.data(), font->file_size_, 1, pFile.get())) != 1) {
      return 0;
    }
    return font->file_size_;
  }

  std::unique_ptr<FILE, FxFileCloser> pFile(
      fopen(font->file_path_.c_str(), "rb"));
  if (!pFile) {
    return 0;
  }

  auto loc = FindFontTable(font->font_tables_.unsigned_span(), table);
  if (!loc) {
    return 0;
  }

  if (buffer.size() < loc->size) {
    return loc->size;
  }

  if (fseek(pFile.get(), loc->offset, SEEK_SET) < 0 ||
      UNSAFE_BUFFERS(fread(buffer.data(), loc->size, 1, pFile.get())) != 1) {
    return 0;
  }
  return loc->size;
}

void CFX_FolderFontInfo::DeleteFont(void* hFont) {}

bool CFX_FolderFontInfo::GetFaceName(void* hFont, ByteString* name) {
  if (!hFont) {
    return false;
  }
  *name = static_cast<FontFaceInfo*>(hFont)->face_name_;
  return true;
}

bool CFX_FolderFontInfo::GetFontCharset(void* hFont, FX_Charset* charset) {
  return false;
}

CFX_FolderFontInfo::FontFaceInfo::FontFaceInfo(ByteString filePath,
                                               ByteString faceName,
                                               ByteString fontTables,
                                               uint32_t fontOffset,
                                               uint32_t fileSize)
    : file_path_(filePath),
      face_name_(faceName),
      font_tables_(fontTables),
      font_offset_(fontOffset),
      file_size_(fileSize) {}

bool CFX_FolderFontInfo::FontFaceInfo::IsEligibleForFindFont(
    CharsetFlag flag,
    FX_Charset charset) const {
  return (charsets_ & flag) || charset == FX_Charset::kDefault;
}

int32_t CFX_FolderFontInfo::FontFaceInfo::SimilarityScore(
    int weight,
    bool italic,
    int pitch_family,
    bool exact_match_bonus) const {
  int32_t score = 0;
  if (FontStyleIsForceBold(styles_) == (weight > 400)) {
    score += 16;
  }
  if (FontStyleIsItalic(styles_) == italic) {
    score += 16;
  }
  if (FontStyleIsSerif(styles_) == FontFamilyIsRoman(pitch_family)) {
    score += 16;
  }
  if (FontStyleIsScript(styles_) == FontFamilyIsScript(pitch_family)) {
    score += 8;
  }
  if (FontStyleIsFixedPitch(styles_) == FontFamilyIsFixedPitch(pitch_family)) {
    score += 8;
  }
  if (exact_match_bonus) {
    score += 4;
  }
  DCHECK_LE(score, kSimilarityScoreMax);
  return score;
}
