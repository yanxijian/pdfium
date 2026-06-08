// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_ANDROID_CFX_ANDROIDFONTINFO_H_
#define CORE_FXGE_ANDROID_CFX_ANDROIDFONTINFO_H_

#include "core/fxge/cfx_folderfontinfo.h"

class CFX_AndroidFontInfo final : public CFX_FolderFontInfo {
 public:
  CFX_AndroidFontInfo();
  ~CFX_AndroidFontInfo() override;

  bool Init(const char** user_paths);

  // SystemFontInfoIface:
  void* MapFont(int weight,
                bool bItalic,
                FX_Charset charset,
                int pitch_family,
                const ByteString& face) override;
};

#endif  // CORE_FXGE_ANDROID_CFX_ANDROIDFONTINFO_H_
