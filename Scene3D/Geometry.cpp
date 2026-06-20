/**********************************************************************************
// Geometry (Código Fonte)
//
// Criação:     03 Fev 2013
// Atualização: 12 Jul 2025
// Compilador:  Visual C++ 2022
//
// Descrição:   Define vértices e índices para várias geometrias
//
**********************************************************************************/

#include <Windows.h>
#include "Geometry.h"
#include <unordered_map>
#include <sstream>
#include <string>
using std::pair;
using std::unordered_map;
using std::stringstream;
using std::string;

//   __________   _____   __________   ________   ___________   ______   ______
// _/ Geometry \_/ Box \_/ Cylinder \_/ Sphere \_/ GeoSphere \_/ Grid \_/ Quad \_
// ------------------------------------------------------------------------------

//   __________
// _/ Geometry \_________________________________________________________________
// ------------------------------------------------------------------------------

pair<uint,uint> AddEdge(uint a, uint b, const vector<Vertex> & base, vector <Vertex> & vec, uint* edges, uint* verts)
{
    Vertex va = base[a];
    Vertex vb = base[b];

    // posição será alterada para permitir tratar 0
    // como uma posição vazia na matriz
    a = a + 1;
    b = b + 1;

    // se aresta não foi processada antes
    if (edges[a * 10243 + b] == 0)
    {
        // acha a posição do meio da aresta
        Vertex m;
        XMStoreFloat3(&m.pos, 0.5f * (XMLoadFloat3(&va.pos) + XMLoadFloat3(&vb.pos)));

        // verifica se o vértice "va" já foi inserido no buffer
        uint pos1, pos2;
        if (verts[a] == 0)
        {
            // se não foi ainda, então insere
            vec.push_back(va);

            // pega posição do vértice no novo buffer
            pos1 = uint(vec.size() - 1);

            // registra adição de vértice
            verts[a] = pos1 + 1;
        }
        else
        {
            // se já foi inserido, recupera posição armazenada
            pos1 = verts[a] - 1;
        }

        // insere novo vértice "m" no buffer
        vec.push_back(m);
        pos2 = uint(vec.size() - 1);

        // registra aresta processada
        // guarda junto a posição do vértice do meio
        edges[a * 10243 + b] = pos2 + 1;
        edges[b * 10243 + a] = pos2 + 1;

        // retorna posições dos vértices va e m 
        return { pos1, pos2 };
    }
    else
    {
        // se a aresta já foi processada, va e m já estão no buffer
        uint pos1, pos2;
        pos1 = verts[a] - 1;
        pos2 = edges[a * 10243 + b] - 1;

        // apenas retorna suas posições
        return { pos1,pos2 };
    }
}

void Geometry::Subdivide()
{
    // Quantidade de vértices, índices e triângulos
    // para cada valor de subdivisão:
    // 
    // 0 - 12 vértices - 60 índices - 20 triângulos
    // 1 - 42 vértices - 240 índices - 80 triângulos
    // 2 - 162 vértices - 960 índices - 320 triângulos
    // 3 - 642 vértices - 3840 índices - 1280 triângulos
    // 4 - 2562 vértices - 15360 índices - 5120 triângulos
    // 5 - 10242 vértice - 61440 índices - 20480 triângulos
    // 6 - 40962 vértices - 245760 índices - 81920 triângulos

    // salva uma cópia da geometria original
    vector <Vertex> base_vertices = vertices;
    vector <uint> base_indices = indices;

    // Guarda o índice das arestas e vértices já processados. 
    // Para o valor máximo de 6 subdivisões, 10242 vértices precisam 
    // ser processados. Porém o valor 0 é usado para indicar uma posição 
    // vazia na tabela, e por isso é usada uma linha e coluna a mais
    // totalizando 10243 x 10243 vértices, que resulta em 104'919'049.

    static uint edges[104'919'049];
    static uint verts[10243];       

    memset(edges, 0, 104'919'049 * sizeof(uint));
    memset(verts, 0, 10243 * sizeof(uint));

    vertices.resize(0);
    indices.resize(0);

    uint numTriangles = uint(base_indices.size()) / 3;
    for (uint i = 0; i < numTriangles; ++i)
    {
        uint i0 = base_indices[size_t(i) * 3 + 0];
        uint i1 = base_indices[size_t(i) * 3 + 1];
        uint i2 = base_indices[size_t(i) * 3 + 2];

        auto [iv0, im0] = AddEdge(i0, i1, base_vertices, vertices, edges, verts);
        auto [iv1, im1] = AddEdge(i1, i2, base_vertices, vertices, edges, verts);
        auto [iv2, im2] = AddEdge(i2, i0, base_vertices, vertices, edges, verts);

        //       v1
        //       *
        //      / \
        //     /   \
        //  m0*-----*m1
        //   / \   / \
        //  /   \ /   \
        // *-----*-----*
        // v0    m2     v2

        indices.push_back(iv0);
        indices.push_back(im0);
        indices.push_back(im2);

        indices.push_back(im0);
        indices.push_back(im1);
        indices.push_back(im2);

        indices.push_back(im2);
        indices.push_back(im1);
        indices.push_back(iv2);

        indices.push_back(im0);
        indices.push_back(iv1);
        indices.push_back(im1);
    }
}

