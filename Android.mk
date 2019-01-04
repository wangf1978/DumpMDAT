LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := mp4rawdatafix
LOCAL_SRC_FILES := \
	aacsyntax.cpp \
       	AMRingBuffer.cpp \
	Bitstream.cpp  \
	DumpAV.cpp  \
	DumpMDAT.cpp  \
	huffman.cpp  \
	StdAfx.cpp

include $(BUILD_SHARED_LIBRARY)
