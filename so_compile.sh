#!/usr/bin/env bash
SOURCE_C_FILE=`find . -name "*.c"`
SOURCE_CPP_FILE=`find . -name "*.cpp"`
if [ $SOURCE_CPP_FILE ];then
	COMPILER="g++"
else
	COMPILER="gcc"
fi
SOURCE_FILE=$SOURCE_C_FILE" $SOURCE_CPP_FILE"
MACRO="-D_REENTRANT"
COMPILE_OPTION="-Wno-deprecated -Wno-parentheses"
DEFAULT_LINK="-lpthread -lm -ldl"
TARGET="libutil_dynamic.so"
if [ `uname` = "Linux" ];then
	DEFAULT_LINK=$DEFAULT_LINK" -lrt -lcrypto -luuid"
fi

rm $TARGET 2>/dev/null
find ./ -type f -exec touch {} \;
echo $COMPILER is compiling......
$COMPILER -fPIC -shared $MACRO $COMPILE_OPTION $SOURCE_FILE -o $TARGET $DEFAULT_LINK
