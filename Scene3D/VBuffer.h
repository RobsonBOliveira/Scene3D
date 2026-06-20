/**********************************************************************************
// VBuffer (Arquivo de Cabeçalho)
//
// Criação:     07 Mar 2025
// Atualização: 20 Jun 2025
// Compilador:  Visual C++ 2022
//
// Descrição:   Representa um recurso do tipo Vertex Buffer
//
**********************************************************************************/

#ifndef DXUT_VBUFFER_H_
#define DXUT_VBUFFER_H_

// -------------------------------------------------------------------------------

#include "Types.h"
#include "Engine.h"
#include "Graphics.h"
#include "Geometry.h"

// -------------------------------------------------------------------------------

template<typename T> class VertexBuffer
{
private:
    ID3D12Resource * upload;            // buffer de Upload CPU -> GPU
    ID3D12Resource * vbuffer;           // buffer na GPU
    D3D12_VERTEX_BUFFER_VIEW view;      // descritor do buffer
    uint size;                          // tamanho do buffer
    uint stride;                        // tamanho de um vértice

public:

    // construtor que aloca e copia dados para o vertex buffer
    VertexBuffer<T>(const void * vertices, uint count)
    {
        // guarda tamanho do vértice
        stride = sizeof(T);

        // guarda tamanho do buffer
        size = count * stride;

        // aloca recursos para o vertex buffer
        Engine::graphics->Allocate(UPLOAD, size, &upload);
        Engine::graphics->Allocate(GPU, size, &vbuffer);

        // se fornecida uma lista de vértices
        if (vertices)
        {
            // copia vértices para o buffer da GPU usando o buffer de Upload
            Engine::graphics->Copy(vertices, size, upload, vbuffer);
        }
    }

    // construtor que recebe uma geometria
    VertexBuffer<T>(const Geometry& geo)
    {
        // guarda tamanho do buffer e vértice
        size = geo.VertexCount() * sizeof(T);
        stride = sizeof(T);

        // aloca recursos para o vertex buffer
        Engine::graphics->Allocate(UPLOAD, size, &upload);
        Engine::graphics->Allocate(GPU, size, &vbuffer);

        // copia vértices para o buffer da GPU usando o buffer de Upload
        Engine::graphics->Copy(geo.VertexData(), size, upload, vbuffer);
    }

    // construtor de cópia
    VertexBuffer(const VertexBuffer & rhs) = delete;

    // operador de atribuição
    VertexBuffer & operator=(const VertexBuffer & rhs) = delete;

    // destrutor
    ~VertexBuffer()
    {
        if (upload) upload->Release();
        if (vbuffer) vbuffer->Release();
    }

    void Copy(const void* vertices, uint count)
    {
        // copia vértices para o buffer da GPU usando o buffer de Upload
        Engine::graphics->Copy(vertices, count * stride, upload, vbuffer);

    }

    D3D12_VERTEX_BUFFER_VIEW * View()
    {
        view.BufferLocation = vbuffer->GetGPUVirtualAddress();
        view.StrideInBytes = stride;
        view.SizeInBytes = size;

        return &view;
    }
};

// -------------------------------------------------------------------------------

#endif



