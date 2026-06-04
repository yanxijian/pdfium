// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_UTILS_SCOPED_TEMP_FILE_H_
#define TESTING_UTILS_SCOPED_TEMP_FILE_H_

#include <stdio.h>

#include <memory>
#include <string>

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

namespace testing {

// Helper functions to create, open, and close temporary files.
// All paths are based on GTest's TempDir().

enum class FileAccessMode { kReadOnly, kWriteOnly };

// Creates a unique temporary file on disk (empty) and returns its path.
// Returns true on success.
bool CreateTempFile(std::string* path);

// Opens a file by path.
#if BUILDFLAG(IS_POSIX)
int OpenTempFileFD(const std::string& path, FileAccessMode mode);
#endif
FILE* OpenTempFileStream(const std::string& path, const char* mode);
#if BUILDFLAG(IS_WIN)
HANDLE OpenTempFileHandle(const std::string& path, FileAccessMode mode);
#endif

// Wrappers to close.
#if BUILDFLAG(IS_POSIX)
void CloseFD(int fd);
#endif
void CloseStream(FILE* fp);
#if BUILDFLAG(IS_WIN)
void CloseHandle(HANDLE h);
#endif

// Scoped wrappers for automatic cleanup.

// Scoped FILE* closer.
struct FileCloser {
  void operator()(FILE* fp) const { CloseStream(fp); }
};
using ScopedFILE = std::unique_ptr<FILE, FileCloser>;

#if BUILDFLAG(IS_POSIX)
// Scoped POSIX file descriptor.
class ScopedFD {
 public:
  ScopedFD() = default;
  explicit ScopedFD(int fd) : fd_(fd) {}
  ~ScopedFD() { Reset(); }

  ScopedFD(const ScopedFD&) = delete;
  ScopedFD& operator=(const ScopedFD&) = delete;

  ScopedFD(ScopedFD&& o) noexcept : fd_(o.release()) {}
  ScopedFD& operator=(ScopedFD&& o) noexcept {
    Reset(o.release());
    return *this;
  }

  int get() const { return fd_; }
  bool is_valid() const { return fd_ >= 0; }

  int release() {
    int ret = fd_;
    fd_ = -1;
    return ret;
  }

  void Reset(int new_fd = -1) {
    if (fd_ >= 0) {
      CloseFD(fd_);
    }
    fd_ = new_fd;
  }

 private:
  int fd_ = -1;
};
#endif  // BUILDFLAG(IS_POSIX)

#if BUILDFLAG(IS_WIN)
// Scoped Windows HANDLE.
class ScopedPlatformHandle {
 public:
  ScopedPlatformHandle() = default;
  explicit ScopedPlatformHandle(HANDLE h) : handle_(h) {}
  ~ScopedPlatformHandle() { Reset(); }

  ScopedPlatformHandle(const ScopedPlatformHandle&) = delete;
  ScopedPlatformHandle& operator=(const ScopedPlatformHandle&) = delete;

  ScopedPlatformHandle(ScopedPlatformHandle&& o) noexcept
      : handle_(o.release()) {}
  ScopedPlatformHandle& operator=(ScopedPlatformHandle&& o) noexcept {
    Reset(o.release());
    return *this;
  }

  HANDLE get() const { return handle_; }
  bool is_valid() const {
    return handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr;
  }

  HANDLE release() {
    HANDLE ret = handle_;
    handle_ = INVALID_HANDLE_VALUE;
    return ret;
  }

  void Reset(HANDLE new_h = INVALID_HANDLE_VALUE) {
    if (is_valid()) {
      CloseHandle(handle_);
    }
    handle_ = new_h;
  }

 private:
  HANDLE handle_ = INVALID_HANDLE_VALUE;
};
#endif  // BUILDFLAG(IS_WIN)

// ScopedTempFile manages the lifetime of a temporary file on disk.
// It deletes the file when it goes out of scope.
class ScopedTempFile {
 public:
  static std::unique_ptr<ScopedTempFile> Create();
  ~ScopedTempFile();

  ScopedTempFile(const ScopedTempFile&) = delete;
  ScopedTempFile& operator=(const ScopedTempFile&) = delete;

  const std::string& path() const { return path_; }

 private:
  explicit ScopedTempFile(std::string path);

  const std::string path_;
};

}  // namespace testing

#endif  // TESTING_UTILS_SCOPED_TEMP_FILE_H_
