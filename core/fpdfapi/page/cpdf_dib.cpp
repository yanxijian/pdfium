// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_dib.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "core/fpdfapi/page/cpdf_colorspace.h"
#include "core/fpdfapi/page/cpdf_docpagedata.h"
#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fpdfapi/parser/fpdf_parser_utility.h"
#include "core/fxcodec/basic/basicmodule.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/jbig2/jbig2_decoder.h"
#include "core/fxcodec/jpeg/jpegmodule.h"
#include "core/fxcodec/jpx/cjpx_decoder.h"
#include "core/fxcodec/scanlinedecoder.h"
#include "core/fxcrt/cfx_fixedbufgrow.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "third_party/base/check.h"
#include "third_party/base/check_op.h"
#include "third_party/base/cxx17_backports.h"

namespace {

bool IsValidDimension(int value) {
  constexpr int kMaxImageDimension = 0x01FFFF;
  return value > 0 && value <= kMaxImageDimension;
}

unsigned int GetBits8(const uint8_t* pData, uint64_t bitpos, size_t nbits) {
  DCHECK(nbits == 1 || nbits == 2 || nbits == 4 || nbits == 8 || nbits == 16);
  DCHECK_EQ((bitpos & (nbits - 1)), 0);
  unsigned int byte = pData[bitpos / 8];
  if (nbits == 8)
    return byte;

  if (nbits == 16)
    return byte * 256 + pData[bitpos / 8 + 1];

  return (byte >> (8 - nbits - (bitpos % 8))) & ((1 << nbits) - 1);
}

bool GetBitValue(const uint8_t* pSrc, uint32_t pos) {
  return pSrc[pos / 8] & (1 << (7 - pos % 8));
}

// Just to sanity check and filter out obvious bad values.
bool IsMaybeValidBitsPerComponent(int bpc) {
  return bpc >= 0 && bpc <= 16;
}

bool IsAllowedBitsPerComponent(int bpc) {
  return bpc == 1 || bpc == 2 || bpc == 4 || bpc == 8 || bpc == 16;
}

bool IsColorIndexOutOfBounds(uint8_t index, const DIB_COMP_DATA& comp_datum) {
  return index < comp_datum.m_ColorKeyMin || index > comp_datum.m_ColorKeyMax;
}

bool AreColorIndicesOutOfBounds(const uint8_t* indices,
                                const DIB_COMP_DATA* comp_data,
                                size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (IsColorIndexOutOfBounds(indices[i], comp_data[i]))
      return true;
  }
  return false;
}

int CalculateBitsPerPixel(uint32_t bpc, uint32_t comps) {
  // TODO(thestig): Can |bpp| be 0 here? Add an DCHECK() or handle it?
  uint32_t bpp = bpc * comps;
  if (bpp == 1)
    return 1;
  if (bpp <= 8)
    return 8;
  return 24;
}

CJPX_Decoder::ColorSpaceOption ColorSpaceOptionFromColorSpace(
    CPDF_ColorSpace* pCS) {
  if (!pCS)
    return CJPX_Decoder::kNoColorSpace;
  if (pCS->GetFamily() == CPDF_ColorSpace::Family::kIndexed)
    return CJPX_Decoder::kIndexedColorSpace;
  return CJPX_Decoder::kNormalColorSpace;
}

enum class JpxDecodeAction {
  kFail,
  kDoNothing,
  kUseRgb,
  kUseCmyk,
  kConvertArgbToRgb,
};

JpxDecodeAction GetJpxDecodeAction(const CJPX_Decoder::JpxImageInfo& jpx_info,
                                   const CPDF_ColorSpace* pdf_colorspace) {
  if (pdf_colorspace) {
    // Make sure the JPX image and the PDF colorspace agree on the number of
    // components. In case of a mismatch, try to handle the discrepancy.
    if (jpx_info.components != pdf_colorspace->CountComponents()) {
      // Many PDFs generated by iOS meets this condition. See
      // https://crbug.com/1012369 for example.
      if (pdf_colorspace->CountComponents() == 3 && jpx_info.components == 4 &&
          jpx_info.colorspace == OPJ_CLRSPC_SRGB) {
        return JpxDecodeAction::kConvertArgbToRgb;
      }

      return JpxDecodeAction::kFail;
    }

    if (pdf_colorspace ==
        CPDF_ColorSpace::GetStockCS(CPDF_ColorSpace::Family::kDeviceRGB)) {
      return JpxDecodeAction::kUseRgb;
    }
    return JpxDecodeAction::kDoNothing;
  }

  // Cases where the PDF did not provide a colorspace.
  // Choose how to decode based on the number of components in the JPX image.
  switch (jpx_info.components) {
    case 3:
      return JpxDecodeAction::kUseRgb;

    case 4:
      return JpxDecodeAction::kUseCmyk;

    default:
      return JpxDecodeAction::kDoNothing;
  }
}

}  // namespace

CPDF_DIB::CPDF_DIB() = default;

CPDF_DIB::~CPDF_DIB() = default;

CPDF_DIB::JpxSMaskInlineData::JpxSMaskInlineData() = default;

CPDF_DIB::JpxSMaskInlineData::~JpxSMaskInlineData() = default;

