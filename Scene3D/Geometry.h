/**********************************************************************************
// Geometry (Arquivo de Cabeçalho)
//
// Criação:     03 Fev 2013
// Atualização: 09 Jul 2025
// Compilador:  Visual C++ 2022
//
// Descrição:   Define vértices e índices para várias geometrias
//
**********************************************************************************/

#ifndef DXUT_GEOMETRY_H_
#define DXUT_GEOMETRY_H_

// -------------------------------------------------------------------------------

#include "Types.h"
#include "Colors.h"
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <vector>
using std::vector;
using namespace DirectX;

// -------------------------------------------------------------------------------

struct Vertex
{
    XMFLOAT3 pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT4 color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    XMFLOAT3 normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT2 texc = XMFLOAT2(0.0f, 0.0f);
};

// -------------------------------------------------------------------------------
// Geometry
// -------------------------------------------------------------------------------

class Geometry
{
protected:
    void Subdivide();                       // subdivide triângulos
    void SetNormals();                   // ajusta normais

public:
    vector<Vertex> vertices;                // vértices da geometria
    vector<uint>   indices;                 // índices da geometria

    // métodos inline
    const Vertex* VertexData() const        // retorna vértices da geometria
    { return vertices.data(); }

    const uint* IndexData() const           // retorna índices da geometria
    { return indices.data(); }

    uint VertexCount() const                // retorna número de vértices
    { return uint(vertices.size()); }

    uint IndexCount() const                 // retorna número de índices
    { return uint(indices.size()); }
};

// -------------------------------------------------------------------------------
// Box
// -------------------------------------------------------------------------------

struct Box : public Geometry
{
    Box(float width,
        float height,
        float depth,
        XMFLOAT4 color = Wood);
};

// -------------------------------------------------------------------------------
// Cylinder
// -------------------------------------------------------------------------------

struct Cylinder : public Geometry
{
    Cylinder(float bottom,
        float top,
        float height,
        uint sliceCount,
        uint stackCount,
        XMFLOAT4 color = Yellow);
};

// -------------------------------------------------------------------------------
// Sphere
// -------------------------------------------------------------------------------

struct Sphere : public Geometry
{
    Sphere(float radius,
        uint sliceCount,
        uint stackCount,
        XMFLOAT4 color = Crimson);
};

// -------------------------------------------------------------------------------
// GeoSphere
// -------------------------------------------------------------------------------

struct GeoSphere : public Geometry
{
    GeoSphere(float radius,
        uint subdivisions,
        XMFLOAT4 color = Orange);
};


// -------------------------------------------------------------------------------
// Grid
// -------------------------------------------------------------------------------

struct Grid : public Geometry
{
    Grid(float width,
        float depth,
        uint m,
        uint n,
        XMFLOAT4 color = Gray);
};

// -------------------------------------------------------------------------------
// Quad
// -------------------------------------------------------------------------------

struct Quad : public Geometry
{
    Quad(float width,
        float height,
        XMFLOAT4 color = Green);
};

// -------------------------------------------------------------------------------

#endif
