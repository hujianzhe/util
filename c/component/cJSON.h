/*
  Copyright (c) 2009 Dave Gamble
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
 
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef Dave_Gamble_cJSON__h
#define Dave_Gamble_cJSON__h

#include <stddef.h>

/* cJSON Types: */
typedef enum cJSON_Type {
	cJSON_Unknown,
	cJSON_Null,
	cJSON_Bool,
	cJSON_Number,
	cJSON_String,
	cJSON_Array,
	cJSON_Object
} cJSON_Type;

/* The cJSON structure: */
typedef struct cJSON {
	int type;					/* The type of the item, as above. */
	char *string;				/* The item's name string, if this item is the child of, or is in the list of subitems of an object. */

	char *valuestring;			/* The item's string, if type==cJSON_String */
	long long valueint;			/* The item's number, if type==cJSON_Number */
	double valuedouble;			/* The item's number, if type==cJSON_Number */

	int needfree;				/* The item will be free */
	int freestring;
	int freevaluestring;
	struct cJSON *parent;		/* An array or object item parent item */
	struct cJSON *next, *prev;	/* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
	struct cJSON *child;		/* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

#ifdef __cplusplus
	int valsize(void);
	cJSON* val(int index);
	cJSON* val(const char* name);

	void del(cJSON* item);
	void delval(const char* name);
	void delval(int index);

	cJSON* add(cJSON* item);
	cJSON* addarray(const char* name);
	cJSON* addobject(const char* name);
	cJSON* addval(const char* name);
	cJSON* addval(const char* name, bool b);
	cJSON* addval(const char* name, long long n);
	cJSON* addval(const char* name, double n);
	cJSON* addval(const char* name, const char* s);
#endif
} cJSON;

typedef struct cJSON_Hooks {
	void *(*malloc_fn)(size_t sz);
	void (*free_fn)(void *ptr);
} cJSON_Hooks;

#ifdef __cplusplus
extern "C"
{
#endif

/* Supply malloc, realloc and free functions to cJSON */
extern void cJSON_SetHooks(cJSON_Hooks* hooks);
extern cJSON_Hooks* cJSON_GetHooks(cJSON_Hooks* hooks);

/* Supply a block of JSON, and this returns a cJSON object you can interrogate. Call cJSON_Delete when finished. */
extern cJSON *cJSON_Parse(cJSON *root, const char *value);
/* Parse JSON from a file. */
extern cJSON *cJSON_ParseFromFile(cJSON *root, const char* path);
/* Render a cJSON entity to text for transfer/storage. Free the char* when finished. */
extern char  *cJSON_Print(cJSON *item);
/* Render a cJSON entity to text for transfer/storage without any formatting. Free the char* when finished. */
extern char  *cJSON_PrintUnformatted(cJSON *item);
/* Render a cJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
extern char *cJSON_PrintBuffered(cJSON *item,int prebuffer,int fmt);
/* Delete a cJSON text */
extern void cJSON_FreeString(char* s);
/* Delete a cJSON entity and all subentities. */
extern void   cJSON_Delete(cJSON *c);
/* Delete all subentities */
extern void cJSON_Reset(cJSON *c);

/* Returns the number of items in an array (or object). */
extern int	  cJSON_Size(cJSON *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
extern cJSON *cJSON_Index(cJSON *array,int item);
/* Get item "string" from object. Case insensitive. */
extern cJSON *cJSON_Field(cJSON *object,const char *string);

/* These calls create a cJSON item of the appropriate type. */
extern cJSON *cJSON_NewNull(const char* name);
extern cJSON *cJSON_NewBool(const char* name, int b);
extern cJSON *cJSON_NewNumber(const char* name, double num);
extern cJSON *cJSON_NewString(const char* name, const char *string);
extern cJSON *cJSON_NewArray(const char* name);
extern cJSON *cJSON_NewObject(const char* name);

/* Append item to the specified array/object. */
extern cJSON* cJSON_Add(cJSON *array, cJSON *item);
#define cJSON_AddNewNull(object,name)				cJSON_Add(object, cJSON_NewNull(name))
#define cJSON_AddNewBool(object,name,b)				cJSON_Add(object, cJSON_NewBool(name,b))
#define cJSON_AddNewNumber(object,name,n)			cJSON_Add(object, cJSON_NewNumber(name,n))
#define cJSON_AddNewString(object,name,s)			cJSON_Add(object, cJSON_NewString(name,s))
#define	cJSON_AddNewArray(object,name)				cJSON_Add(object, cJSON_NewArray(name))
#define	cJSON_AddNewObject(object,name)				cJSON_Add(object, cJSON_NewObject(name))

/* Remove/Detatch items from Arrays/Objects. */
extern cJSON* cJSON_Detach(cJSON* item);
#define	cJSON_DetachDelete(item)					cJSON_Delete(cJSON_Detach(item))
#define	cJSON_DetachReset(item)						cJSON_Reset(cJSON_Detach(item))

/* Duplicate a cJSON item */
extern cJSON *cJSON_Duplicate(cJSON *item,int recurse);
extern void cJSON_Minify(char *json);

#ifdef __cplusplus
}
#endif

#endif
