//
// Created by hujianzhe on 17-11-6.
//

#ifndef UTIL_CPP_LIBPROTOBUF_LIBPROTOBUF_IMPORT_LIB_H
#define	UTIL_CPP_LIBPROTOBUF_LIBPROTOBUF_IMPORT_LIB_H

#ifdef PROTOCOL_ENABLE_PROTOBUF
#if defined(_WIN64)
#ifdef _DEBUG
#pragma comment(lib, "util/cpp/libprotobuf/2.6.1/win64-debug/libprotobuf.lib")
#else
#pragma comment(lib, "util/cpp/libprotobuf/2.6.1/win64-release/libprotobuf.lib")
#endif
#elif defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "util/cpp/libprotobuf/2.6.1/win32-debug/libprotobuf.lib")
#else
#pragma comment(lib, "util/cpp/libprotobuf/2.6.1/win32-release/libprotobuf.lib")
#endif
#endif
#endif // PROTOCOL_ENABLE_PROTOBUF

#endif