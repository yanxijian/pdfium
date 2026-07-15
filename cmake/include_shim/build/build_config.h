// Copyright 2012 The Chromium Authors
// Minimal copy for PDFium CMake MVP builds (from Chromium build/build_config.h).

#ifndef BUILD_BUILD_CONFIG_H_
#define BUILD_BUILD_CONFIG_H_

#include "build/buildflag.h"

#if defined(ANDROID)
#define OS_ANDROID 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define OS_IOS 1
#else
#define OS_MAC 1
#endif
#elif defined(__linux__)
#define OS_LINUX 1
#elif defined(_WIN32)
#define OS_WIN 1
#elif defined(__FreeBSD__)
#define OS_FREEBSD 1
#elif defined(__NetBSD__)
#define OS_NETBSD 1
#elif defined(__OpenBSD__)
#define OS_OPENBSD 1
#else
#error Please add support for your platform in cmake/include_shim/build/build_config.h
#endif

#if defined(OS_MAC) || defined(OS_IOS)
#define OS_APPLE 1
#endif

#if defined(OS_FREEBSD) || defined(OS_NETBSD) || defined(OS_OPENBSD)
#define OS_BSD 1
#endif

#if defined(OS_ANDROID) || defined(OS_FREEBSD) || defined(OS_IOS) || \
    defined(OS_LINUX) || defined(OS_MAC) || defined(OS_NETBSD) ||   \
    defined(OS_OPENBSD)
#define OS_POSIX 1
#endif

#if defined(OS_ANDROID)
#define BUILDFLAG_INTERNAL_IS_ANDROID() (1)
#else
#define BUILDFLAG_INTERNAL_IS_ANDROID() (0)
#endif
#if defined(OS_APPLE)
#define BUILDFLAG_INTERNAL_IS_APPLE() (1)
#else
#define BUILDFLAG_INTERNAL_IS_APPLE() (0)
#endif
#if defined(OS_BSD)
#define BUILDFLAG_INTERNAL_IS_BSD() (1)
#else
#define BUILDFLAG_INTERNAL_IS_BSD() (0)
#endif
#define BUILDFLAG_INTERNAL_IS_CHROMEOS() (0)
#define BUILDFLAG_INTERNAL_IS_FUCHSIA() (0)
#if defined(OS_IOS)
#define BUILDFLAG_INTERNAL_IS_IOS() (1)
#else
#define BUILDFLAG_INTERNAL_IS_IOS() (0)
#endif
#define BUILDFLAG_INTERNAL_IS_IOS_MACCATALYST() (0)
#define BUILDFLAG_INTERNAL_IS_IOS_TVOS() (0)
#if defined(OS_LINUX)
#define BUILDFLAG_INTERNAL_IS_LINUX() (1)
#else
#define BUILDFLAG_INTERNAL_IS_LINUX() (0)
#endif
#if defined(OS_MAC)
#define BUILDFLAG_INTERNAL_IS_MAC() (1)
#else
#define BUILDFLAG_INTERNAL_IS_MAC() (0)
#endif
#if defined(OS_POSIX)
#define BUILDFLAG_INTERNAL_IS_POSIX() (1)
#else
#define BUILDFLAG_INTERNAL_IS_POSIX() (0)
#endif
#define BUILDFLAG_INTERNAL_IS_WATCHOS() (0)
#if defined(OS_WIN)
#define BUILDFLAG_INTERNAL_IS_WIN() (1)
#else
#define BUILDFLAG_INTERNAL_IS_WIN() (0)
#endif
#define BUILDFLAG_INTERNAL_IS_OZONE() (0)
#define BUILDFLAG_INTERNAL_IS_AIX() (0)
#define BUILDFLAG_INTERNAL_IS_ASMJS() (0)
#define BUILDFLAG_INTERNAL_IS_FREEBSD() (0)
#define BUILDFLAG_INTERNAL_IS_NETBSD() (0)
#define BUILDFLAG_INTERNAL_IS_OPENBSD() (0)
#define BUILDFLAG_INTERNAL_IS_QNX() (0)
#define BUILDFLAG_INTERNAL_IS_SOLARIS() (0)
#define BUILDFLAG_INTERNAL_ARCH_CPU_PTRAUTH() (0)

#if defined(__GNUC__)
#define COMPILER_GCC 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#else
#error Please add support for your compiler in cmake/include_shim/build/build_config.h
#endif

#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86_64 1
#define ARCH_CPU_64_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_CPU_ARM_FAMILY 1
#define ARCH_CPU_ARM64 1
#define ARCH_CPU_64_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(__ARMEL__)
#define ARCH_CPU_ARM_FAMILY 1
#define ARCH_CPU_ARMEL 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#else
#error Please add support for your architecture in cmake/include_shim/build/build_config.h
#endif

#if defined(OS_WIN)
#define WCHAR_T_IS_16_BIT
#elif defined(OS_POSIX)
#define WCHAR_T_IS_32_BIT
#endif

#endif  // BUILD_BUILD_CONFIG_H_