void Geometry::SetNormals()
{

    // ----------------------------------------
    // Ajusta as normais dos vértices da malha 
    // ----------------------------------------

    uint numTriangles = IndexCount() / 3;

    for (size_t i = 0; i < numTriangles; ++i)
    {
        uint i0 = indices[i * 3 + 0];
        uint i1 = indices[i * 3 + 1];
        uint i2 = indices[i * 3 + 2];

        XMVECTOR v0 = XMLoadFloat3(&vertices[i0].pos);
        XMVECTOR v1 = XMLoadFloat3(&vertices[i1].pos);
        XMVECTOR v2 = XMLoadFloat3(&vertices[i2].pos);

        XMVECTOR u = v1 - v0;
        XMVECTOR v = v2 - v0;
        XMVECTOR faceNormal = XMVector3Cross(u, v);

        XMVECTOR n0 = XMLoadFloat3(&vertices[i0].normal);
        XMVECTOR n1 = XMLoadFloat3(&vertices[i1].normal);
        XMVECTOR n2 = XMLoadFloat3(&vertices[i2].normal);

        n0 += faceNormal;
        n1 += faceNormal;
        n2 += faceNormal;

        XMStoreFloat3(&vertices[i0].normal, n0);
        XMStoreFloat3(&vertices[i1].normal, n1);
        XMStoreFloat3(&vertices[i2].normal, n2);
    }

    // normaliza vetores resultantes
    for (auto& v : vertices)
    {
        XMVECTOR n = XMLoadFloat3(&v.normal);
        XMStoreFloat3(&v.normal, XMVector3Normalize(n));
    }
}

//                _____
// ______________/ Box \_________________________________________________________
// ------------------------------------------------------------------------------

