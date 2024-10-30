SOURCE_C_FILE += $(shell find . -name "*.c")
SOURCE_CPP_FILE += $(shell find . -name "*.cpp")

TARGET_PATH += .
COMPILE_OPTION := -fPIC -shared -fvisibility=hidden -Wno-deprecated -Wno-parentheses -Wno-unused-result -Wreturn-type
MACRO := -D_REENTRANT -DDECLSPEC_DLL_EXPORT

DEFAULT_LINK := -pthread -lm -ldl
ifeq ($(shell uname), Linux)
DEFAULT_LINK += -lrt
endif

COMPILER := gcc
ifeq ($(COMPILER), gcc)
SOURCE_CPP_FILE :=
endif

DEBUG_TARGET := $(TARGET_PATH)/libUtilDynamicDebug.so
ASAN_TARGET := $(TARGET_PATH)/libUtilDynamicAsan.so
RELEASE_TARGET := $(TARGET_PATH)/libUtilDynamic.so

all:

debug:
	$(COMPILER) $(MACRO) -D_DEBUG -g $(COMPILE_OPTION) $(SOURCE_C_FILE) $(SOURCE_CPP_FILE) -o $(DEBUG_TARGET) $(DEFAULT_LINK)

asan:
	$(COMPILER) $(MACRO) -D_DEBUG -g -fsanitize=address $(COMPILE_OPTION) $(SOURCE_C_FILE) $(SOURCE_CPP_FILE) -o $(ASAN_TARGET) $(DEFAULT_LINK)

release:
	$(COMPILER) $(MACRO) -DNDEBUG -O2 $(COMPILE_OPTION) $(SOURCE_C_FILE) $(SOURCE_CPP_FILE) -o $(RELEASE_TARGET) $(DEFAULT_LINK)
