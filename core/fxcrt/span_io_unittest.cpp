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
#include "testing/utils/scoped_temp_file.h"

#if BUILDFLAG(IS_POSIX)
#include <fcntl.h>
#include <unistd.h>
#endif

#if BUILDFLAG(IS_WIN)
#include <windows.h>
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

TEST(Spanread, ReadEmpty) {
  FILE* f = tmpfile();
  ASSERT_TRUE(f);
  pdfium::span<char> empty_span;
  pdfium::span<char> read_span = fxcrt::spanread(empty_span, f);
  EXPECT_TRUE(read_span.empty());
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

TEST(Spanwrite, WriteEmpty) {
  FILE* f = tmpfile();
  ASSERT_TRUE(f);
  pdfium::span<const char> empty_span;
  EXPECT_EQ(fxcrt::spanwrite(empty_span, f), 0u);
  fclose(f);
}

#if BUILDFLAG(IS_POSIX)
TEST(SpanreadFD, ReadNormal) {
  auto temp_file = testing::ScopedTempFile::Create();
  ASSERT_TRUE(temp_file);
  testing::ScopedFD fd(testing::OpenTempFileFD(temp_file->path()));
  ASSERT_TRUE(fd.is_valid());

  const char kData[] = "hello world";
  ASSERT_EQ(write(fd.get(), kData, sizeof(kData)),
            static_cast<ssize_t>(sizeof(kData)));
  lseek(fd.get(), 0, SEEK_SET);

  char buffer[100];
  pdfium::span<char> read_span = fxcrt::spanread(buffer, fd.get());
  EXPECT_EQ(read_span.size(), sizeof(kData));
  EXPECT_STREQ(read_span.data(), kData);
}

TEST(SpanreadFD, ReadEmpty) {
  auto temp_file = testing::ScopedTempFile::Create();
  ASSERT_TRUE(temp_file);
  testing::ScopedFD fd(testing::OpenTempFileFD(temp_file->path()));
  ASSERT_TRUE(fd.is_valid());
  pdfium::span<char> empty_span;
  pdfium::span<char> read_span = fxcrt::spanread(empty_span, fd.get());
  EXPECT_TRUE(read_span.empty());
}

TEST(SpanwriteFD, WriteNormal) {
  auto temp_file = testing::ScopedTempFile::Create();
  ASSERT_TRUE(temp_file);
  testing::ScopedFD fd(testing::OpenTempFileFD(temp_file->path()));
  ASSERT_TRUE(fd.is_valid());

  const char kData[] = "hello world";
  EXPECT_EQ(fxcrt::spanwrite(kData, fd.get()), sizeof(kData));

  lseek(fd.get(), 0, SEEK_SET);
  char buffer[100];
  ssize_t bytes_read = read(fd.get(), buffer, sizeof(kData));
  EXPECT_EQ(bytes_read, static_cast<ssize_t>(sizeof(kData)));
  EXPECT_STREQ(buffer, kData);
}

TEST(SpanwriteFD, WriteEmpty) {
  auto temp_file = testing::ScopedTempFile::Create();
  ASSERT_TRUE(temp_file);
  testing::ScopedFD fd(testing::OpenTempFileFD(temp_file->path()));
  ASSERT_TRUE(fd.is_valid());
  pdfium::span<const char> empty_span;
  EXPECT_EQ(fxcrt::spanwrite(empty_span, fd.get()), 0u);
}
#endif  // BUILDFLAG(IS_POSIX)

#if BUILDFLAG(IS_WIN)
TEST(SpanreadWin, ReadNormal) {
  auto temp_file = testing::ScopedTempFile::Create();
  ASSERT_TRUE(temp_file);
  testing::ScopedPlatformHandle handle(
      testing::OpenTempFileHandle(temp_file->path()));
  ASSERT_TRUE(handle.is_valid());

  const char kData[] = "hello world";
  DWORD bytes_written = 0;
  ASSERT_TRUE(
      ::WriteFile(handle.get(), kData, sizeof(kData), &bytes_written, nullptr));
  ASSERT_EQ(bytes_written, sizeof(kData));

  // Seek to beginning
  ASSERT_NE(::SetFilePointer(handle.get(), 0, nullptr, FILE_BEGIN),
            INVALID_SET_FILE_POINTER);

  char buffer[100];
  pdfium::span<char> read_span = fxcrt::spanread(buffer, handle.get());
  EXPECT_EQ(read_span.size(), sizeof(kData));
  EXPECT_STREQ(read_span.data(), kData);
}

TEST(SpanreadWin, ReadEmpty) {
  auto temp_file = testing::ScopedTempFile::Create();
  ASSERT_TRUE(temp_file);
  testing::ScopedPlatformHandle handle(
      testing::OpenTempFileHandle(temp_file->path()));
  ASSERT_TRUE(handle.is_valid());
  pdfium::span<char> empty_span;
  pdfium::span<char> read_span = fxcrt::spanread(empty_span, handle.get());
  EXPECT_TRUE(read_span.empty());
}

TEST(SpanwriteWin, WriteNormal) {
  auto temp_file = testing::ScopedTempFile::Create();
  ASSERT_TRUE(temp_file);
  testing::ScopedPlatformHandle handle(
      testing::OpenTempFileHandle(temp_file->path()));
  ASSERT_TRUE(handle.is_valid());

  const char kData[] = "hello world";
  EXPECT_EQ(fxcrt::spanwrite(kData, handle.get()), sizeof(kData));

  // Seek to beginning
  ASSERT_NE(::SetFilePointer(handle.get(), 0, nullptr, FILE_BEGIN),
            INVALID_SET_FILE_POINTER);

  char buffer[100];
  DWORD bytes_read = 0;
  ASSERT_TRUE(
      ::ReadFile(handle.get(), buffer, sizeof(kData), &bytes_read, nullptr));
  EXPECT_EQ(bytes_read, sizeof(kData));
  EXPECT_STREQ(buffer, kData);
}

TEST(SpanwriteWin, WriteEmpty) {
  auto temp_file = testing::ScopedTempFile::Create();
  ASSERT_TRUE(temp_file);
  testing::ScopedPlatformHandle handle(
      testing::OpenTempFileHandle(temp_file->path()));
  ASSERT_TRUE(handle.is_valid());
  pdfium::span<const char> empty_span;
  EXPECT_EQ(fxcrt::spanwrite(empty_span, handle.get()), 0u);
}
#endif  // BUILDFLAG(IS_WIN)
