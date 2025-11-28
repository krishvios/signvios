LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp
LOCAL_MODULE    := mpeg4_atoms
LOCAL_SRC_FILES := Atom.cpp \
			CttsAtom.cpp \
			DinfAtom.cpp \
			DrefAtom.cpp \
			ElstAtom.cpp \
			FtypAtom.cpp \
			FullAtom.cpp \
			HdlrAtom.cpp \
			HmhdAtom.cpp \
			MdatAtom.cpp \
			MdhdAtom.cpp \
			MdiaAtom.cpp \
			MinfAtom.cpp \
			MoovAtom.cpp \
			MvhdAtom.cpp \
			NmhdAtom.cpp \
			SmhdAtom.cpp \
			StblAtom.cpp \
			StcoAtom.cpp \
			StscAtom.cpp \
			StsdAtom.cpp \
			StssAtom.cpp \
			StszAtom.cpp \
			SttsAtom.cpp \
			TkhdAtom.cpp \
			TrakAtom.cpp \
			TrefAtom.cpp \
			UdtaAtom.cpp \
			UnknownAtom.cpp \
			UrlAtom.cpp \
			UrnAtom.cpp \
			VmhdAtom.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../Atoms

LOCAL_STATIC_LIBRARIES := http \
			os \
			vpcommon \
			mpeg4 \
			mpeg4_toolbox

include $(BUILD_STATIC_LIBRARY)
