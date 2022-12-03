SOURCE_C_FILE += $(shell find . -name "*.c")
SOURCE_CPP_FILE += $(shell find . -name "*.cpp")

TARGET_PATH += .
COMPILE_OPTION := -fPIC -shared -Wno-deprecated -Wno-parentheses
MACRO := -D_REENTRANT
#ASAN := -fsanitize=address

DEFAULT_LINK := -pthread -lm -ldl
ifeq ($(shell uname), Linux)
DEFAULT_LINK += -lrt
endif

COMPILER := gcc
ifeq ($(COMPILER), gcc)
SOURCE_CPP_FILE :=
endif

DEBUG_TARGET := $(TARGET_PATH)/libUtilDynamicDebug.so
RELEASE_TARGET := $(TARGET_PATH)/libUtilDynamic.so

all:

debug:
	$(COMPILER) $(MACRO) -D_DEBUG -g $(ASAN) $(COMPILE_OPTION) $(SOURCE_C_FILE) $(SOURCE_CPP_FILE) -o $(DEBUG_TARGET) $(DEFAULT_LINK)

release:
	$(COMPILER) $(MACRO) -DNDEBUG -O1 $(COMPILE_OPTION) $(SOURCE_C_FILE) $(SOURCE_CPP_FILE) -o $(RELEASE_TARGET) $(DEFAULT_LINK)
