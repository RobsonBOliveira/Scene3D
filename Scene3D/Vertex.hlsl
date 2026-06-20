/**********************************************************************************
// Vertex (Arquivo de Sombreamento)
//
// Criação:     22 Jul 2020
// Atualização: 18 Jul 2025
// Compilador:  Direct3D Shader Compiler (FXC)
//
// Descrição:   Aplica a transformação de mundo aos vértices do objeto
//
**********************************************************************************/

struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 PosW : POSITION;
    float4 Color : COLOR;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

// ------------------------------------------------------------------------------

cbuffer Scene : register(b0)
{
    float4x4 ViewProj;          // matrix de visualização e projeção
}

cbuffer Object : register(b1)
{
    float4x4 World;             // matriz de mundo 
    float4x4 TexTransform;      // transformação da textura
    int ObjIndex;               // índice do objeto
}

// ------------------------------------------------------------------------------

VertexOut main(VertexIn vin)
{
    VertexOut vout;
    
    // transforma para coordenadas do mundo
    vout.PosW = mul(float4(vin.PosL, 1.0f), World);

    // transforma para coordenadas de projeção
    vout.PosH = mul(vout.PosW, ViewProj);
    
    // transforma normais para coordenadas do mundo
    vout.NormalW = mul(vin.NormalL, (float3x3) World);
    
    // calcula cor do vértice
    vout.Color = vin.Color;
    
    /// transforma coordenadas de textura
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), TexTransform).xy;

    return vout;
}