bool CPDF_DIB::Load(CPDF_Document* pDoc, const CPDF_Stream* pStream) {
  if (!pStream)
    return false;

  m_pDocument = pDoc;
  m_pDict.Reset(pStream->GetDict());
  if (!m_pDict)
    return false;

  m_pStream.Reset(pStream);
  m_Width = m_pDict->GetIntegerFor("Width");
  m_Height = m_pDict->GetIntegerFor("Height");
  if (!IsValidDimension(m_Width) || !IsValidDimension(m_Height))
    return false;

  m_GroupFamily = CPDF_ColorSpace::Family::kUnknown;
  m_bLoadMask = false;
  if (!LoadColorInfo(nullptr, nullptr))
    return false;

  if (m_bDoBpcCheck && (m_bpc == 0 || m_nComponents == 0))
    return false;

  const Optional<uint32_t> maybe_size =
      fxcodec::CalculatePitch8(m_bpc, m_nComponents, m_Width);
  if (!maybe_size.has_value())
    return false;

  FX_SAFE_UINT32 src_size = maybe_size.value();
  src_size *= m_Height;
  if (!src_size.IsValid())
    return false;

  m_pStreamAcc = pdfium::MakeRetain<CPDF_StreamAcc>(pStream);
  m_pStreamAcc->LoadAllDataImageAcc(src_size.ValueOrDie());
  if (m_pStreamAcc->GetSize() == 0 || !m_pStreamAcc->GetData())
    return false;

  if (CreateDecoder() == LoadState::kFail)
    return false;

  if (m_bImageMask)
    SetMaskProperties();
  else
    m_Format = MakeRGBFormat(CalculateBitsPerPixel(m_bpc, m_nComponents));

  Optional<uint32_t> pitch =
      fxcodec::CalculatePitch32(GetBppFromFormat(m_Format), m_Width);
  if (!pitch.has_value())
    return false;

  m_pLineBuf.reset(FX_Alloc(uint8_t, pitch.value()));
  LoadPalette();
  if (m_bColorKey) {
    m_Format = FXDIB_Format::kArgb;
    pitch = fxcodec::CalculatePitch32(GetBppFromFormat(m_Format), m_Width);
    if (!pitch.has_value())
      return false;

    m_pMaskedLine.reset(FX_Alloc(uint8_t, pitch.value()));
  }
  m_Pitch = pitch.value();
  return true;
}

bool CPDF_DIB::ContinueToLoadMask() {
  if (m_bImageMask) {
    SetMaskProperties();
  } else {
    if (!m_bpc || !m_nComponents)
      return false;

    m_Format = MakeRGBFormat(CalculateBitsPerPixel(m_bpc, m_nComponents));
  }

  Optional<uint32_t> pitch =
      fxcodec::CalculatePitch32(GetBppFromFormat(m_Format), m_Width);
  if (!pitch.has_value())
    return false;

  m_pLineBuf.reset(FX_Alloc(uint8_t, pitch.value()));
  if (m_pColorSpace && m_bStdCS) {
    m_pColorSpace->EnableStdConversion(true);
  }
  LoadPalette();
  if (m_bColorKey) {
    m_Format = FXDIB_Format::kArgb;
    pitch = fxcodec::CalculatePitch32(GetBppFromFormat(m_Format), m_Width);
    if (!pitch.has_value())
      return false;
    m_pMaskedLine.reset(FX_Alloc(uint8_t, pitch.value()));
  }
  m_Pitch = pitch.value();
  return true;
}

CPDF_DIB::LoadState CPDF_DIB::StartLoadDIBBase(
    CPDF_Document* pDoc,
    const CPDF_Stream* pStream,
    bool bHasMask,
    const CPDF_Dictionary* pFormResources,
    const CPDF_Dictionary* pPageResources,
    bool bStdCS,
    CPDF_ColorSpace::Family GroupFamily,
    bool bLoadMask) {
  if (!pStream)
    return LoadState::kFail;

  m_pDocument = pDoc;
  m_pDict.Reset(pStream->GetDict());
  m_pStream.Reset(pStream);
  m_bStdCS = bStdCS;
  m_bHasMask = bHasMask;
  m_Width = m_pDict->GetIntegerFor("Width");
  m_Height = m_pDict->GetIntegerFor("Height");
  if (!IsValidDimension(m_Width) || !IsValidDimension(m_Height))
    return LoadState::kFail;

  m_GroupFamily = GroupFamily;
  m_bLoadMask = bLoadMask;
  if (!LoadColorInfo(m_pStream->IsInline() ? pFormResources : nullptr,
                     pPageResources)) {
    return LoadState::kFail;
  }
  if (m_bDoBpcCheck && (m_bpc == 0 || m_nComponents == 0))
    return LoadState::kFail;

  const Optional<uint32_t> maybe_size =
      fxcodec::CalculatePitch8(m_bpc, m_nComponents, m_Width);
  if (!maybe_size.has_value())
    return LoadState::kFail;

  FX_SAFE_UINT32 src_size = maybe_size.value();
  src_size *= m_Height;
  if (!src_size.IsValid())
    return LoadState::kFail;

  m_pStreamAcc = pdfium::MakeRetain<CPDF_StreamAcc>(pStream);
  m_pStreamAcc->LoadAllDataImageAcc(src_size.ValueOrDie());
  if (m_pStreamAcc->GetSize() == 0 || !m_pStreamAcc->GetData())
    return LoadState::kFail;

  LoadState iCreatedDecoder = CreateDecoder();
  if (iCreatedDecoder == LoadState::kFail)
    return LoadState::kFail;

  if (!ContinueToLoadMask())
    return LoadState::kFail;

  LoadState iLoadedMask = m_bHasMask ? StartLoadMask() : LoadState::kSuccess;
  if (iCreatedDecoder == LoadState::kContinue ||
      iLoadedMask == LoadState::kContinue) {
    return LoadState::kContinue;
  }

  DCHECK_EQ(iCreatedDecoder, LoadState::kSuccess);
  DCHECK_EQ(iLoadedMask, LoadState::kSuccess);
  if (m_pColorSpace && m_bStdCS)
    m_pColorSpace->EnableStdConversion(false);
  return LoadState::kSuccess;
}

