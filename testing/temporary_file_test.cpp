// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/temporary_file_test.h"

#include <unistd.h>

#include "core/fxcrt/span_io.h"
#include "testing/utils/scoped_temp_file.h"

TemporaryFileTest::TemporaryFileTest() = default;

TemporaryFileTest::~TemporaryFileTest() = default;

void TemporaryFileTest::SetUp() {
  ASSERT_TRUE(testing::CreateTempFile(&temp_name_));
  fd_ =
      testing::OpenTempFileFD(temp_name_, testing::FileAccessMode::kWriteOnly);
  ASSERT_GE(fd_, 0);
}

void TemporaryFileTest::TearDown() {
  testing::CloseFD(fd_);
  unlink(temp_name_.c_str());
}

void TemporaryFileTest::WriteAndClose(pdfium::span<const uint8_t> data) {
  if (!data.empty()) {
    EXPECT_EQ(fxcrt::spanwrite(data, fd_), data.size());
  }
  testing::CloseFD(fd_);
  fd_ = -1;
}
