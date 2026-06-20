/**********************************************************************************
// Mesh (Arquivo de Cabeçalho)
//
// Criação:     28 Abr 2016
// Atualização: 08 Mar 2025
// Compilador:  Visual C++ 2022
//
// Descrição:   Representa uma malha 3D
//
**********************************************************************************/

#ifndef DXUT_MESH_H_
#define DXUT_MESH_H_

// -------------------------------------------------------------------------------

#include "Types.h"
#include "Graphics.h"
#include "Geometry.h"

// -------------------------------------------------------------------------------

struct Mesh
{
    uint indexCount = 0;
    uint vertexCount = 0;
    uint startIndex = 0;
    uint baseVertex = 0;
    uint cbufferIndex = -1;
    D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    
    Mesh() {}
    Mesh(const Geometry & geo) 
        : indexCount(geo.IndexCount())
        , vertexCount(geo.VertexCount()) {}
};

// -------------------------------------------------------------------------------

#endif