CPDF_DIB::LoadState CPDF_DIB::ContinueLoadDIBBase(PauseIndicatorIface* pPause) {
  if (m_Status == LoadState::kContinue)
    return ContinueLoadMaskDIB(pPause);

  ByteString decoder = m_pStreamAcc->GetImageDecoder();
  if (decoder == "JPXDecode")
    return LoadState::kFail;

  if (decoder != "JBIG2Decode")
    return LoadState::kSuccess;

  if (m_Status == LoadState::kFail)
    return LoadState::kFail;

  FXCODEC_STATUS iDecodeStatus;
  if (!m_pJbig2Context) {
    m_pJbig2Context = std::make_unique<Jbig2Context>();
    if (m_pStreamAcc->GetImageParam()) {
      const CPDF_Stream* pGlobals =
          m_pStreamAcc->GetImageParam()->GetStreamFor("JBIG2Globals");
      if (pGlobals) {
        m_pGlobalAcc = pdfium::MakeRetain<CPDF_StreamAcc>(pGlobals);
        m_pGlobalAcc->LoadAllDataFiltered();
      }
    }
    uint32_t nSrcObjNum = 0;
    pdfium::span<const uint8_t> pSrcSpan;
    if (m_pStreamAcc) {
      pSrcSpan = m_pStreamAcc->GetSpan();
      if (m_pStreamAcc->GetStream())
        nSrcObjNum = m_pStreamAcc->GetStream()->GetObjNum();
    }
    uint32_t nGlobalObjNum = 0;
    pdfium::span<const uint8_t> pGlobalSpan;
    if (m_pGlobalAcc) {
      pGlobalSpan = m_pGlobalAcc->GetSpan();
      if (m_pGlobalAcc->GetStream())
        nGlobalObjNum = m_pGlobalAcc->GetStream()->GetObjNum();
    }
    iDecodeStatus = Jbig2Decoder::StartDecode(
        m_pJbig2Context.get(), m_pDocument->GetOrCreateCodecContext(), m_Width,
        m_Height, pSrcSpan, nSrcObjNum, pGlobalSpan, nGlobalObjNum,
        m_pCachedBitmap->GetBuffer(), m_pCachedBitmap->GetPitch(), pPause);
  } else {
    iDecodeStatus = Jbig2Decoder::ContinueDecode(m_pJbig2Context.get(), pPause);
  }

  if (iDecodeStatus == FXCODEC_STATUS::kError) {
    m_pJbig2Context.reset();
    m_pCachedBitmap.Reset();
    m_pGlobalAcc.Reset();
    return LoadState::kFail;
  }
  if (iDecodeStatus == FXCODEC_STATUS::kDecodeToBeContinued)
    return LoadState::kContinue;

  LoadState iContinueStatus = LoadState::kSuccess;
  if (m_bHasMask) {
    if (ContinueLoadMaskDIB(pPause) == LoadState::kContinue) {
      iContinueStatus = LoadState::kContinue;
      m_Status = LoadState::kContinue;
    }
  }
  if (iContinueStatus == LoadState::kContinue)
    return LoadState::kContinue;

  if (m_pColorSpace && m_bStdCS)
    m_pColorSpace->EnableStdConversion(false);
  return iContinueStatus;
}

bool CPDF_DIB::LoadColorInfo(const CPDF_Dictionary* pFormResources,
                             const CPDF_Dictionary* pPageResources) {
  Optional<DecoderArray> decoder_array = GetDecoderArray(m_pDict.Get());
  if (!decoder_array.has_value())
    return false;

  m_bpc_orig = m_pDict->GetIntegerFor("BitsPerComponent");
  if (!IsMaybeValidBitsPerComponent(m_bpc_orig))
    return false;

  if (m_pDict->GetIntegerFor("ImageMask"))
    m_bImageMask = true;

  if (m_bImageMask || !m_pDict->KeyExist("ColorSpace")) {
    if (!m_bImageMask && !decoder_array.value().empty()) {
      const ByteString& filter = decoder_array.value().back().first;
      if (filter == "JPXDecode") {
        m_bDoBpcCheck = false;
        return true;
      }
    }
    m_bImageMask = true;
    m_bpc = m_nComponents = 1;
    const CPDF_Array* pDecode = m_pDict->GetArrayFor("Decode");
    m_bDefaultDecode = !pDecode || !pDecode->GetIntegerAt(0);
    return true;
  }

  const CPDF_Object* pCSObj = m_pDict->GetDirectObjectFor("ColorSpace");
  if (!pCSObj)
    return false;

  auto* pDocPageData = CPDF_DocPageData::FromDocument(m_pDocument.Get());
  if (pFormResources)
    m_pColorSpace = pDocPageData->GetColorSpace(pCSObj, pFormResources);
  if (!m_pColorSpace)
    m_pColorSpace = pDocPageData->GetColorSpace(pCSObj, pPageResources);
  if (!m_pColorSpace)
    return false;

  // If the checks above failed to find a colorspace, and the next line to set
  // |m_nComponents| does not get reached, then a decoder can try to set
  // |m_nComponents| based on the number of components in the image being
  // decoded.
  m_nComponents = m_pColorSpace->CountComponents();
  m_Family = m_pColorSpace->GetFamily();
  if (m_Family == CPDF_ColorSpace::Family::kICCBased && pCSObj->IsName()) {
    ByteString cs = pCSObj->GetString();
    if (cs == "DeviceGray")
      m_nComponents = 1;
    else if (cs == "DeviceRGB")
      m_nComponents = 3;
    else if (cs == "DeviceCMYK")
      m_nComponents = 4;
  }

  ByteString filter;
  if (!decoder_array.value().empty())
    filter = decoder_array.value().back().first;

  ValidateDictParam(filter);
  return GetDecodeAndMaskArray(&m_bDefaultDecode, &m_bColorKey);
}

