// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_UTILS_FILE_UTIL_H_
#define TESTING_UTILS_FILE_UTIL_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "build/build_config.h"
#include "public/fpdfview.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif
namespace pdfium {

// Scoper for FILE*.
struct FileCloser {
  void operator()(FILE* f) const;
};
using ScopedFILE = std::unique_ptr<FILE, FileCloser>;

#if BUILDFLAG(IS_POSIX)
// Scoper for POSIX file descriptor.
class ScopedFD {
 public:
  ScopedFD();
  explicit ScopedFD(int fd);
  ~ScopedFD();

  ScopedFD(const ScopedFD&) = delete;
  ScopedFD& operator=(const ScopedFD&) = delete;

  ScopedFD(ScopedFD&& other) noexcept;
  ScopedFD& operator=(ScopedFD&& other) noexcept;

  int get() const { return fd_; }
  bool is_valid() const { return fd_ >= 0; }

  int release();
  void reset(int fd = -1);

 private:
  int fd_ = -1;
};
#endif  // BUILDFLAG(IS_POSIX)

#if BUILDFLAG(IS_WIN)
// Scoper for Windows HANDLE.
class ScopedHandle {
 public:
  ScopedHandle();
  explicit ScopedHandle(HANDLE handle);
  ~ScopedHandle();

  ScopedHandle(const ScopedHandle&) = delete;
  ScopedHandle& operator=(const ScopedHandle&) = delete;

  ScopedHandle(ScopedHandle&& other) noexcept;
  ScopedHandle& operator=(ScopedHandle&& other) noexcept;

  HANDLE get() const { return handle_; }
  bool is_valid() const { return handle_ && handle_ != INVALID_HANDLE_VALUE; }

  HANDLE release();
  void reset(HANDLE handle = INVALID_HANDLE_VALUE);

 private:
  HANDLE handle_ = INVALID_HANDLE_VALUE;
};
#endif  // BUILDFLAG(IS_WIN)

}  // namespace pdfium

// Returns true if the path can be read from.
bool CanReadFile(const char* filename);

// Reads the entire contents of a file into a vector. Returns an empty vector on
// failure. Note that this function assumes reading an empty file is not a valid
// use case, and treats such an action as a failure.
std::vector<uint8_t> GetFileContents(const char* filename);

// Use an ordinary file anywhere a FPDF_FILEACCESS is required.
class FileAccessForTesting final : public FPDF_FILEACCESS {
 public:
  explicit FileAccessForTesting(const std::string& file_name);

 private:
  static int SGetBlock(void* param,
                       unsigned long pos,
                       unsigned char* pBuf,
                       unsigned long size);

  int GetBlockImpl(unsigned long pos, unsigned char* pBuf, unsigned long size);

  std::vector<uint8_t> file_contents_;
};

#endif  // TESTING_UTILS_FILE_UTIL_H_
