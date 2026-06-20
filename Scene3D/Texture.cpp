/**********************************************************************************
// Texture (Código Fonte)
//
// Criação:     04 Oct 2023
// Atualização: 20 Jul 2025
// Compilador:  Visual C++ 2022
//
// Descrição:   Representa uma textura de uma malha 3D
//
**********************************************************************************/

#include "Engine.h"
#include "Error.h"
#include "Texture.h"
#include "WIC.h"
#include "DDS.h"
#include <vector>
#include <memory>
using std::unique_ptr;
using std::vector;

// -------------------------------------------------------------------------------

Texture::Texture(initializer_list<const char*> files)
{
    // heap de descritores para a textura
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = uint(files.size());
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(Engine::graphics->Device()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&heap)));

    // slot inicial das texturas
    uint slot = 0;

    // carrega arquivos
    for (auto& file : files)
    {
        // nova textura na lista
        textures.push_back(SubTexture{});

        // converte de char para wchar_t
        wchar_t filename[4096];
        MultiByteToWideChar(CP_ACP, 0, file, -1, filename, 4096);
        
        // carrega textura no slot
        LoadTexture(filename, slot++);
    }
}

// -------------------------------------------------------------------------------

Texture::Texture(const char* file) : Texture({file})
{
}

// -------------------------------------------------------------------------------

Texture::~Texture()
{
    if (heap)
        heap->Release();

    for (auto& tex : textures)
    {
        if (tex.upload)
            tex.upload->Release();

        if (tex.texture)
            tex.texture->Release();
    }    
}

// -------------------------------------------------------------------------------

void Texture::LoadTexture(wchar_t* filename, uint slot)
{
    // guarda dados obtidos da textura
    unique_ptr<uint8_t[]> decodedData;
    vector<D3D12_SUBRESOURCE_DATA> subresource = {};

    // aponta para extensão do arquivo
    const wchar_t* extension = filename + wcslen(filename) - 4;

    // arquivo do tipo Direct Draw Surface
    bool dds = extension[0] == '.' && extension[1] == 'd' && extension[2] == 'd' && extension[3] == 's';

    // verifica tipo do arquivo pela extensão
    if (dds)
    {
        // carrega textura em um recurso
        ThrowIfFailed(DirectX::LoadDDSTextureFromFile(
            Engine::graphics->Device(), // dispositivo Direct3D
            filename,                   // caminho para o arquivo
            &textures[slot].texture,    // saída: texture resource
            decodedData,                // saída: dados decodificados
            subresource                 // saída: dados do subrecurso
        ));
    }
    else
    {
        subresource.push_back(D3D12_SUBRESOURCE_DATA{});

        // carrega textura em um recurso
        ThrowIfFailed(DirectX::LoadTextureFromFile(
            Engine::graphics->Device(), // dispositivo Direct3D
            filename,                   // caminho para o arquivo
            &textures[slot].texture,    // saída: texture resource
            decodedData,                // saída: dados decodificados
            subresource[0]              // saída: dados do subrecurso
        ));
    }

    // -------------------------
    // Upload na Memória da GPU 
    // -------------------------

    // informações da textura
    D3D12_RESOURCE_DESC texDesc = textures[slot].texture->GetDesc();

    // número de mipmaps na textura
    const uint numSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;

    // recupera tamanho da textura
    ullong textureSize = 0;
    Engine::graphics->Device()->GetCopyableFootprints(
        &texDesc, 0, numSubresources, 0, nullptr, nullptr, nullptr, &textureSize
    );

    // aloca memória de upload
    Engine::graphics->Allocate(UPLOAD, textureSize, &textures[slot].upload);

    // copia textura para a memória de Upload e depois para a GPU
    Engine::graphics->Copy(subresource.data(), textureSize, textures[slot].upload, textures[slot].texture);

    // ---------------------
    // Shader Resource View
    // ---------------------

    // configura descritor para a textura
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    // tamanho de um descritor do tipo SRV
    uint descriptorSize =
        Engine::graphics->Device()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // endereço do primeiro descritor na heap
    D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();

    // posição na heap de descritores
    handle.ptr += slot * descriptorSize;

    // cria descritor para a textura
    Engine::graphics->Device()->CreateShaderResourceView(textures[slot].texture, &srvDesc, handle);
}

// -------------------------------------------------------------------------------
