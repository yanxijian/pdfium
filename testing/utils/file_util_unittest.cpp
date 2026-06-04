// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/utils/file_util.h"

#include <fcntl.h>

#include <string>

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_POSIX)
#include <errno.h>
#include <unistd.h>
#endif

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

namespace pdfium {

TEST(FileUtilTest, ScopedFILENull) {
  ScopedFILE file;
  EXPECT_FALSE(file);
  EXPECT_EQ(nullptr, file.get());
}

#if BUILDFLAG(IS_POSIX)
TEST(FileUtilTest, ScopedFILECloses) {
  int fd = -1;
  std::string temp_path = testing::TempDir() + "scoped_file_test.txt";
  {
    ScopedFILE file(fopen(temp_path.c_str(), "w+b"));
    ASSERT_TRUE(file);
    fd = fileno(file.get());
    ASSERT_GE(fd, 0);
    EXPECT_NE(fcntl(fd, F_GETFD), -1);
  }
  EXPECT_EQ(fcntl(fd, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
  unlink(temp_path.c_str());
}

TEST(FileUtilTest, ScopedFILERelease) {
  std::string temp_path = testing::TempDir() + "scoped_file_test.txt";
  FILE* raw_file = fopen(temp_path.c_str(), "w+b");
  ASSERT_TRUE(raw_file);
  int fd = fileno(raw_file);
  {
    ScopedFILE file(raw_file);
    EXPECT_EQ(raw_file, file.release());
  }
  // Since we released it, it should still be open.
  EXPECT_NE(fcntl(fd, F_GETFD), -1);
  fclose(raw_file);
  unlink(temp_path.c_str());
}

TEST(FileUtilTest, ScopedFDNull) {
  ScopedFD fd;
  EXPECT_FALSE(fd.is_valid());
  EXPECT_EQ(-1, fd.get());
}

TEST(FileUtilTest, ScopedFDCloses) {
  int fd = -1;
  std::string temp_path = testing::TempDir() + "scoped_fd_test.txt";
  {
    fd = open(temp_path.c_str(), O_CREAT | O_RDWR, 0666);
    ASSERT_GE(fd, 0);
    ScopedFD scoped_fd(fd);
    EXPECT_TRUE(scoped_fd.is_valid());
    EXPECT_EQ(fd, scoped_fd.get());
    EXPECT_NE(fcntl(fd, F_GETFD), -1);
  }
  EXPECT_EQ(fcntl(fd, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
  unlink(temp_path.c_str());
}

TEST(FileUtilTest, ScopedFDRelease) {
  std::string temp_path = testing::TempDir() + "scoped_fd_test.txt";
  int fd = open(temp_path.c_str(), O_CREAT | O_RDWR, 0666);
  ASSERT_GE(fd, 0);
  {
    ScopedFD scoped_fd(fd);
    EXPECT_EQ(fd, scoped_fd.release());
  }
  // Since we released it, it should still be open.
  EXPECT_NE(fcntl(fd, F_GETFD), -1);
  close(fd);
  unlink(temp_path.c_str());
}
#endif  // BUILDFLAG(IS_POSIX)

#if BUILDFLAG(IS_WIN)
TEST(FileUtilTest, ScopedHandleNull) {
  ScopedHandle handle;
  EXPECT_FALSE(handle.is_valid());
  EXPECT_EQ(INVALID_HANDLE_VALUE, handle.get());
}

TEST(FileUtilTest, ScopedHandleCloses) {
  HANDLE handle = INVALID_HANDLE_VALUE;
  {
    handle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    ASSERT_NE(handle, nullptr);
    ASSERT_NE(handle, INVALID_HANDLE_VALUE);
    ScopedHandle scoped_handle(handle);
    EXPECT_TRUE(scoped_handle.is_valid());
    EXPECT_EQ(handle, scoped_handle.get());

    DWORD flags;
    EXPECT_TRUE(GetHandleInformation(handle, &flags));
  }
  DWORD flags;
  EXPECT_FALSE(GetHandleInformation(handle, &flags));
}

TEST(FileUtilTest, ScopedHandleRelease) {
  HANDLE handle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  ASSERT_NE(handle, nullptr);
  ASSERT_NE(handle, INVALID_HANDLE_VALUE);
  {
    ScopedHandle scoped_handle(handle);
    EXPECT_EQ(handle, scoped_handle.release());
  }
  DWORD flags;
  EXPECT_TRUE(GetHandleInformation(handle, &flags));
  CloseHandle(handle);
}
#endif  // BUILDFLAG(IS_WIN)

}  // namespace pdfium
