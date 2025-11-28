SHELL := /bin/bash

MAKEFILE_DIR := $(shell pwd)

HEADERS := 

LIBS := 

OBJS := $(wildcard $(BUILD_DIR)/*.o)

# Not defined for Apple.
ifeq ($(PLATFORM), APPLE)
	RANLIB := ranlib
endif

# Apple builds libupnp outside of this makefile.
ifeq ($(PLATFORM), LINUX)
ifneq ($(APPLICATION), APP_REGISTRAR_LOAD_TEST_TOOL)

	HEADERS += \
		$(BUILD_DIR)/ixml.h
	LIBS += $(UPNP_BUILD_DIR)/ixml/.libs/libixml.a \
		$(UPNP_BUILD_DIR)/upnp/.libs/libupnp.a \
		$(UPNP_BUILD_DIR)/threadutil/.libs/libthreadutil.a
endif
endif

FINAL_LIB := $(shell pwd)/$(BUILD_DIR)/libVideophoneEngine.a

all: $(BUILD_DIR) $(HEADERS) $(FINAL_LIB)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(FINAL_LIB): $(LIBS) $(OBJS)
	@echo Creating VideophoneEngine library...
	@pushd $(BUILD_DIR); \
	for i in $(LIBS); do $(AR) x "$${i}"; done; \
	$(AR) cr $(FINAL_LIB) *.o; \
	$(RANLIB) $(FINAL_LIB); \
	popd

clean:
	rm -fr $(BUILD_DIR)


$(BUILD_DIR)/ixml.h: $(IXML_DIR)/inc/ixml.h
	cp -f $? $@

