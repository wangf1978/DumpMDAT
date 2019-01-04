NDK_TOOLCHAIN_VERSION := clang
APP_ABI := armeabi-v7a arm64-v8a x86
APP_CPPFLAGS := -std=c++11 -fexceptions -fno-permissive
APP_STL := c++_static
APP_CFLAGS += -I. -I./LibPlatform
APP_CPPFLAGS += -Wall -Wno-switch-enum -Wno-switch -Wno-multichar -Wno-shift-overflow -Wno-unused-variable -D__int64="long long" -DNO_ASM
APP_LDFLAGS += -Wl,-Bsymbolic