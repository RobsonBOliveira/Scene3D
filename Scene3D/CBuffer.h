/**********************************************************************************
// CBuffer (Arquivo de Cabeçalho)
//
// Criação:     05 Mar 2025
// Atualização: 22 Jun 2025
// Compilador:  Visual C++ 2022
//
// Descrição:   Representa um recurso do tipo Constant Buffer 
//
**********************************************************************************/

#ifndef DXUT_CBUFFER_H_
#define DXUT_CBUFFER_H_

// -------------------------------------------------------------------------------

#include "Types.h"
#include "Graphics.h"

// -------------------------------------------------------------------------------

template<typename T>
class ConstantBuffer
{
private:
    ID3D12Resource * upload = nullptr;          // buffer de Upload CPU -> GPU
    byte * data = nullptr;                      // buffer mapeado na CPU
    uint elementSize = 0;                       // tamanho de um elemento no buffer

public:
    // construtor padrão
    ConstantBuffer(uint count = 1)
    {
        // o tamanho dos constant buffers precisam ser múltiplos 
        // do tamanho de alocação mínima do hardware (256 bytes)
        elementSize = (sizeof(T) + 255) & ~255;

        // aloca recursos para o constant buffer
        Engine::graphics->Allocate(CBUFFER, elementSize * count, &upload);

        // mapeia memória do upload buffer para um endereço acessível pela CPU
        upload->Map(0, nullptr, reinterpret_cast<void **>(&data));
    }

    // construtor para criar buffer e copiar dados
    ConstantBuffer(const void * cbData, uint count = 1) : ConstantBuffer(count)
    { Copy(cbData); }

    // construtor de cópia
    ConstantBuffer(const ConstantBuffer & rhs) = delete;

    // operador de atribuição
    ConstantBuffer & operator=(const ConstantBuffer & rhs) = delete;

    // destrutor
    ~ConstantBuffer()
    {
        if (upload)
        {
            upload->Unmap(0, nullptr);
            upload->Release();
        }
    }

    // copia dados para o constant buffer
    void Copy(const void * cbData, uint cbIndex = 0)
    { memcpy(data + (cbIndex * elementSize), cbData, elementSize); }

    // retorna endereço do buffer constante
    D3D12_GPU_VIRTUAL_ADDRESS View(uint index = 0) const
    {
        // encontra endereço da posição 'objIndex' no buffer constante
        D3D12_GPU_VIRTUAL_ADDRESS address = upload->GetGPUVirtualAddress();
        address += index * elementSize;
        return address;
    }
};

// -------------------------------------------------------------------------------

#endif


