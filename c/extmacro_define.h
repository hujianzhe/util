//
// Created by hujianzhe
//

#ifndef UTIL_C_EXTMACRO_DEFINE_H
#define	UTIL_C_EXTMACRO_DEFINE_H

#define	field_offset(type, field)				((char*)(&((type *)0)->field) - (char*)(0))
#define field_container(address, type, field)	((type *)((char*)(address) - (char*)(&((type *)0)->field)))

#endif