//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_VAR_TYPE_H
#define	UTIL_C_DATASTRUCT_VAR_TYPE_H

typedef	union var_t {
	signed char c, i8;
	unsigned char byte, u8;
	short i16;
	unsigned short u16;
	int i32;
	unsigned int u32;
	long long i64;
	unsigned long long u64;
	float f32;
	double f64;

	void* p_void;
	char* str;
	char* p_char;
	signed char* p_i8;
	unsigned char* p_byte, *p_u8;
	short* p_i16;
	unsigned short* p_u16;
	int* p_i32;
	unsigned int* p_u32;
	long long* p_i64;
	unsigned long long* p_u64;
	float* p_f32;
	double* p_f64;
} var_t;

#endif // !UTIL_C_VAR_TYPE_H
