// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/utils/scoped_temp_file.h"

#include <fcntl.h>
#include <stdio.h>

#include <utility>

#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_POSIX)
#include <unistd.h>
#endif

#if BUILDFLAG(IS_WIN)
#include <io.h>
#include <share.h>
#include <windows.h>
#endif

namespace testing {

namespace {

std::string JoinPath(const std::string& dir, const std::string& file) {
  if (dir.empty()) {
    return file;
  }
  if (dir.back() == '/' || dir.back() == '\\') {
    return dir + file;
  }
#if BUILDFLAG(IS_WIN)
  return dir + '\\' + file;
#else
  return dir + '/' + file;
#endif
}

}  // namespace

bool CreateTempFile(std::string* path) {
  std::string temp_dir = testing::TempDir();
#if BUILDFLAG(IS_WIN)
  char temp_file[_MAX_PATH + 1] = {'\0'};
  if (::GetTempFileNameA(temp_dir.c_str(), "pdf", 0, temp_file) == 0) {
    return false;
  }
  *path = temp_file;
  return true;
#else
  *path = JoinPath(temp_dir, "pdfium_XXXXXX");
  int fd = mkstemp(path->data());
  if (fd < 0) {
    return false;
  }
  close(fd);
  return true;
#endif
}

int OpenTempFileFD(const std::string& path) {
#if BUILDFLAG(IS_WIN)
  int fd;
  if (_sopen_s(&fd, path.c_str(), _O_RDWR | _O_BINARY, _SH_DENYNO,
               _S_IREAD | _S_IWRITE) != 0) {
    return -1;
  }
  return fd;
#else
  return open(path.c_str(), O_RDWR);
#endif
}

FILE* OpenTempFileStream(const std::string& path, const char* mode) {
  return fopen(path.c_str(), mode);
}

#if BUILDFLAG(IS_WIN)
HANDLE OpenTempFileHandle(const std::string& path) {
  std::wstring wpath(path.begin(), path.end());
  return ::CreateFileW(wpath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                       OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, nullptr);
}
#endif

void CloseFD(int fd) {
  if (fd >= 0) {
#if BUILDFLAG(IS_WIN)
    _close(fd);
#else
    close(fd);
#endif
  }
}

void CloseStream(FILE* fp) {
  if (fp) {
    fclose(fp);
  }
}

#if BUILDFLAG(IS_WIN)
void CloseHandle(HANDLE h) {
  if (h != INVALID_HANDLE_VALUE && h != nullptr) {
    ::CloseHandle(h);
  }
}
#endif

// static
std::unique_ptr<ScopedTempFile> ScopedTempFile::Create() {
  std::string path;
  if (!CreateTempFile(&path)) {
    return nullptr;
  }
  return std::unique_ptr<ScopedTempFile>(new ScopedTempFile(std::move(path)));
}

ScopedTempFile::ScopedTempFile(std::string path) : path_(std::move(path)) {}

ScopedTempFile::~ScopedTempFile() {
#if BUILDFLAG(IS_WIN)
  ::DeleteFileA(path_.c_str());
#else
  unlink(path_.c_str());
#endif
}

}  // namespace testing
