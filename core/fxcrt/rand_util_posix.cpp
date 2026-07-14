// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/rand_util.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "build/build_config.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/compiler_specific.h"

#if BUILDFLAG(IS_APPLE)
#include <sys/random.h>
#endif

namespace {

#if BUILDFLAG(IS_AIX)
constexpr int kOpenFlags = O_RDONLY;
#else
constexpr int kOpenFlags = O_RDONLY | O_CLOEXEC;
#endif

int OpenURandomFD() {
  int fd = -1;
  do {
    fd = open("/dev/urandom", kOpenFlags);
  } while (fd == -1 && errno == EINTR);
  CHECK(fd >= 0);
  return fd;
}

int GetURandomFD() {
  static const int fd = OpenURandomFD();
  return fd;
}

bool ReadFromURandomFD(int fd, pdfium::span<uint8_t> output) {
  size_t total_read = 0;
  while (total_read < output.size()) {
    pdfium::span<uint8_t> remaining = output.subspan(total_read);
    ssize_t bytes_read = read(fd, remaining.data(), remaining.size());
    if (bytes_read <= 0) {
      if (bytes_read == -1 && errno == EINTR) {
        continue;
      }
      return false;
    }
    total_read += static_cast<size_t>(bytes_read);
  }
  return true;
}

}  // namespace

namespace pdfium {

void RandBytes(pdfium::span<uint8_t> output) {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
  ssize_t ret;
  do {
    ret = syscall(__NR_getrandom, output.data(), output.size(), 0);
  } while (ret == -1 && errno == EINTR);
  if (output.size() == static_cast<size_t>(ret)) {
    return;
  }
#elif BUILDFLAG(IS_APPLE)
  if (getentropy(output.data(), output.size()) == 0) {
    return;
  }
#endif

  const int urandom_fd = GetURandomFD();
  const bool success = ReadFromURandomFD(urandom_fd, output);
  CHECK(success);
}

}  // namespace pdfium
