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

#ifdef __cplusplus
extern "C"
{
#endif

/* cJSON Types: */
typedef enum cJSON_Type {
	cJSON_Unknown,
	cJSON_False,
	cJSON_True,
	cJSON_NULL,
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
} cJSON;

typedef struct cJSON_Hooks {
	void *(*malloc_fn)(size_t sz);
	void (*free_fn)(void *ptr);
} cJSON_Hooks;

/* Supply malloc, realloc and free functions to cJSON */
extern void cJSON_InitHooks(cJSON_Hooks* hooks);
extern cJSON_Hooks* cJSON_GetHooks(cJSON_Hooks* hooks);

/* Supply a block of JSON, and this returns a cJSON object you can interrogate. Call cJSON_Delete when finished. */
extern cJSON *cJSON_Parse(const char *value);
/* Parse JSON from a file. */
extern cJSON *cJSON_ParseFromFile(const char* path);
/* Render a cJSON entity to text for transfer/storage. Free the char* when finished. */
extern char  *cJSON_Print(cJSON *item);
/* Render a cJSON entity to text for transfer/storage without any formatting. Free the char* when finished. */
extern char  *cJSON_PrintUnformatted(cJSON *item);
/* Render a cJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
extern char *cJSON_PrintBuffered(cJSON *item,int prebuffer,int fmt);
/* Delete a cJSON entity and all subentities. */
extern void   cJSON_Delete(cJSON *c);
/* Delete all subentities */
extern void cJSON_DeleteSub(cJSON *c);

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

/* These utilities create an Array of count items. */
/*
extern cJSON *cJSON_CreateIntArray(const int *numbers,int count);
extern cJSON *cJSON_CreateFloatArray(const float *numbers,int count);
extern cJSON *cJSON_CreateDoubleArray(const double *numbers,int count);
extern cJSON *cJSON_CreateStringArray(const char **strings,int count);
*/

/* Append item to the specified array/object. */
extern cJSON* cJSON_Add(cJSON *array, cJSON *item);

/* Remove/Detatch items from Arrays/Objects. */
extern cJSON* cJSON_Detach(cJSON* item);
extern cJSON* cJSON_IndexDetach(cJSON *array,int which);
extern cJSON* cJSON_FieldDetach(cJSON *object,const char *string);
#define	cJSON_ObjectDelete(object,name)				cJSON_Delete(cJSON_FieldDetach(object,name))
#define	cJSON_ArrayDelete(array,i)					cJSON_Delete(cJSON_IndexDetach(array,i))

/* Update array items. */
/*
extern void cJSON_InsertItemInArray(cJSON *array,int which,cJSON *newitem);
extern void cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem);
extern void cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem);
*/

/* Duplicate a cJSON item */
extern cJSON *cJSON_Duplicate(cJSON *item,int recurse);
/* Duplicate will create a new, identical cJSON item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */

extern void cJSON_Minify(char *json);

/* Macros for creating things quickly. */
#define cJSON_ObjectAddNull(object,name)			cJSON_Add(object, cJSON_NewNull(name))
#define cJSON_ObjectAddBool(object,name,b)			cJSON_Add(object, cJSON_NewBool(name,b))
#define cJSON_ObjectAddNumber(object,name,n)		cJSON_Add(object, cJSON_NewNumber(name,n))
#define cJSON_ObjectAddString(object,name,s)		cJSON_Add(object, cJSON_NewString(name,s))
#define	cJSON_ObjectAddArray(object,name)			cJSON_Add(object, cJSON_NewArray(name))
#define	cJSON_ObjectAddObject(object,name)			cJSON_Add(object, cJSON_NewObject(name))

#define cJSON_ArrayAddNull(array)					cJSON_Add(array, cJSON_NewNull(NULL))
#define cJSON_ArrayAddBool(array,b)					cJSON_Add(array, cJSON_NewBool(NULL,b))
#define cJSON_ArrayAddNumber(array,n)				cJSON_Add(array, cJSON_NewNumber(NULL,n))
#define cJSON_ArrayAddString(array,s)				cJSON_Add(array, cJSON_NewString(NULL,s))
#define	cJSON_ArrayAddArray(array)					cJSON_Add(array, cJSON_NewArray(NULL))
#define	cJSON_ArrayAddObject(array)					cJSON_Add(array, cJSON_NewObject(NULL))

#ifdef __cplusplus
}
#endif

#endif
