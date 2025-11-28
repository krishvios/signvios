ifdef CHILD_DIRECTORIES
DEPENDENTS += children
CHILDREN = children
endif

ifdef TARGET
DEPENDENTS += $(BUILD_DIR)/$(TARGET) Makefile
endif

ifdef EXECUTABLE
DEPENDENTS += $(BUILD_DIR)/$(EXECUTABLE)
endif

all: $(DEPENDENTS)
		
$(BUILD_DIR)/$(TARGET): $(addprefix $(BUILD_DIR)/,$(OBJS)) $(addprefix $(BUILD_DIR)/,$(addsuffix .clang.out,$(basename $(OBJS)))) $(CHILDREN) $(EXECUTABLE_DEPENDENTS)
ifdef OBJS
	$(AR) rcv $@ $(addprefix $(BUILD_DIR)/,$(OBJS))
endif

$(BUILD_DIR)/$(EXECUTABLE): $(addprefix $(BUILD_DIR)/,$(OBJS)) $(CHILDREN) $(EXECUTABLE_DEPENDENTS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(DEFS) $(INCS) $(LIBDIRS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: %.c | $(CHILDREN) $(PRIORITY_TARGETS)
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEFS) $(INCS) -c -o $@ $<

$(BUILD_DIR)/%.clang.out: %.c | $(CHILDREN) $(PRIORITY_TARGETS)
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi

$(BUILD_DIR)/%.o: %.cpp | $(CHILDREN) $(PRIORITY_TARGETS)
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEFS) $(INCS) -c -o $@ $<

$(BUILD_DIR)/%.clang.out: %.cpp | $(CHILDREN) $(PRIORITY_TARGETS)
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
ifeq ($(USE_CLANG_TIDY), true)
	clang-tidy-9 -config="" $< -- $(CPPFLAGS) $(CXXFLAGS) $(DEFS) $(INCS) | tee $@
else
	@if [ -e $@ ]; then rm $@; fi
endif


.PHONY: children $(CHILD_DIRECTORIES)
children: $(CHILD_DIRECTORIES)

$(CHILD_DIRECTORIES):
	$(MAKE) -C $@

clean:
	@$(foreach CHILD, $(CHILD_DIRECTORIES), $(MAKE) -C $(CHILD) clean|| exit 1;)
	rm -rf $(BUILD_DIR) $(PRIORITY_TARGETS)

install:
	@if [ ! -d $(DISTRIBUTION_FOLDER) ]; then mkdir -p $(DISTRIBUTION_FOLDER); fi
	@$(foreach CHILD, $(CHILD_DIRECTORIES), $(MAKE) -C $(CHILD) install || exit 1;)
	@$(foreach OBJ, $(OBJS), if [  ! -e $(DISTRIBUTION_FOLDER)/$(OBJ) -o $(BUILD_DIR)/$(OBJ) -nt $(DISTRIBUTION_FOLDER)/$(OBJ) ]; then { cp -f $(BUILD_DIR)/$(OBJ) $(DISTRIBUTION_FOLDER); } fi || exit 1;)
	@$(foreach FILE, $(INSTALL_FILES), if [ ! -e $(DISTRIBUTION_FOLDER)/$(notdir $(FILE)) -o $(FILE) -nt $(DISTRIBUTION_FOLDER)/$(notdir $(FILE)) ]; then { cp -f $(FILE) $(DISTRIBUTION_FOLDER); } fi || exit 1;)
	
document:
	@if [ ! -d $(DOC_FOLDER) ]; then mkdir -p $(DOC_FOLDER); fi
	@$(foreach CHILD, $(CHILD_DIRECTORIES), $(MAKE) -C $(CHILD) document || exit 1;)
	@$(foreach FILE, $(DOC_FILES), if [ ! -e $(DOC_FOLDER)/$(FILE) -o $(FILE) -nt $(DOC_FOLDER)/$(FILE) ]; then { cp -f $(FILE) $(DOC_FOLDER); } fi || exit 1;)

ifdef OBJS
-include $(addprefix $(BUILD_DIR)/,$(OBJS:.o=.d))
endif
