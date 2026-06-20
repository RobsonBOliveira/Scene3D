//--------------------------------------------------------------------------------------
// WIC Image Loader for DX12
//
// Function for loading a WIC image and creating a Direct3D runtime texture for it
// (auto-generating mipmaps if possible)
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include <d3d12.h>
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"windowscodecs.lib")

#include <cstddef>
#include <cstdint>
#include <memory>

namespace DirectX
{

#ifndef WIC_LOADER_FLAGS_DEFINED
#define WIC_LOADER_FLAGS_DEFINED
    enum WIC_LOADER_FLAGS : uint32_t
    {
        WIC_LOADER_DEFAULT = 0,
        WIC_LOADER_FORCE_SRGB = 0x1,
        WIC_LOADER_IGNORE_SRGB = 0x2,
        WIC_LOADER_SRGB_DEFAULT = 0x4,
        WIC_LOADER_MIP_AUTOGEN = 0x8,
        WIC_LOADER_MIP_RESERVE = 0x10,
        WIC_LOADER_FIT_POW2 = 0x20,
        WIC_LOADER_MAKE_SQUARE = 0x40,
        WIC_LOADER_FORCE_RGBA32 = 0x80,
    };

    DEFINE_ENUM_FLAG_OPERATORS(WIC_LOADER_FLAGS);
#endif

    HRESULT __cdecl LoadTextureFromFile(
        _In_ ID3D12Device* d3dDevice,
        _In_z_ const wchar_t* szFileName,
        _Outptr_ ID3D12Resource** texture,
        std::unique_ptr<uint8_t[]>& decodedData,
        D3D12_SUBRESOURCE_DATA& subresource,
        size_t maxsize = 0) noexcept;
}