bool CPDF_DIB::GetDecodeAndMaskArray(bool* bDefaultDecode, bool* bColorKey) {
  if (!m_pColorSpace)
    return false;

  m_CompData.resize(m_nComponents);
  int max_data = (1 << m_bpc) - 1;
  const CPDF_Array* pDecode = m_pDict->GetArrayFor("Decode");
  if (pDecode) {
    for (uint32_t i = 0; i < m_nComponents; i++) {
      m_CompData[i].m_DecodeMin = pDecode->GetNumberAt(i * 2);
      float max = pDecode->GetNumberAt(i * 2 + 1);
      m_CompData[i].m_DecodeStep = (max - m_CompData[i].m_DecodeMin) / max_data;
      float def_value;
      float def_min;
      float def_max;
      m_pColorSpace->GetDefaultValue(i, &def_value, &def_min, &def_max);
      if (m_Family == CPDF_ColorSpace::Family::kIndexed)
        def_max = max_data;
      if (def_min != m_CompData[i].m_DecodeMin || def_max != max)
        *bDefaultDecode = false;
    }
  } else {
    for (uint32_t i = 0; i < m_nComponents; i++) {
      float def_value;
      m_pColorSpace->GetDefaultValue(i, &def_value, &m_CompData[i].m_DecodeMin,
                                     &m_CompData[i].m_DecodeStep);
      if (m_Family == CPDF_ColorSpace::Family::kIndexed)
        m_CompData[i].m_DecodeStep = max_data;
      m_CompData[i].m_DecodeStep =
          (m_CompData[i].m_DecodeStep - m_CompData[i].m_DecodeMin) / max_data;
    }
  }
  if (m_pDict->KeyExist("SMask"))
    return true;

  const CPDF_Object* pMask = m_pDict->GetDirectObjectFor("Mask");
  if (!pMask)
    return true;

  if (const CPDF_Array* pArray = pMask->AsArray()) {
    if (pArray->size() >= m_nComponents * 2) {
      for (uint32_t i = 0; i < m_nComponents; i++) {
        int min_num = pArray->GetIntegerAt(i * 2);
        int max_num = pArray->GetIntegerAt(i * 2 + 1);
        m_CompData[i].m_ColorKeyMin = std::max(min_num, 0);
        m_CompData[i].m_ColorKeyMax = std::min(max_num, max_data);
      }
    }
    *bColorKey = true;
  }
  return true;
}

CPDF_DIB::LoadState CPDF_DIB::CreateDecoder() {
  ByteString decoder = m_pStreamAcc->GetImageDecoder();
  if (decoder.IsEmpty())
    return LoadState::kSuccess;

  if (m_bDoBpcCheck && m_bpc == 0)
    return LoadState::kFail;

  if (decoder == "JPXDecode") {
    m_pCachedBitmap = LoadJpxBitmap();
    return m_pCachedBitmap ? LoadState::kSuccess : LoadState::kFail;
  }

  if (decoder == "JBIG2Decode") {
    m_pCachedBitmap = pdfium::MakeRetain<CFX_DIBitmap>();
    if (!m_pCachedBitmap->Create(
            m_Width, m_Height,
            m_bImageMask ? FXDIB_Format::k1bppMask : FXDIB_Format::k1bppRgb)) {
      m_pCachedBitmap.Reset();
      return LoadState::kFail;
    }
    m_Status = LoadState::kSuccess;
    return LoadState::kContinue;
  }

  pdfium::span<const uint8_t> src_span = m_pStreamAcc->GetSpan();
  const CPDF_Dictionary* pParams = m_pStreamAcc->GetImageParam();
  if (decoder == "CCITTFaxDecode") {
    m_pDecoder = CreateFaxDecoder(src_span, m_Width, m_Height, pParams);
  } else if (decoder == "FlateDecode") {
    m_pDecoder = CreateFlateDecoder(src_span, m_Width, m_Height, m_nComponents,
                                    m_bpc, pParams);
  } else if (decoder == "RunLengthDecode") {
    m_pDecoder = BasicModule::CreateRunLengthDecoder(
        src_span, m_Width, m_Height, m_nComponents, m_bpc);
  } else if (decoder == "DCTDecode") {
    if (!CreateDCTDecoder(src_span, pParams))
      return LoadState::kFail;
  }
  if (!m_pDecoder)
    return LoadState::kFail;

  const Optional<uint32_t> requested_pitch =
      fxcodec::CalculatePitch8(m_bpc, m_nComponents, m_Width);
  if (!requested_pitch.has_value())
    return LoadState::kFail;
  const Optional<uint32_t> provided_pitch = fxcodec::CalculatePitch8(
      m_pDecoder->GetBPC(), m_pDecoder->CountComps(), m_pDecoder->GetWidth());
  if (!provided_pitch.has_value())
    return LoadState::kFail;
  if (provided_pitch.value() < requested_pitch.value())
    return LoadState::kFail;
  return LoadState::kSuccess;
}

bool CPDF_DIB::CreateDCTDecoder(pdfium::span<const uint8_t> src_span,
                                const CPDF_Dictionary* pParams) {
  m_pDecoder = JpegModule::CreateDecoder(
      src_span, m_Width, m_Height, m_nComponents,
      !pParams || pParams->GetIntegerFor("ColorTransform", 1));
  if (m_pDecoder)
    return true;

  Optional<JpegModule::ImageInfo> info_opt = JpegModule::LoadInfo(src_span);
  if (!info_opt.has_value())
    return false;

  const JpegModule::ImageInfo& info = info_opt.value();
  m_Width = info.width;
  m_Height = info.height;

  if (!CPDF_Image::IsValidJpegComponent(info.num_components) ||
      !CPDF_Image::IsValidJpegBitsPerComponent(info.bits_per_components)) {
    return false;
  }

  if (m_nComponents == static_cast<uint32_t>(info.num_components)) {
    m_bpc = info.bits_per_components;
    m_pDecoder = JpegModule::CreateDecoder(src_span, m_Width, m_Height,
                                           m_nComponents, info.color_transform);
    return true;
  }

  m_nComponents = static_cast<uint32_t>(info.num_components);
  m_CompData.clear();
  if (m_pColorSpace) {
    uint32_t colorspace_comps = m_pColorSpace->CountComponents();
    switch (m_Family) {
      case CPDF_ColorSpace::Family::kDeviceGray:
      case CPDF_ColorSpace::Family::kDeviceRGB:
      case CPDF_ColorSpace::Family::kDeviceCMYK: {
        uint32_t dwMinComps = CPDF_ColorSpace::ComponentsForFamily(m_Family);
        if (colorspace_comps < dwMinComps || m_nComponents < dwMinComps)
          return false;
        break;
      }
      case CPDF_ColorSpace::Family::kLab: {
        if (m_nComponents != 3 || colorspace_comps < 3)
          return false;
        break;
      }
      case CPDF_ColorSpace::Family::kICCBased: {
        if (!CPDF_ColorSpace::IsValidIccComponents(colorspace_comps) ||
            !CPDF_ColorSpace::IsValidIccComponents(m_nComponents) ||
            colorspace_comps < m_nComponents) {
          return false;
        }
        break;
      }
      default: {
        if (colorspace_comps != m_nComponents)
          return false;
        break;
      }
    }
  } else {
    if (m_Family == CPDF_ColorSpace::Family::kLab && m_nComponents != 3)
      return false;
  }
  if (!GetDecodeAndMaskArray(&m_bDefaultDecode, &m_bColorKey))
    return false;

  m_bpc = info.bits_per_components;
  m_pDecoder = JpegModule::CreateDecoder(src_span, m_Width, m_Height,
                                         m_nComponents, info.color_transform);
  return true;
}

