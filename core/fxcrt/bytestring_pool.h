// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_BYTESTRING_POOL_H_
#define CORE_FXCRT_BYTESTRING_POOL_H_

#include <unordered_set>

#include "core/fxcrt/bytestring.h"

namespace fxcrt {

class ByteStringPool {
 public:
  struct Hash {
    using is_transparent = void;
    size_t operator()(ByteStringView view) const;
    size_t operator()(const ByteString& str) const;
  };

  struct Equal {
    using is_transparent = void;
    bool operator()(ByteStringView lhs, ByteStringView rhs) const {
      return lhs == rhs;
    }
    bool operator()(const ByteString& lhs, ByteStringView rhs) const {
      return lhs.AsStringView() == rhs;
    }
    bool operator()(ByteStringView lhs, const ByteString& rhs) const {
      return lhs == rhs.AsStringView();
    }
    bool operator()(const ByteString& lhs, const ByteString& rhs) const {
      return lhs == rhs;
    }
  };

  ByteStringPool();
  ~ByteStringPool();

  ByteString Intern(const ByteString& str);
  ByteString Intern(ByteStringView str);
  ByteString Intern(const char* str) { return Intern(ByteStringView(str)); }

 private:
  std::unordered_set<ByteString, Hash, Equal> pool_;
};

}  // namespace fxcrt

using fxcrt::ByteStringPool;

#endif  // CORE_FXCRT_BYTESTRING_POOL_H_
