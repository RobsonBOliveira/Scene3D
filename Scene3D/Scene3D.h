/**********************************************************************************
// Scene3D (Arquivo de Cabeçalho)
//
// Criação:     02 Out 2023
// Atualização: 18 Jul 2025
// Compilador:  Visual C++ 2022
//
// Descrição:   Demonstra os MipMaps do formato DDS
//
**********************************************************************************/

#include "DXUT.h"

// ------------------------------------------------------------------------------

struct Light
{
    XMFLOAT3 Direction;         // vetor na direção da luz
    float FalloffStart;         // inicio da atenuação
    XMFLOAT3 Strength;          // potência da luz
    float FalloffEnd;           // limite da atenuação
    XMFLOAT3 Position;          // posição da luz
    float SpotPower;            // potência do holofote
};

struct Scene
{
    XMFLOAT4X4 ViewProj;        // matriz de visualização e projeção
    XMFLOAT4 Ambient;           // cor da luz ambiente
    Light Lights;               // luzes da cena
    XMFLOAT3 Eye;               // vetor na direção do observador
    int Amb;                    // luz ambiente
    int Dif;                    // luz difusa
    int Spe;                    // luz especular
    int Lam;                    // ângulo de incidência
    int Mat;                    // material dos objetos
    int Tex;                    // textura dos objetos
};

struct Constants
{
    XMFLOAT4X4 World;           // matriz de mundo
    XMFLOAT4X4 TexTransform;    // transformação da textura
    int ObjIndex;               // índice do objeto
};

struct Material
{
    XMFLOAT4 Albedo;            // cor 
    XMFLOAT3 FresnelR0;         // refletividade
    float    Roughness;         // rugosidade
};

// ------------------------------------------------------------------------------

struct Object
{
    XMFLOAT4X4 world = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };

    Mesh * mesh = nullptr;
    VertexBuffer<Vertex> * vbuffer = nullptr;
    IndexBuffer<uint> * ibuffer = nullptr;
    ConstantBuffer<Constants> * cbuffer = nullptr;
    ConstantBuffer<Material> * material = nullptr;
    Texture * texture = nullptr;
};

// ------------------------------------------------------------------------------

class Scene3D : public App
{
private:
    ID3D12RootSignature * rootSignature = nullptr;
    ID3D12PipelineState * pipelineState = nullptr;
    ID3D12PipelineState * pipelineWire = nullptr;
    ID3D12PipelineState * pipelineSolid = nullptr;

    ConstantBuffer<Scene>* sceneBuffer = nullptr;
    vector<Object> scene;
    OrbitCamera orbitcam;
    XMFLOAT4X4 Proj;

    static Timer timer;
    bool solid = true;
    bool rotating = false;

    XMFLOAT4X4 Identity
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // posição da luz
    float thetaL = 1.1f;
    float phiL = XM_PIDIV4;
    float radiusL = 8.0f;

    // intensidade da luz
    float Strength = 2.0f;

    bool Amb = true;
    bool Dif = true;
    bool Spe = true;
    bool Lam = true;
    bool Mat = true;
    bool Tex = true;

public:
    void Init();
    void Update();
    void Draw();
    void Finalize();

    static void Pause() { timer.Stop(); }
    static void Resume() { timer.Start(); }

    void BuildRootSignature();
    void BuildPipelineState();
};

// ------------------------------------------------------------------------------