RetainPtr<CFX_DIBitmap> CPDF_DIB::LoadJpxBitmap() {
  std::unique_ptr<CJPX_Decoder> decoder =
      CJPX_Decoder::Create(m_pStreamAcc->GetSpan(),
                           ColorSpaceOptionFromColorSpace(m_pColorSpace.Get()));
  if (!decoder)
    return nullptr;

  if (!decoder->StartDecode())
    return nullptr;

  CJPX_Decoder::JpxImageInfo image_info = decoder->GetInfo();
  if (static_cast<int>(image_info.width) < m_Width ||
      static_cast<int>(image_info.height) < m_Height) {
    return nullptr;
  }

  RetainPtr<CPDF_ColorSpace> original_colorspace = m_pColorSpace;
  bool swap_rgb = false;
  bool convert_argb_to_rgb = false;
  switch (GetJpxDecodeAction(image_info, m_pColorSpace.Get())) {
    case JpxDecodeAction::kFail:
      return nullptr;

    case JpxDecodeAction::kDoNothing:
      break;

    case JpxDecodeAction::kUseRgb:
      DCHECK(image_info.components >= 3);
      swap_rgb = true;
      m_pColorSpace = nullptr;
      break;

    case JpxDecodeAction::kUseCmyk:
      m_pColorSpace =
          CPDF_ColorSpace::GetStockCS(CPDF_ColorSpace::Family::kDeviceCMYK);
      break;

    case JpxDecodeAction::kConvertArgbToRgb:
      swap_rgb = true;
      convert_argb_to_rgb = true;
      m_pColorSpace.Reset();
  }

  // If |original_colorspace| exists, then LoadColorInfo() already set
  // |m_nComponents|.
  if (original_colorspace) {
    DCHECK_NE(0, m_nComponents);
  } else {
    DCHECK_EQ(0, m_nComponents);
    m_nComponents = image_info.components;
  }

  FXDIB_Format format;
  if (image_info.components == 1) {
    format = FXDIB_Format::k8bppRgb;
  } else if (image_info.components <= 3) {
    format = FXDIB_Format::kRgb;
  } else if (image_info.components == 4) {
    format = FXDIB_Format::kRgb32;
  } else {
    image_info.width = (image_info.width * image_info.components + 2) / 3;
    format = FXDIB_Format::kRgb;
  }

  auto result_bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  if (!result_bitmap->Create(image_info.width, image_info.height, format))
    return nullptr;

  result_bitmap->Clear(0xFFFFFFFF);
  if (!decoder->Decode(result_bitmap->GetBuffer(), result_bitmap->GetPitch(),
                       swap_rgb)) {
    return nullptr;
  }

  if (convert_argb_to_rgb) {
    DCHECK_EQ(3, m_nComponents);
    auto rgb_bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
    if (!rgb_bitmap->Create(image_info.width, image_info.height,
                            FXDIB_Format::kRgb)) {
      return nullptr;
    }
    if (m_pDict->GetIntegerFor("SMaskInData") == 1) {
      // TODO(thestig): Acrobat does not support "/SMaskInData 1" combined with
      // filters. Check for that and fail early.
      DCHECK(m_JpxInlineData.data.empty());
      m_JpxInlineData.width = image_info.width;
      m_JpxInlineData.height = image_info.height;
      m_JpxInlineData.data.reserve(image_info.width * image_info.height);
      for (uint32_t row = 0; row < image_info.height; ++row) {
        const uint8_t* src = result_bitmap->GetScanline(row);
        uint8_t* dest = rgb_bitmap->GetWritableScanline(row);
        for (uint32_t col = 0; col < image_info.width; ++col) {
          uint8_t a = src[3];
          m_JpxInlineData.data.push_back(a);
          uint8_t na = 255 - a;
          uint8_t b = (src[0] * a + 255 * na) / 255;
          uint8_t g = (src[1] * a + 255 * na) / 255;
          uint8_t r = (src[2] * a + 255 * na) / 255;
          dest[0] = b;
          dest[1] = g;
          dest[2] = r;
          src += 4;
          dest += 3;
        }
      }
    } else {
      // TODO(thestig): Is there existing code that does this already?
      for (uint32_t row = 0; row < image_info.height; ++row) {
        const uint8_t* src = result_bitmap->GetScanline(row);
        uint8_t* dest = rgb_bitmap->GetWritableScanline(row);
        for (uint32_t col = 0; col < image_info.width; ++col) {
          memcpy(dest, src, 3);
          src += 4;
          dest += 3;
        }
      }
    }
    result_bitmap = std::move(rgb_bitmap);
  } else if (m_pColorSpace &&
             m_pColorSpace->GetFamily() == CPDF_ColorSpace::Family::kIndexed &&
             m_bpc < 8) {
    int scale = 8 - m_bpc;
    for (uint32_t row = 0; row < image_info.height; ++row) {
      uint8_t* scanline = result_bitmap->GetWritableScanline(row);
      for (uint32_t col = 0; col < image_info.width; ++col) {
        *scanline = (*scanline) >> scale;
        ++scanline;
      }
    }
  }
  m_bpc = 8;
  return result_bitmap;
}

