//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_MESH_H
#define	UTIL_C_CRT_GEOMETRY_MESH_H

#include "geometry_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll GeometryMesh_t* mathMeshCooking(const float (*v)[3], unsigned int v_cnt, const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryMesh_t* mesh);
__declspec_dll GeometryMesh_t* mathMeshDeepCopy(GeometryMesh_t* dst, const GeometryMesh_t* src);
__declspec_dll void mathMeshFreeCookingData(GeometryMesh_t* mesh);
__declspec_dll int mathMeshIsClosed(const GeometryMesh_t* mesh);
__declspec_dll int mathMeshIsConvex(const GeometryMesh_t* mesh);
__declspec_dll void mathConvexMeshMakeFacesOut(GeometryMesh_t* mesh);
__declspec_dll int mathConvexMeshHasPoint(const GeometryMesh_t* mesh, const float p[3]);
__declspec_dll int mathConvexMeshContainConvexMesh(const GeometryMesh_t* mesh1, const GeometryMesh_t* mesh2);

#ifdef	__cplusplus
}
#endif

#endif
