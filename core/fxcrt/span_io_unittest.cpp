// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/span_io.h"

#include <stdio.h>

#include <string>
#include <string_view>

#include "build/build_config.h"
#include "core/fxcrt/span.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_POSIX)
#include <fcntl.h>
#include <unistd.h>
#endif

TEST(Spanread, ReadNormal) {
  FILE* f = tmpfile();
  ASSERT_TRUE(f);
  const char kData[] = "hello world";
  ASSERT_EQ(UNSAFE_BUFFERS(fwrite(kData, 1, sizeof(kData), f)), sizeof(kData));
  rewind(f);

  char buffer[100];
  pdfium::span<char> read_span = fxcrt::spanread(buffer, f);
  EXPECT_EQ(read_span.size(), sizeof(kData));
  EXPECT_STREQ(read_span.data(), kData);
  fclose(f);
}

TEST(Spanread, ReadTruncated) {
  FILE* f = tmpfile();
  ASSERT_TRUE(f);
  const char kData[] = "hello world";
  ASSERT_EQ(UNSAFE_BUFFERS(fwrite(kData, 1, sizeof(kData), f)), sizeof(kData));
  rewind(f);

  char buffer[5];
  pdfium::span<char> read_span = fxcrt::spanread(buffer, f);
  EXPECT_EQ(read_span.size(), 5u);
  EXPECT_EQ(std::string_view(read_span.data(), read_span.size()), "hello");
  fclose(f);
}

TEST(Spanwrite, WriteNormal) {
  FILE* f = tmpfile();
  ASSERT_TRUE(f);
  const char kData[] = "hello world";

  // Also test that raw array is acceptable (implicit conversion).
  EXPECT_EQ(fxcrt::spanwrite(kData, f), sizeof(kData));

  rewind(f);
  char buffer[100];
  size_t read = UNSAFE_BUFFERS(fread(buffer, 1, sizeof(kData), f));
  EXPECT_EQ(read, sizeof(kData));
  EXPECT_STREQ(buffer, kData);
  fclose(f);
}

#if BUILDFLAG(IS_POSIX)
TEST(SpanreadFD, ReadNormal) {
  std::string path = testing::TempDir() + "/pdfium_span_unittest_XXXXXX";
  int fd = mkstemp(path.data());
  ASSERT_GE(fd, 0);

  const char kData[] = "hello world";
  ASSERT_EQ(write(fd, kData, sizeof(kData)),
            static_cast<ssize_t>(sizeof(kData)));
  lseek(fd, 0, SEEK_SET);

  char buffer[100];
  pdfium::span<char> read_span = fxcrt::spanread(buffer, fd);
  EXPECT_EQ(read_span.size(), sizeof(kData));
  EXPECT_STREQ(read_span.data(), kData);

  close(fd);
  unlink(path.c_str());
}

TEST(SpanwriteFD, WriteNormal) {
  std::string path = testing::TempDir() + "/pdfium_span_unittest_XXXXXX";
  int fd = mkstemp(path.data());
  ASSERT_GE(fd, 0);

  const char kData[] = "hello world";
  EXPECT_EQ(fxcrt::spanwrite(kData, fd), sizeof(kData));

  lseek(fd, 0, SEEK_SET);
  char buffer[100];
  ssize_t bytes_read = read(fd, buffer, sizeof(kData));
  EXPECT_EQ(bytes_read, static_cast<ssize_t>(sizeof(kData)));
  EXPECT_STREQ(buffer, kData);

  close(fd);
  unlink(path.c_str());
}
#endif  // BUILDFLAG(IS_POSIX)

#if BUILDFLAG(IS_WIN)
#include <windows.h>

TEST(SpanreadWin, ReadNormal) {
  std::string path = testing::TempDir() + "/pdfium_span_win_unittest.tmp";
  std::wstring wpath(path.begin(), path.end());

  HANDLE hFile = ::CreateFileW(
      wpath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
      FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(hFile, INVALID_HANDLE_VALUE);

  const char kData[] = "hello world";
  DWORD bytes_written = 0;
  ASSERT_TRUE(
      ::WriteFile(hFile, kData, sizeof(kData), &bytes_written, nullptr));
  ASSERT_EQ(bytes_written, sizeof(kData));

  // Seek to beginning
  ASSERT_NE(::SetFilePointer(hFile, 0, nullptr, FILE_BEGIN),
            INVALID_SET_FILE_POINTER);

  char buffer[100];
  pdfium::span<char> read_span = fxcrt::spanread(buffer, hFile);
  EXPECT_EQ(read_span.size(), sizeof(kData));
  EXPECT_STREQ(read_span.data(), kData);

  ::CloseHandle(hFile);  // Auto-deletes
}

TEST(SpanwriteWin, WriteNormal) {
  std::string path = testing::TempDir() + "/pdfium_span_win_unittest.tmp";
  std::wstring wpath(path.begin(), path.end());

  HANDLE hFile = ::CreateFileW(
      wpath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
      FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
  ASSERT_NE(hFile, INVALID_HANDLE_VALUE);

  const char kData[] = "hello world";
  EXPECT_EQ(fxcrt::spanwrite(kData, hFile), sizeof(kData));

  // Seek to beginning
  ASSERT_NE(::SetFilePointer(hFile, 0, nullptr, FILE_BEGIN),
            INVALID_SET_FILE_POINTER);

  char buffer[100];
  DWORD bytes_read = 0;
  ASSERT_TRUE(::ReadFile(hFile, buffer, sizeof(kData), &bytes_read, nullptr));
  EXPECT_EQ(bytes_read, sizeof(kData));
  EXPECT_STREQ(buffer, kData);

  ::CloseHandle(hFile);  // Auto-deletes
}
#endif  // BUILDFLAG(IS_WIN)