CPDF_DIB::LoadState CPDF_DIB::StartLoadMask() {
  m_MatteColor = 0XFFFFFFFF;

  if (!m_JpxInlineData.data.empty()) {
    auto dict = pdfium::MakeRetain<CPDF_Dictionary>();
    dict->SetNewFor<CPDF_Name>("Type", "XObject");
    dict->SetNewFor<CPDF_Name>("Subtype", "Image");
    dict->SetNewFor<CPDF_Name>("ColorSpace", "DeviceGray");
    dict->SetNewFor<CPDF_Number>("Width", m_JpxInlineData.width);
    dict->SetNewFor<CPDF_Number>("Height", m_JpxInlineData.height);
    dict->SetNewFor<CPDF_Number>("BitsPerComponent", 8);

    auto mask_in_data = pdfium::MakeRetain<CPDF_Stream>();
    mask_in_data->InitStream(m_JpxInlineData.data, dict);
    return StartLoadMaskDIB(std::move(mask_in_data));
  }

  RetainPtr<const CPDF_Stream> mask(m_pDict->GetStreamFor("SMask"));
  if (!mask) {
    mask.Reset(ToStream(m_pDict->GetDirectObjectFor("Mask")));
    return mask ? StartLoadMaskDIB(std::move(mask)) : LoadState::kSuccess;
  }

  const CPDF_Array* pMatte = mask->GetDict()->GetArrayFor("Matte");
  if (pMatte && m_pColorSpace &&
      m_Family != CPDF_ColorSpace::Family::kPattern &&
      pMatte->size() == m_nComponents &&
      m_pColorSpace->CountComponents() <= m_nComponents) {
    std::vector<float> colors =
        ReadArrayElementsToVector(pMatte, m_nComponents);

    float R;
    float G;
    float B;
    m_pColorSpace->GetRGB(colors, &R, &G, &B);
    m_MatteColor = ArgbEncode(0, FXSYS_roundf(R * 255), FXSYS_roundf(G * 255),
                              FXSYS_roundf(B * 255));
  }
  return StartLoadMaskDIB(std::move(mask));
}

CPDF_DIB::LoadState CPDF_DIB::ContinueLoadMaskDIB(PauseIndicatorIface* pPause) {
  if (!m_pMask)
    return LoadState::kSuccess;

  LoadState ret = m_pMask->ContinueLoadDIBBase(pPause);
  if (ret == LoadState::kContinue)
    return LoadState::kContinue;

  if (m_pColorSpace && m_bStdCS)
    m_pColorSpace->EnableStdConversion(false);

  if (ret == LoadState::kFail) {
    m_pMask.Reset();
    return LoadState::kFail;
  }
  return LoadState::kSuccess;
}

RetainPtr<CPDF_DIB> CPDF_DIB::DetachMask() {
  return std::move(m_pMask);
}

bool CPDF_DIB::IsJBigImage() const {
  return m_pStreamAcc->GetImageDecoder() == "JBIG2Decode";
}

CPDF_DIB::LoadState CPDF_DIB::StartLoadMaskDIB(
    RetainPtr<const CPDF_Stream> mask) {
  m_pMask = pdfium::MakeRetain<CPDF_DIB>();
  LoadState ret = m_pMask->StartLoadDIBBase(
      m_pDocument.Get(), mask.Get(), false, nullptr, nullptr, true,
      CPDF_ColorSpace::Family::kUnknown, false);
  if (ret == LoadState::kContinue) {
    if (m_Status == LoadState::kFail)
      m_Status = LoadState::kContinue;
    return LoadState::kContinue;
  }
  if (ret == LoadState::kFail)
    m_pMask.Reset();
  return LoadState::kSuccess;
}

void CPDF_DIB::LoadPalette() {
  if (!m_pColorSpace || m_Family == CPDF_ColorSpace::Family::kPattern)
    return;

  if (m_bpc == 0)
    return;

  // Use FX_SAFE_UINT32 just to be on the safe side, in case |m_bpc| or
  // |m_nComponents| somehow gets a bad value.
  FX_SAFE_UINT32 safe_bits = m_bpc;
  safe_bits *= m_nComponents;
  uint32_t bits = safe_bits.ValueOrDefault(255);
  if (bits > 8)
    return;

  if (bits == 1) {
    if (m_bDefaultDecode && (m_Family == CPDF_ColorSpace::Family::kDeviceGray ||
                             m_Family == CPDF_ColorSpace::Family::kDeviceRGB)) {
      return;
    }
    if (m_pColorSpace->CountComponents() > 3) {
      return;
    }
    float color_values[3];
    std::fill(std::begin(color_values), std::end(color_values),
              m_CompData[0].m_DecodeMin);

    float R = 0.0f;
    float G = 0.0f;
    float B = 0.0f;
    m_pColorSpace->GetRGB(color_values, &R, &G, &B);

    FX_ARGB argb0 = ArgbEncode(255, FXSYS_roundf(R * 255),
                               FXSYS_roundf(G * 255), FXSYS_roundf(B * 255));
    color_values[0] += m_CompData[0].m_DecodeStep;
    color_values[1] += m_CompData[0].m_DecodeStep;
    color_values[2] += m_CompData[0].m_DecodeStep;
    m_pColorSpace->GetRGB(color_values, &R, &G, &B);
    FX_ARGB argb1 = ArgbEncode(255, FXSYS_roundf(R * 255),
                               FXSYS_roundf(G * 255), FXSYS_roundf(B * 255));
    if (argb0 != 0xFF000000 || argb1 != 0xFFFFFFFF) {
      SetPaletteArgb(0, argb0);
      SetPaletteArgb(1, argb1);
    }
    return;
  }
  if (m_bpc == 8 && m_bDefaultDecode &&
      m_pColorSpace ==
          CPDF_ColorSpace::GetStockCS(CPDF_ColorSpace::Family::kDeviceGray)) {
    return;
  }

  int palette_count = 1 << bits;
  // Using at least 16 elements due to the call m_pColorSpace->GetRGB().
  std::vector<float> color_values(std::max(m_nComponents, 16u));
  for (int i = 0; i < palette_count; i++) {
    int color_data = i;
    for (uint32_t j = 0; j < m_nComponents; j++) {
      int encoded_component = color_data % (1 << m_bpc);
      color_data /= 1 << m_bpc;
      color_values[j] = m_CompData[j].m_DecodeMin +
                        m_CompData[j].m_DecodeStep * encoded_component;
    }
    float R = 0;
    float G = 0;
    float B = 0;
    if (m_nComponents == 1 && m_Family == CPDF_ColorSpace::Family::kICCBased &&
        m_pColorSpace->CountComponents() > 1) {
      int nComponents = m_pColorSpace->CountComponents();
      std::vector<float> temp_buf(nComponents);
      for (int k = 0; k < nComponents; ++k)
        temp_buf[k] = color_values[0];
      m_pColorSpace->GetRGB(temp_buf, &R, &G, &B);
    } else {
      m_pColorSpace->GetRGB(color_values, &R, &G, &B);
    }
    SetPaletteArgb(i, ArgbEncode(255, FXSYS_roundf(R * 255),
                                 FXSYS_roundf(G * 255), FXSYS_roundf(B * 255)));
  }
}