Box::Box(float width, float height, float depth, XMFLOAT4 color)
{
    float w = 0.5f * width;
    float h = 0.5f * height;
    float d = 0.5f * depth;

    // cria os vértices da geometria
    Vertex boxVertices[24] =
    {
        // front
        { XMFLOAT3(-w, -h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },  // 0
        { XMFLOAT3(-w, +h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },  // 1
        { XMFLOAT3(+w, +h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },  // 2
        { XMFLOAT3(+w, -h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },  // 3
                                
        // back                 
        { XMFLOAT3(-w, -h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },  // 4
        { XMFLOAT3(-w, +h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },  // 5
        { XMFLOAT3(+w, +h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },  // 6
        { XMFLOAT3(+w, -h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },  // 7
                                
        // right                
        { XMFLOAT3(-w, -h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },  // 8
        { XMFLOAT3(-w, +h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },  // 9
        { XMFLOAT3(-w, -h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },  // 10
        { XMFLOAT3(-w, +h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },  // 11
                                
        // left                 
        { XMFLOAT3(+w, +h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },  // 12
        { XMFLOAT3(+w, -h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },  // 13
        { XMFLOAT3(+w, +h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },  // 14
        { XMFLOAT3(+w, -h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },  // 15
                                
        // top                  
        { XMFLOAT3(-w, +h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },  // 16
        { XMFLOAT3(-w, +h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },  // 17
        { XMFLOAT3(+w, +h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },  // 18
        { XMFLOAT3(+w, +h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },  // 19
                                
        { XMFLOAT3(-w, -h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },  // 20
        { XMFLOAT3(-w, -h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },  // 21
        { XMFLOAT3(+w, -h, +d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },  // 22
        { XMFLOAT3(+w, -h, -d), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }   // 23
    };

    // insere vértices na malha
    for (const Vertex& v : boxVertices)
        vertices.push_back(v);

    // índices indicam como os vértices são interligados
    uint boxIndices[36] =
    {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 7, 5,
        7, 6, 5,

        // left face
        10, 11, 9,
        10, 9, 8,

        // right face
        13, 12, 14,
        13, 14, 15,

        // top face
        16, 17, 18,
        16, 18, 19,

        // bottom face
        21, 20, 23,
        21, 23, 22
    };

    // insere índices na malha
    for (ushort i : boxIndices)
        indices.push_back(i);

    // ajusta normais dos vértices
    SetNormals();
}

//                        __________
// ______________________/ Cylinder \____________________________________________
// ------------------------------------------------------------------------------

Cylinder::Cylinder(float bottom, float top, float height, uint sliceCount, uint stackCount, XMFLOAT4 color)
{
    // altura de uma camada
    float stackHeight = height / stackCount;

    // incremento no raio de cada camada
    float radiusStep = (top - bottom) / stackCount;

    // número de anéis do cilindro
    uint ringCount = stackCount + 1;

    // calcula vértices de cada anel
    for (uint i = 0; i < ringCount; ++i)
    {
        float y = -0.5f * height + i * stackHeight;
        float r = bottom + i * radiusStep;
        float theta = 2.0f * XM_PI / sliceCount;

        for (uint j = 0; j <= sliceCount; ++j)
        {
            float c = cosf(j * theta);
            float s = sinf(j * theta);

            // -----------------------
            // coordenadas da textura
            // -----------------------
            // textura da lateral ocupa 50.0% da altura total
            // fatores 0.25 e 0.50 no 'u' são "ajustes finos"
            // -----------------------------------------------
            float u = float(j + 0.25f) / (sliceCount + 0.50f);
            float v = 0.50f + (1.00f - float(i) / stackCount) * 0.50f;

            Vertex vertex;
            vertex.pos = XMFLOAT3(r * c, y, r * s);
            vertex.color = color;
            vertex.texc = XMFLOAT2(u, v);
            vertices.push_back(vertex);
        }
    }

    // número de vértices em cada anel do cilindro
    uint ringVertexCount = sliceCount + 1;

    // calcula índices para cada camada
    for (uint i = 0; i < stackCount; ++i)
    {
        for (uint j = 0; j < sliceCount; ++j)
        {
            uint v0 = i * ringVertexCount + j;
            uint v1 = (i + 1) * ringVertexCount + j;
            uint v2 = (i + 1) * ringVertexCount + j + 1;
            uint v3 = i * ringVertexCount + j + 1;

            // muda direção do triângulo a cada fatia
            // para manter a mesma quantidade de arestas
            // chegando nos vértices. Isso faz com que as
            // normais fiquem uniformes ao longo de toda
            // a lateral do cilindro
            bool flip = (j % 2 == 0);

            if (flip)
            {
                indices.push_back(v0);
                indices.push_back(v1);
                indices.push_back(v2);

                indices.push_back(v0);
                indices.push_back(v2);
                indices.push_back(v3);
            }
            else
            {
                indices.push_back(v0);
                indices.push_back(v1);
                indices.push_back(v3);

                indices.push_back(v1);
                indices.push_back(v2);
                indices.push_back(v3);
            }
        }
    }

    // constrói vértices das tampas do cilindro
    for (uint k = 0; k < 2; ++k)
    {
        uint baseIndex = uint(vertices.size());

        float y = (k - 0.5f) * height;
        float theta = 2.0f * XM_PI / sliceCount;
        float r = (k ? top : bottom);
        Vertex vertex;

        for (uint i = 0; i <= sliceCount; i++)
        {
            float x = r * cosf(i * theta);
            float z = r * sinf(i * theta);

            // -----------------------
            // coordenadas da textura
            // -----------------------
            // as tampas ocupam 50.0% da altura da textura 
            // a tampa inferior deve andar mais 50.0% da largura
            // o valor de k controla a escolha da tampa
            // --------------------------------------------------
            float u = (1.0f - (x + r) / (2 * r)) * 0.5f + 0.5f * (k ? 0 : 1);
            float v = ((z + r) / (2 * r)) * 0.5f;

            vertex.pos = XMFLOAT3(x, y, z);
            vertex.color = color;
            vertex.texc = XMFLOAT2(u, v);
            vertices.push_back(vertex);
        }

        // vértice central da tampa
        vertex.pos = XMFLOAT3(0.0f, y, 0.0f);
        vertex.color = color;
        vertex.texc = (k ? XMFLOAT2(0.25f, 0.25f) : XMFLOAT2(0.75f, 0.25f));
        vertices.push_back(vertex);

        uint centerIndex = uint(vertices.size() - 1);

        // indices para a tampa
        for (uint i = 0; i < sliceCount; ++i)
        {
            indices.push_back(centerIndex);
            indices.push_back(baseIndex + i + k);
            indices.push_back(baseIndex + i + 1 - k);
        }
    }

    // ajusta normais dos vértices
    SetNormals();

    // corrige normais dos vértices que se encontram para fechar o cilindro
    // a função SetNormals() não leva em conta que a malha é aberta
    int sideVertices = (sliceCount + 1) * (stackCount + 1);
    for (int i = 0; i < sideVertices; i = i + sliceCount + 1)
    {
        XMVECTOR v0 = XMLoadFloat3(&vertices[i].normal);
        XMVECTOR v1 = XMLoadFloat3(&vertices[i + sliceCount].normal);
        XMVECTOR n = XMVector3Normalize(v0 + v1);

        XMStoreFloat3(&vertices[i].normal, n);
        XMStoreFloat3(&vertices[i + sliceCount].normal, n);
    }
}

//                                     ________
// ___________________________________/ Sphere \_________________________________
// ------------------------------------------------------------------------------

Sphere::Sphere(float radius, uint sliceCount, uint stackCount, XMFLOAT4 color)
{
    // calcula os vértices iniciando no pólo superior e descendo pelas camadas
    Vertex topVertex;
    topVertex.pos = XMFLOAT3(0.0f, radius, 0.0f);
    topVertex.color = color;

    Vertex bottomVertex;
    bottomVertex.pos = XMFLOAT3(0.0f, -radius, 0.0f);
    bottomVertex.color = color;

    float basePos = 1.0f / (2  * sliceCount);
    float deltaPos = 1.0f / sliceCount;
    
    // número de vértices no polo superior igual ao número de fatias
    for (uint i = 0; i < sliceCount; i++)
    {
        topVertex.texc = XMFLOAT2(basePos + i * deltaPos, 0.0f);
        vertices.push_back(topVertex);
    }

    float phiStep = XM_PI / stackCount;
    float thetaStep = 2.0f * XM_PI / sliceCount;

    // calcula os vértices para cada anel (não conta os pólos como anéis)
    for (uint i = 1; i <= stackCount - 1; ++i)
    {
        float phi = i * phiStep;

        // vértices do anel
        for (uint j = 0; j <= sliceCount; ++j)
        {
            float theta = j * thetaStep;

            Vertex v;

            // coordenadas esféricas para cartesianas
            v.pos.x = radius * sinf(phi) * cosf(theta);
            v.pos.y = radius * cosf(phi);
            v.pos.z = radius * sinf(phi) * sinf(theta);

            v.color = color;
            v.texc.x = theta / XM_2PI;
            v.texc.y = phi / XM_PI;

            vertices.push_back(v);
        }
    }

    // número de vértices no polo inferior igual ao número de fatias
    for (uint i = 0; i < sliceCount; i++)
    {
        bottomVertex.texc = XMFLOAT2(basePos + i * deltaPos, 1.0f);
        vertices.push_back(bottomVertex);
    }

    // conecta os vértices do pólo superior ao primeiro anel
    for (uint i = 0; i < sliceCount; ++i)
    {
        indices.push_back(sliceCount + i);
        indices.push_back(i);
        indices.push_back(sliceCount + i + 1);
    }

    // calcula os índices para as camadas internas (não conectadas aos pólos)
    uint baseIndex = sliceCount;
    uint ringVertexCount = sliceCount + 1;
    for (uint i = 0; i < stackCount - 2; ++i)
    {
        for (uint j = 0; j < sliceCount; ++j)
        {
            uint i0 = baseIndex + i * ringVertexCount + j;
            uint i1 = baseIndex + i * ringVertexCount + j + 1;
            uint i2 = baseIndex + (i + 1) * ringVertexCount + j;
            uint i3 = baseIndex + (i + 1) * ringVertexCount + j + 1;

            bool flip = (i + j) % 2 == 0;

            if (flip)
            {
                indices.push_back(i2); // 4
                indices.push_back(i0); // 1
                indices.push_back(i3); // 5

                indices.push_back(i3); // 5
                indices.push_back(i0); // 1
                indices.push_back(i1); // 2
            }
            else
            {
                indices.push_back(i2); // 4
                indices.push_back(i0); // 1
                indices.push_back(i1); // 2

                indices.push_back(i1); // 2
                indices.push_back(i3); // 5
                indices.push_back(i2); // 4
            }
        }
    }

    // os vértices do pólo inferior são adicionados por último
    uint southPoleIndex = (uint)vertices.size() - sliceCount;

    // se posiciona no índice do primeiro vértice do último anel
    baseIndex = southPoleIndex - ringVertexCount;

    // conecta os vértices do pólo inferior ao último anel
    for (uint i = 0; i < sliceCount; ++i)
    {
        indices.push_back(southPoleIndex + i);
        indices.push_back(baseIndex + i);
        indices.push_back(baseIndex + i + 1);
    }

    // ajusta normais dos vértices
    SetNormals();

    // corrige normais dos vértices que se encontram para fechar a esfera
    // a função SetNormals() não leva em conta que a malha é aberta
    int sideVertices = (sliceCount+1) * (stackCount-1);
    for (int i = sliceCount; i < sideVertices; i = i + sliceCount + 1)
    {
        XMVECTOR v0 = XMLoadFloat3(&vertices[i].normal);
        XMVECTOR v1 = XMLoadFloat3(&vertices[i + sliceCount].normal);
        XMVECTOR n = XMVector3Normalize(v0 + v1);

        XMStoreFloat3(&vertices[i].normal, n);
        XMStoreFloat3(&vertices[i + sliceCount].normal, n);
    }
}

//                                                ___________
// ______________________________________________/ GeoSphere \___________________
// ------------------------------------------------------------------------------

GeoSphere::GeoSphere(float radius, uint subdivisions, XMFLOAT4 color)
{
    // limita o número de subdivisões
    subdivisions = (subdivisions > 6U ? 6U : subdivisions);

    // aproxima uma esfera pela subdivisão de um icosaédro
    const float X = 0.525731f;
    const float Z = 0.850651f;

    // vértices do icosaédro
    XMFLOAT3 pos[12] =
    {
        XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),
        XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
        XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X),
        XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),
        XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f),
        XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
    };

    // índices do icosaédro
    uint k[60] =
    {
        1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
        1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
        3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
        10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
    };

    // ajusta e inicializa vetores de vértices e índices
    vertices.resize(12);
    indices.assign(&k[0], &k[60]);

    for (uint i = 0; i < 12; ++i)
        vertices[i].pos = pos[i];

    // subdivide cada triângulo do icosaedro um certo número de vezes
    for (uint i = 0; i < subdivisions; ++i)
        Subdivide();

    // projeta os vértices em uma esfera e ajusta a escala
    for (uint i = 0; i < vertices.size(); ++i)
    {
        // normaliza vetor (ponto)
        XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&vertices[i].pos));

        // projeta na esfera
        XMVECTOR p = radius * n;

        XMStoreFloat3(&vertices[i].pos, p);
        vertices[i].color = color;

        // deriva coordenadas da textura a partir das coordenadas esféricas
        float theta = atan2f(vertices[i].pos.z, vertices[i].pos.x) + XM_PI;
        float phi = acosf(vertices[i].pos.y / radius);
        vertices[i].texc = XMFLOAT2(theta / XM_2PI, phi / XM_PI);        
    }

    // ajusta normais dos vértices
    SetNormals();
}

//                                                              ______
// ____________________________________________________________/ Grid \__________
// ------------------------------------------------------------------------------

Grid::Grid(float width, float depth, uint m, uint n, XMFLOAT4 color)
{
    uint vertexCount = m * n;
    uint triangleCount = 2 * (m - 1) * (n - 1);

    // cria os vértices
    float halfWidth = 0.5f * width;
    float halfDepth = 0.5f * depth;

    float dx = width / (n - 1);
    float dz = depth / (m - 1);

    float du = 1.0f / (n - 1);
    float dv = 1.0f / (m - 1);

    // ajusta tamanho do vetor de vértices
    vertices.resize(vertexCount);

    for (uint i = 0; i < m; ++i)
    {
        float z = halfDepth - i * dz;

        for (uint j = 0; j < n; ++j)
        {
            float x = -halfWidth + j * dx;

            // define vértices do grid
            vertices[size_t(i) * n + j].pos = XMFLOAT3(x, 0.0f, z);
            vertices[size_t(i) * n + j].color = color;

            // mapeia textura sobre o grid
            vertices[size_t(i) * n + j].texc = XMFLOAT2(j * du, i * dv);
        }
    }

    // ajusta tamanho do vetor de índices
    indices.resize(size_t(triangleCount) * 3);

    size_t k = 0;

    for (uint i = 0; i < m - 1; ++i)
    {
        for (uint j = 0; j < n - 1; ++j)
        {
            indices[k] = i * n + j;
            indices[k + 1] = i * n + j + 1;
            indices[k + 2] = (i + 1) * n + j;
            indices[k + 3] = (i + 1) * n + j;
            indices[k + 4] = i * n + j + 1;
            indices[k + 5] = (i + 1) * n + j + 1;

            k += 6; // próximo quad
        }
    }

    // ajusta normais dos vértices
    SetNormals();
}

//                                                                       ______
// _____________________________________________________________________/ Quad \_
// ------------------------------------------------------------------------------

Quad::Quad(float width, float height, XMFLOAT4 color)
{
    float w = 0.5f * width;
    float h = 0.5f * height;

    // cria vertex buffer
    Vertex quadVertices[4] =
    {
        { XMFLOAT3(-w, -h, 0.0f), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-w, +h, 0.0f), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(+w, +h, 0.0f), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(+w, -h, 0.0f), color, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }
    };

    // insere vértices na malha
    for (const Vertex& v : quadVertices)
        vertices.push_back(v);

    // cria o objIndex buffer
    uint quadIndices[6] =
    {
        2, 1, 0,
        3, 2, 0
    };

    // insere índices na malha
    for (uint i : quadIndices)
        indices.push_back(i);

    // ajusta normais dos vértices
    SetNormals();
}

// -------------------------------------------------------------------------------