SOURCE_FILE += $(shell find -name "*.c" && find -name "*.cpp")
OBJECT_O += $(shell find -name "*.o")
OBJECT_GCH += $(shell find -name "*.gch")

TARGET := libutil
COMPILE_OPTION := -std=c++11 -Wno-deprecated
DEFAULT_LINK := -lpthread -lm -ldl
ifeq ($(shell uname), Linux)
DEFAULT_LINK += -lrt -lcrypto -luuid
endif

all:

time:
	find ./ -type f -exec touch {} \;

clean:
	-rm $(OBJECT_O) $(OBJECT_GCH)

compile:
	g++ -c -D_REENTRANT $(COMPILE_OPTION) $(SOURCE_FILE)

static:
	-rm $(TARGET).a
	ar rcs $(TARGET).a $(OBJECT_O)
	-rm $(OBJECT_O) $(OBJECT_GCH)

dll:
	-rm $(TARGET).so
	g++ -DNDEBUG -shared -fPIC $(COMPILE_OPTION) $(SOURCE_FILE) -o $(TARGET).so $(DEFAULT_LINK)