void CPDF_DIB::ValidateDictParam(const ByteString& filter) {
  m_bpc = m_bpc_orig;

  // Per spec, |m_bpc| should always be 8 for RunLengthDecode, but too many
  // documents do not conform to it. So skip this check.

  if (filter == "JPXDecode") {
    m_bDoBpcCheck = false;
    return;
  }

  if (filter == "CCITTFaxDecode" || filter == "JBIG2Decode") {
    m_bpc = 1;
    m_nComponents = 1;
  } else if (filter == "DCTDecode") {
    m_bpc = 8;
  }

  if (!IsAllowedBitsPerComponent(m_bpc))
    m_bpc = 0;
}

void CPDF_DIB::TranslateScanline24bpp(uint8_t* dest_scan,
                                      const uint8_t* src_scan) const {
  if (m_bpc == 0)
    return;

  if (TranslateScanline24bppDefaultDecode(dest_scan, src_scan))
    return;

  // Using at least 16 elements due to the call m_pColorSpace->GetRGB().
  std::vector<float> color_values(std::max(m_nComponents, 16u));
  float R = 0.0f;
  float G = 0.0f;
  float B = 0.0f;
  uint64_t src_bit_pos = 0;
  uint64_t src_byte_pos = 0;
  size_t dest_byte_pos = 0;
  const bool bpp8 = m_bpc == 8;
  for (int column = 0; column < m_Width; column++) {
    for (uint32_t color = 0; color < m_nComponents; color++) {
      if (bpp8) {
        uint8_t data = src_scan[src_byte_pos++];
        color_values[color] = m_CompData[color].m_DecodeMin +
                              m_CompData[color].m_DecodeStep * data;
      } else {
        unsigned int data = GetBits8(src_scan, src_bit_pos, m_bpc);
        color_values[color] = m_CompData[color].m_DecodeMin +
                              m_CompData[color].m_DecodeStep * data;
        src_bit_pos += m_bpc;
      }
    }

    if (TransMask()) {
      float k = 1.0f - color_values[3];
      R = (1.0f - color_values[0]) * k;
      G = (1.0f - color_values[1]) * k;
      B = (1.0f - color_values[2]) * k;
    } else if (m_Family != CPDF_ColorSpace::Family::kPattern) {
      m_pColorSpace->GetRGB(color_values, &R, &G, &B);
    }
    R = pdfium::clamp(R, 0.0f, 1.0f);
    G = pdfium::clamp(G, 0.0f, 1.0f);
    B = pdfium::clamp(B, 0.0f, 1.0f);
    dest_scan[dest_byte_pos] = static_cast<uint8_t>(B * 255);
    dest_scan[dest_byte_pos + 1] = static_cast<uint8_t>(G * 255);
    dest_scan[dest_byte_pos + 2] = static_cast<uint8_t>(R * 255);
    dest_byte_pos += 3;
  }
}

bool CPDF_DIB::TranslateScanline24bppDefaultDecode(
    uint8_t* dest_scan,
    const uint8_t* src_scan) const {
  if (!m_bDefaultDecode)
    return false;

  if (m_Family != CPDF_ColorSpace::Family::kDeviceRGB &&
      m_Family != CPDF_ColorSpace::Family::kCalRGB) {
    if (m_bpc != 8)
      return false;

    if (m_nComponents == m_pColorSpace->CountComponents()) {
      m_pColorSpace->TranslateImageLine(dest_scan, src_scan, m_Width, m_Width,
                                        m_Height, TransMask());
    }
    return true;
  }

  if (m_nComponents != 3)
    return true;

  const uint8_t* src_pos = src_scan;
  switch (m_bpc) {
    case 8:
      for (int column = 0; column < m_Width; column++) {
        *dest_scan++ = src_pos[2];
        *dest_scan++ = src_pos[1];
        *dest_scan++ = *src_pos;
        src_pos += 3;
      }
      break;
    case 16:
      for (int col = 0; col < m_Width; col++) {
        *dest_scan++ = src_pos[4];
        *dest_scan++ = src_pos[2];
        *dest_scan++ = *src_pos;
        src_pos += 6;
      }
      break;
    default:
      const unsigned int max_data = (1 << m_bpc) - 1;
      uint64_t src_bit_pos = 0;
      size_t dest_byte_pos = 0;
      for (int column = 0; column < m_Width; column++) {
        unsigned int R = GetBits8(src_scan, src_bit_pos, m_bpc);
        src_bit_pos += m_bpc;
        unsigned int G = GetBits8(src_scan, src_bit_pos, m_bpc);
        src_bit_pos += m_bpc;
        unsigned int B = GetBits8(src_scan, src_bit_pos, m_bpc);
        src_bit_pos += m_bpc;
        R = std::min(R, max_data);
        G = std::min(G, max_data);
        B = std::min(B, max_data);
        dest_scan[dest_byte_pos] = B * 255 / max_data;
        dest_scan[dest_byte_pos + 1] = G * 255 / max_data;
        dest_scan[dest_byte_pos + 2] = R * 255 / max_data;
        dest_byte_pos += 3;
      }
      break;
  }
  return true;
}

uint8_t* CPDF_DIB::GetBuffer() const {
  return m_pCachedBitmap ? m_pCachedBitmap->GetBuffer() : nullptr;
}

