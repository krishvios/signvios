TARGET := libstiHttp.a

DEFS := 

INCS := \
	-I$(HTTP_DIR) \
	-I$(OS_DIR) \
	-I$(COMMON_DIR) 

INSTALL_FILES := \
	CstiHTTPService.h \
	stiHTTPServiceTools.h \
	core_client.pem \
	CAfile.crt \
	amazon-combined.pem \
	key.pem \
	gdroot-g2.crt \
	gdig2.crt.pem \
	TrustedRoot-DigiCertCA.pem

OBJS := \
	CstiHTTPService.o \
	CstiURLResolve.o \
	CstiHTTPTask.o \
	stiSSLTools.o \
	stiHTTPServiceTools.o \
	SslCertificateHandler.o

ifneq (,$(findstring clang,$(CXX)))
	CXXFLAGS += \
		-Wno-overloaded-virtual
endif

include $(MAKEFILE_RULES)

