// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/bytestring_pool.h"

namespace fxcrt {

size_t ByteStringPool::Hash::operator()(ByteStringView view) const {
  return FX_HashCode_GetA(view);
}

size_t ByteStringPool::Hash::operator()(const ByteString& str) const {
  return FX_HashCode_GetA(str.AsStringView());
}

ByteStringPool::ByteStringPool() = default;

ByteStringPool::~ByteStringPool() = default;

ByteString ByteStringPool::Intern(const ByteString& str) {
  if (str.IsEmpty()) {
    return str;
  }
  return *pool_.insert(str).first;
}

ByteString ByteStringPool::Intern(ByteStringView str) {
  if (str.IsEmpty()) {
    return ByteString();
  }
  auto it = pool_.find(str);
  if (it != pool_.end()) {
    return *it;
  }
  return *pool_.insert(ByteString(str)).first;
}

}  // namespace fxcrt