const uint8_t* CPDF_DIB::GetScanline(int line) const {
  if (m_bpc == 0)
    return nullptr;

  const Optional<uint32_t> src_pitch =
      fxcodec::CalculatePitch8(m_bpc, m_nComponents, m_Width);
  if (!src_pitch.has_value())
    return nullptr;

  uint32_t src_pitch_value = src_pitch.value();
  const uint8_t* pSrcLine = nullptr;
  if (m_pCachedBitmap && src_pitch_value <= m_pCachedBitmap->GetPitch()) {
    if (line >= m_pCachedBitmap->GetHeight()) {
      line = m_pCachedBitmap->GetHeight() - 1;
    }
    pSrcLine = m_pCachedBitmap->GetScanline(line);
  } else if (m_pDecoder) {
    pSrcLine = m_pDecoder->GetScanline(line);
  } else if (m_pStreamAcc->GetSize() >= (line + 1) * src_pitch_value) {
    pSrcLine = m_pStreamAcc->GetData() + line * src_pitch_value;
  }
  if (!pSrcLine) {
    uint8_t* pLineBuf = m_pMaskedLine ? m_pMaskedLine.get() : m_pLineBuf.get();
    memset(pLineBuf, 0xFF, m_Pitch);
    return pLineBuf;
  }

  if (m_bpc * m_nComponents == 1) {
    if (m_bImageMask && m_bDefaultDecode) {
      for (uint32_t i = 0; i < src_pitch_value; i++)
        m_pLineBuf.get()[i] = ~pSrcLine[i];
      return m_pLineBuf.get();
    }

    if (!m_bColorKey) {
      memcpy(m_pLineBuf.get(), pSrcLine, src_pitch_value);
      return m_pLineBuf.get();
    }

    uint32_t reset_argb = Get1BitResetValue();
    uint32_t set_argb = Get1BitSetValue();
    uint32_t* dest_scan = reinterpret_cast<uint32_t*>(m_pMaskedLine.get());
    for (int col = 0; col < m_Width; col++) {
      *dest_scan = GetBitValue(pSrcLine, col) ? set_argb : reset_argb;
      dest_scan++;
    }
    return m_pMaskedLine.get();
  }
  if (m_bpc * m_nComponents <= 8) {
    if (m_bpc == 8) {
      memcpy(m_pLineBuf.get(), pSrcLine, src_pitch_value);
    } else {
      uint64_t src_bit_pos = 0;
      for (int col = 0; col < m_Width; col++) {
        unsigned int color_index = 0;
        for (uint32_t color = 0; color < m_nComponents; color++) {
          unsigned int data = GetBits8(pSrcLine, src_bit_pos, m_bpc);
          color_index |= data << (color * m_bpc);
          src_bit_pos += m_bpc;
        }
        m_pLineBuf.get()[col] = color_index;
      }
    }
    if (!m_bColorKey)
      return m_pLineBuf.get();

    uint8_t* pDestPixel = m_pMaskedLine.get();
    const uint8_t* pSrcPixel = m_pLineBuf.get();
    pdfium::span<const uint32_t> palette = GetPaletteSpan();
    for (int col = 0; col < m_Width; col++) {
      uint8_t index = *pSrcPixel++;
      if (HasPalette()) {
        *pDestPixel++ = FXARGB_B(palette[index]);
        *pDestPixel++ = FXARGB_G(palette[index]);
        *pDestPixel++ = FXARGB_R(palette[index]);
      } else {
        *pDestPixel++ = index;
        *pDestPixel++ = index;
        *pDestPixel++ = index;
      }
      *pDestPixel = IsColorIndexOutOfBounds(index, m_CompData[0]) ? 0xFF : 0;
      pDestPixel++;
    }
    return m_pMaskedLine.get();
  }
  if (m_bColorKey) {
    if (m_nComponents == 3 && m_bpc == 8) {
      uint8_t* alpha_channel = m_pMaskedLine.get() + 3;
      for (int col = 0; col < m_Width; col++) {
        const uint8_t* pPixel = pSrcLine + col * 3;
        alpha_channel[col * 4] =
            AreColorIndicesOutOfBounds(pPixel, m_CompData.data(), 3) ? 0xFF : 0;
      }
    } else {
      memset(m_pMaskedLine.get(), 0xFF, m_Pitch);
    }
  }
  if (m_pColorSpace) {
    TranslateScanline24bpp(m_pLineBuf.get(), pSrcLine);
    pSrcLine = m_pLineBuf.get();
  }
  if (!m_bColorKey)
    return pSrcLine;

  const uint8_t* pSrcPixel = pSrcLine;
  uint8_t* pDestPixel = m_pMaskedLine.get();
  for (int col = 0; col < m_Width; col++) {
    *pDestPixel++ = *pSrcPixel++;
    *pDestPixel++ = *pSrcPixel++;
    *pDestPixel++ = *pSrcPixel++;
    pDestPixel++;
  }
  return m_pMaskedLine.get();
}

bool CPDF_DIB::SkipToScanline(int line, PauseIndicatorIface* pPause) const {
  return m_pDecoder && m_pDecoder->SkipToScanline(line, pPause);
}

bool CPDF_DIB::TransMask() const {
  return m_bLoadMask && m_GroupFamily == CPDF_ColorSpace::Family::kDeviceCMYK &&
         m_Family == CPDF_ColorSpace::Family::kDeviceCMYK;
}

void CPDF_DIB::SetMaskProperties() {
  m_bpc = 1;
  m_nComponents = 1;
  m_Format = FXDIB_Format::k1bppMask;
}

uint32_t CPDF_DIB::Get1BitSetValue() const {
  if (m_CompData[0].m_ColorKeyMax == 1)
    return 0x00000000;
  return HasPalette() ? GetPaletteSpan()[1] : 0xFFFFFFFF;
}

uint32_t CPDF_DIB::Get1BitResetValue() const {
  if (m_CompData[0].m_ColorKeyMin == 0)
    return 0x00000000;
  return HasPalette() ? GetPaletteSpan()[0] : 0xFF000000;
}
