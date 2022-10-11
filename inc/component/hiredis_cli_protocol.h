/*
 * Copyright (c) 2009-2011, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2011, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* All Code is from HIREDIS
 * https://github.com/redis/hiredis
 *
 * hiredis version:
 *
#define HIREDIS_MAJOR 1
#define HIREDIS_MINOR 0
#define HIREDIS_PATCH 3
#define HIREDIS_SONAME 1.0.3-dev
 */

#ifndef __HIREDIS_CLI_PROTOCOL_H
#define __HIREDIS_CLI_PROTOCOL_H

#ifdef _MSC_VER
	#ifndef	__declspec_dll
		#ifdef	DECLSPEC_DLL_EXPORT
			#define	__declspec_dll						__declspec(dllexport)
		#elif	DECLSPEC_DLL_IMPORT
			#define	__declspec_dll						__declspec(dllimport)
		#else
			#define	__declspec_dll
		#endif
	#endif
#else
	#ifndef	__declspec_dll
		#define	__declspec_dll
	#endif
#endif

#include <stddef.h>
#include <stdarg.h>

#define REDIS_ERR -1
#define REDIS_OK 0

/* When an error occurs, the err flag in a context is set to hold the type of
 * error that occurred. REDIS_ERR_IO means there was an I/O error and you
 * should use the "errno" variable to find out what is wrong.
 * For other values, the "errstr" field will hold a description. */
#define REDIS_ERR_IO 1 /* Error in read or write */
#define REDIS_ERR_EOF 3 /* End of file */
#define REDIS_ERR_PROTOCOL 4 /* Protocol error */
#define REDIS_ERR_OOM 5 /* Out of memory */
#define REDIS_ERR_TIMEOUT 6 /* Timed out */
#define REDIS_ERR_OTHER 2 /* Everything else... */

#define REDIS_REPLY_STRING 1
#define REDIS_REPLY_ARRAY 2
#define REDIS_REPLY_INTEGER 3
#define REDIS_REPLY_NIL 4
#define REDIS_REPLY_STATUS 5
#define REDIS_REPLY_ERROR 6
#define REDIS_REPLY_DOUBLE 7
#define REDIS_REPLY_BOOL 8
#define REDIS_REPLY_MAP 9
#define REDIS_REPLY_SET 10
#define REDIS_REPLY_ATTR 11
#define REDIS_REPLY_PUSH 12
#define REDIS_REPLY_BIGNUM 13
#define REDIS_REPLY_VERB 14

/* Structure pointing to our actually configured allocators */
typedef struct RedisProtocolAllocFuncs_t {
    void *(*mallocFn)(size_t);
    void *(*callocFn)(size_t,size_t);
    void *(*reallocFn)(void*,size_t);
    char *(*strdupFn)(const char*);
    void (*freeFn)(void*);
} RedisProtocolAllocFuncs_t;

/* This is the reply object */
typedef struct RedisReply_t {
    int type; /* REDIS_REPLY_* */
    long long integer; /* The integer when type is REDIS_REPLY_INTEGER */
    double dval; /* The double when type is REDIS_REPLY_DOUBLE */
    size_t len; /* Length of string */
    char *str; /* Used for REDIS_REPLY_ERROR, REDIS_REPLY_STRING
                  REDIS_REPLY_VERB, REDIS_REPLY_DOUBLE (in additional to dval),
                  and REDIS_REPLY_BIGNUM. */
    char vtype[4]; /* Used for REDIS_REPLY_VERB, contains the null
                      terminated 3 character content type, such as "txt". */
    size_t elements; /* number of elements, for REDIS_REPLY_ARRAY */
    struct RedisReply_t **element; /* elements vector for REDIS_REPLY_ARRAY */
} RedisReply_t;

struct RedisReplyReadTask_t; /* note: hide detail */
struct RedisReplyObjectFunctions_t; /* note: hide detail */

typedef struct RedisReplyReader_t {
    int err; /* Error flags, 0 when there is no error */
    char errstr[128]; /* String representation of error when applicable */

    char *buf; /* Read buffer */
    size_t pos; /* Buffer cursor */
    size_t len; /* Buffer length */
    size_t maxbuf; /* Max length of unused buffer */
    long long maxelements; /* Max multi-bulk elements */

    struct RedisReplyReadTask_t **task;
    int tasks;

    int ridx; /* Index of current read task */
    void *reply; /* Temporary reply pointer *//* default is struct RedisReply_t */

    const struct RedisReplyObjectFunctions_t *fn;
    void *privdata;
} RedisReplyReader_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll RedisProtocolAllocFuncs_t RedisProtocolAllocFuncs_set(const RedisProtocolAllocFuncs_t *ha);
__declspec_dll void RedisProtocolAllocFuncs_reset(void);

__declspec_dll RedisReplyReader_t *RedisReplyReader_create(void);
__declspec_dll void RedisReplyReader_free(RedisReplyReader_t *r);
__declspec_dll int RedisReplyReader_feed(RedisReplyReader_t *r, const char *buf, size_t len);
__declspec_dll int RedisReplyReader_pop_reply(RedisReplyReader_t *r, RedisReply_t **reply);
__declspec_dll void RedisReply_free(RedisReply_t *reply);

__declspec_dll int RedisCommand_vformat(char **target, const char *format, va_list ap);
__declspec_dll int RedisCommand_format(char **target, const char *format, ...);
__declspec_dll size_t RedisCommand_format_argv(char **target, int argc, const char **argv, const size_t *argvlen);
__declspec_dll void RedisCommand_free(char *target);

#ifdef __cplusplus
}
#endif

#endif
