SOURCE_FILE += $(shell find -name "*.h" && find -name "*.c" && find -name "*.cpp")
OBJECT_O += $(shell find -name "*.o")
OBJECT_GCH += $(shell find -name "*.gch")

COMPILE_OPTION := -c -std=c++11
TARGET := libutil.a

all:
clean:
	-rm $(OBJECT_O) $(OBJECT_GCH)
compile:
	g++ $(COMPILE_OPTION) $(SOURCE_FILE)
static:
	-rm $(TARGET)
	ar rcs $(TARGET) $(OBJECT_O)
	-rm $(OBJECT_O) $(OBJECT_GCH)
