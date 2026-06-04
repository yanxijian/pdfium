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

#if BUILDFLAG(IS_POSIX)
#include <unistd.h>
#endif

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

namespace pdfium {

// Scoper for FILE*.
struct FileCloser {
  void operator()(FILE* f) const {
    if (f) {
      fclose(f);
    }
  }
};
using ScopedFILE = std::unique_ptr<FILE, FileCloser>;

#if BUILDFLAG(IS_POSIX)
// Scoper for POSIX file descriptor.
class ScopedFD {
 public:
  ScopedFD() = default;
  explicit ScopedFD(int fd) : fd_(fd) {}
  ~ScopedFD() {
    if (fd_ >= 0) {
      close(fd_);
    }
  }

  ScopedFD(const ScopedFD&) = delete;
  ScopedFD& operator=(const ScopedFD&) = delete;

  ScopedFD(ScopedFD&& other) noexcept : fd_(other.release()) {}
  ScopedFD& operator=(ScopedFD&& other) noexcept {
    reset(other.release());
    return *this;
  }

  int get() const { return fd_; }
  bool is_valid() const { return fd_ >= 0; }

  int release() {
    int fd = fd_;
    fd_ = -1;
    return fd;
  }

  void reset(int fd = -1) {
    if (fd_ >= 0) {
      close(fd_);
    }
    fd_ = fd;
  }

 private:
  int fd_ = -1;
};
#endif  // BUILDFLAG(IS_POSIX)

#if BUILDFLAG(IS_WIN)
// Scoper for Windows HANDLE.
class ScopedHandle {
 public:
  ScopedHandle() = default;
  explicit ScopedHandle(HANDLE handle) : handle_(handle) {}
  ~ScopedHandle() {
    if (handle_ && handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
    }
  }

  ScopedHandle(const ScopedHandle&) = delete;
  ScopedHandle& operator=(const ScopedHandle&) = delete;

  ScopedHandle(ScopedHandle&& other) noexcept : handle_(other.release()) {}
  ScopedHandle& operator=(ScopedHandle&& other) noexcept {
    reset(other.release());
    return *this;
  }

  HANDLE get() const { return handle_; }
  bool is_valid() const { return handle_ && handle_ != INVALID_HANDLE_VALUE; }

  HANDLE release() {
    HANDLE handle = handle_;
    handle_ = INVALID_HANDLE_VALUE;
    return handle;
  }

  void reset(HANDLE handle = INVALID_HANDLE_VALUE) {
    if (handle_ && handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
    }
    handle_ = handle;
  }

 private:
  HANDLE handle_ = INVALID_HANDLE_VALUE;
};
#endif  // BUILDFLAG(IS_WIN)

}  // namespace pdfium

#include "public/fpdfview.h"

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
