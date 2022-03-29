#!/usr/bin/env bash
OBJECT_FILE=`find . -name "*.o"`
SOURCE_C_FILE=`find . -name "*.c"`
SOURCE_CPP_FILE=`find . -name "*.cpp"`
MACRO="-D_REENTRANT"
COMPILE_OPTION="-Wno-deprecated -Wno-parentheses -pthread"

COMPILER="gcc"
if [ $COMPILER == "gcc" ];then
	SOURCE_CPP_FILE=""
fi
if [ ! -n "$1" ];then
	echo "no spec build mode"
	exit
elif [ $1 == "debug" ];then
	MACRO="-D_DEBUG $MACRO"
	COMPILE_OPTION="-g $COMPILE_OPTION"
	TARGET="libUtilStaticDebug.a"
elif [ $1 == "release" ];then
	MACRO="-DNDEBUG $MACRO"
	COMPILE_OPTION="-O1 $COMPILE_OPTION"
	TARGET="libUtilStatic.a"
else
	echo "no spec build mode"
	exit
fi

rm $OBJECT_FILE 2>/dev/null
rm $TARGET 2>/dev/null
#find ./ -type f -exec touch {} \;
echo "$COMPILER -c $MACRO $COMPILE_OPTION" $SOURCE_C_FILE $SOURCE_CPP_FILE 
$COMPILER -c $MACRO $COMPILE_OPTION $SOURCE_C_FILE $SOURCE_CPP_FILE
if [ "$?" != 0 ];then
	OBJECT_FILE=`find . -name "*.o"`
	rm $OBJECT_FILE 2>/dev/null
	exit
fi
OBJECT_FILE=`find . -name "*.o"`
echo "ar rcs $TARGET" $OBJECT_FILE
ar rcs $TARGET $OBJECT_FILE
rm $OBJECT_FILE 2>/dev/null
