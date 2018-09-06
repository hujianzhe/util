#!/usr/bin/env bash
OBJECT_FILE=`find . -name "*.o"`
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
TARGET="libutil.a"

rm $OBJECT_FILE 2>/dev/null
rm $TARGET 2>/dev/null
find ./ -type f -exec touch {} \;
echo $COMPILER is compiling......
$COMPILER -c $MACRO $COMPILE_OPTION $SOURCE_FILE
if [ "$?" != 0 ];then
	OBJECT_FILE=`find . -name "*.o"`
	rm $OBJECT_FILE 2>/dev/null
	exit
fi
OBJECT_FILE=`find . -name "*.o"`
ar rcs $TARGET $OBJECT_FILE
rm $OBJECT_FILE 2>/dev/null
