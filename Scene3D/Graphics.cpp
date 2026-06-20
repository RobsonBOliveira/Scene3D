/**********************************************************************************
// Graphics (Código Fonte)
// 
// Criação:     06 Abr 2011
// Atualização: 26 Jul 2025
// Compilador:  Visual C++ 2022
//
// Descrição:   Usa funções do Direct3D 12 para acessar a GPU
//
**********************************************************************************/

#include "Graphics.h"
#include "Error.h"
#include <sstream>
using std::wstringstream;

// ------------------------------------------------------------------------------

Graphics::Graphics()
{
    // configuração
    bufferIndex = 0;            // primeiro buffer
    samples = 1;                // amostragem padrão
    quality = 0;                // qualidade padrão
    vSync = false;              // sem vertical sync
    antialiasing = false;       // anti-aliasing desligado

    // cor de fundo
    bgColor[0] = 0.0f;          // (R)ed
    bgColor[1] = 0.0f;          // (G)reen
    bgColor[2] = 0.0f;          // (B)lue
    bgColor[3] = 0.0f;          // (A)lpha: 0.0 = transparente, 1.0 = sólido

    // infraestrutura gráfica
    factory           = nullptr;
    device            = nullptr;
    swapChain         = nullptr;
    commandQueue      = nullptr;
    commandList       = nullptr;

    // um alocador para cada render target buffer
    for (uint i = 0; i < bufferCount; i++)
        commandAlloc[i] = nullptr;
    
    // pipeline do Direct3D
    renderTargets     = new ID3D12Resource*[bufferCount] {nullptr};
    depthStencil      = nullptr;
    renderTargetHeap  = nullptr;
    depthStencilHeap  = nullptr;
    rtDescriptorSize  = 0;
    ZeroMemory(&viewport, sizeof(viewport));
    ZeroMemory(&scissorRect, sizeof(scissorRect));

    // msaa
    msaaRenderTarget = nullptr;
    msaaRenderTargetHeap = nullptr;
    msaaDepthStencilHeap = nullptr;

    // sincronização cpu/gpu
    fence = nullptr;
    fenceEvent = nullptr;

    // as cercas iniciam em zero
    for (uint i = 0; i < bufferCount; i++)
        fenceValue[i] = 1;
}

// ------------------------------------------------------------------------------

Graphics::~Graphics()
{
    // libera sinalizador de eventos
    CloseHandle(fenceEvent);

    // libera cerca
    if (fence)
        fence->Release();

    // libera depth stencil buffer
    if (depthStencil)
        depthStencil->Release();

    // libera render targets buffers
    if (renderTargets)
    {
        for (uint i = 0; i < bufferCount; ++i)
        {
            if (renderTargets[i])
                renderTargets[i]->Release();
        }
        delete[] renderTargets;
    }

    // libera depth stencil heap
    if (depthStencilHeap)
        depthStencilHeap->Release();

    // libera render target heap
    if (renderTargetHeap)
        renderTargetHeap->Release();

    // libera msaa depth stencil
    if (msaaDepthStencil)
        msaaDepthStencil->Release();

    // libera msaa depth stencil heap
    if (msaaDepthStencilHeap)
        msaaDepthStencilHeap->Release();

    // libera msaa render target
    if (msaaRenderTarget)
        msaaRenderTarget->Release();

    // libera msaa render target heap
    if (msaaRenderTargetHeap)
        msaaRenderTargetHeap->Release();

    // libera swap chain
    if (swapChain)
    {
        // Direct3D é incapaz de fechar quando em tela cheia
        swapChain->SetFullscreenState(false, NULL);
        swapChain->Release();
    }

    // libera lista de comandos
    if (commandList)
        commandList->Release();

    // libera alocadores de comandos
    for (uint i = 0; i < bufferCount; i++)
    {
        if (commandAlloc[i])
            commandAlloc[i]->Release();
    }

    // libera fila de comandos
    if (commandQueue)
        commandQueue->Release();

    // libera dispositivo gráfico
    if (device)
        device->Release();

    // libera interface principal
    if (factory)
        factory->Release();
}

// ------------------------------------------------------------------------------

void Graphics::LogHardwareInfo()
{
    const uint BytesInMegaByte = 1048576U;

    // --------------------------------------
    // Adaptador de vídeo (placa de vídeo)
    // --------------------------------------
    IDXGIAdapter* adapter = nullptr;
    if (factory->EnumAdapters(0, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        wstringstream text;
        text << L"---> Placa de vídeo: " << desc.Description << L"\n";
        OutputDebugStringW(text.str().c_str());
    }

    IDXGIAdapter4* adapter4 = nullptr;
    if (SUCCEEDED(adapter->QueryInterface(IID_PPV_ARGS(&adapter4))))
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO memInfo;
        adapter4->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfo);
        
        wstringstream text;
        text << L"---> Memória de vídeo (livre): " << memInfo.Budget / BytesInMegaByte << L"MB\n";
        text << L"---> Memória de vídeo (usada): " << memInfo.CurrentUsage / BytesInMegaByte << L"MB\n";
        OutputDebugStringW(text.str().c_str());

        adapter4->Release();
    }    

    // -----------------------------------------
    // Feature Level máximo suportado pela GPU
    // -----------------------------------------
    D3D_FEATURE_LEVEL featureLevels[10] =
    {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelsInfo;
    featureLevelsInfo.NumFeatureLevels = 10;
    featureLevelsInfo.pFeatureLevelsRequested = featureLevels;

    device->CheckFeatureSupport(
        D3D12_FEATURE_FEATURE_LEVELS,
        &featureLevelsInfo,
        sizeof(featureLevelsInfo));

    // bloco de instruções
    {
        wstringstream text;
        text << L"---> Feature Level: ";
        switch (featureLevelsInfo.MaxSupportedFeatureLevel)
        {
        case D3D_FEATURE_LEVEL_12_2: text << L"12_2\n"; break;
        case D3D_FEATURE_LEVEL_12_1: text << L"12_1\n"; break;
        case D3D_FEATURE_LEVEL_12_0: text << L"12_0\n"; break;
        case D3D_FEATURE_LEVEL_11_1: text << L"11_1\n"; break;
        case D3D_FEATURE_LEVEL_11_0: text << L"11_0\n"; break;
        case D3D_FEATURE_LEVEL_10_1: text << L"10_1\n"; break;
        case D3D_FEATURE_LEVEL_10_0: text << L"10_0\n"; break;
        case D3D_FEATURE_LEVEL_9_3:  text << L"9_3\n";  break;
        case D3D_FEATURE_LEVEL_9_2:  text << L"9_2\n";  break;
        case D3D_FEATURE_LEVEL_9_1:  text << L"9_1\n";  break;
        }
        OutputDebugStringW(text.str().c_str());
    }

    // -----------------------------------------
    // Saída de vídeo (monitor)
    // -----------------------------------------

    IDXGIOutput* output = nullptr;
    if (adapter->EnumOutputs(0, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        wstringstream text;
        text << L"---> Monitor: " << desc.DeviceName << L"\n";
        OutputDebugStringW(text.str().c_str());
    }

    // ------------------------------------------
    // Modo de vídeo (resolução)
    // ------------------------------------------

    // pega as dimensões da tela
    uint dpi = GetDpiForSystem();
    uint screenWidth = GetSystemMetricsForDpi(SM_CXSCREEN, dpi);
    uint screenHeight = GetSystemMetricsForDpi(SM_CYSCREEN, dpi);

    // pega a frequencia de atualização da tela
    DEVMODE devMode = { 0 };
    devMode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
    uint refresh = devMode.dmDisplayFrequency;

    wstringstream text;
    text << L"---> Resolução: " << screenWidth << L"x" << screenHeight << L" " << refresh << L" Hz\n";
    text << L"---> Dpi: " << dpi << "\n";
    OutputDebugStringW(text.str().c_str());

    // ------------------------------------------
    // Multi-Sample Anti-Aliasing (MSAA)
    // ------------------------------------------

    text.str(L"");
    text << L"---> Msaa: " << samples << L"x (" << (antialiasing ? L"on" : L"off") << L")\n";
    OutputDebugStringW(text.str().c_str());

    // ------------------------------------------

    // libera interfaces DXGI utilizadas
    if (adapter) adapter->Release();
    if (output) output->Release();
}


// -----------------------------------------------------------------------------

void Graphics::Initialize(Window * window)
{
    // ---------------------------------------------------
    // Infraestrutura DXGI
    // ---------------------------------------------------

    uint factoryFlags = 0;

#ifdef _DEBUG
    // habilita a camada de depuração do DXGI
    factoryFlags = DXGI_CREATE_FACTORY_DEBUG;

    // habilita a camada de depuração do D3D12
    ID3D12Debug * debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();
#endif

    // cria objeto para infraestrutura gráfica do DirectX (DXGI)
    ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));

    // ---------------------------------------------------
    // Dispositivo Direct 3D
    // ---------------------------------------------------

    // cria objeto para dispositivo gráfico
    if FAILED(D3D12CreateDevice(
        nullptr,                                // adaptador de vídeo (nullptr = adaptador padrão)
        D3D_FEATURE_LEVEL_11_0,                 // versão mínima dos recursos do Direct3D
        IID_PPV_ARGS(&device)))                 // guarda o dispositivo D3D criado
    {
        // tenta criar um dispositivo WARP 
        IDXGIAdapter * warp;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));

        // cria objeto D3D usando dispositivo WARP
        ThrowIfFailed(D3D12CreateDevice(
            warp,                               // adaptador de vídeo WARP (software)
            D3D_FEATURE_LEVEL_11_0,             // versão mínima dos recursos do Direct3D
            IID_PPV_ARGS(&device)));            // guarda o dispositivo D3D criado

        // libera objeto não mais necessário
        warp->Release();

        // informa uso de um disposito WARP:
        // implementa as funcionalidades do 
        // D3D12 em software (lento)
        OutputDebugString("---> Usando Adaptador WARP: não há suporte ao D3D12\n");
    }

    // ---------------------------------------------------
    // Multi-Sample Anti-Aliasing (MSAA)
    // ---------------------------------------------------

    uint level = 0;
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multisample = {};

    // verifica o maior nível suportado de MSAA
    for (level = samples; level > 1; level--)
    {
        multisample = { DXGI_FORMAT_R8G8B8A8_UNORM, level };
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &multisample, sizeof(multisample))))
            continue;

        if (multisample.NumQualityLevels > 0)
            break;
    }

    // se não há suporte a MSAA
    if (level < 2)
    {
        samples = 1;
        quality = 0;
        antialiasing = false;
    }
    else
    {
        samples = multisample.SampleCount;
        quality = multisample.NumQualityLevels - 1;
    }

    // ---------------------------------------------------
    // Log Hardware Info
    // ---------------------------------------------------

    // exibe informações do hardware gráfico no Output do Visual Studio
#ifdef _DEBUG
    LogHardwareInfo();
#endif 

    // ---------------------------------------------------
    // Fila, Lista e Alocador de Commandos
    // ---------------------------------------------------

    // cria fila de comandos da GPU
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

    // cria alocadores de comandos
    for (uint i = 0; i < bufferCount; i++)
    {
        ThrowIfFailed(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,    // tipo de lista de comandos
            IID_PPV_ARGS(&commandAlloc[i])));  // objeto alocador de comandos
    }

    // cria a lista de comandos
    ThrowIfFailed(device->CreateCommandList(
        0,										// GPU a ser utilizada
        D3D12_COMMAND_LIST_TYPE_DIRECT,			// não herda estado na GPU
        commandAlloc[bufferIndex],			    // alocador de comandos
        nullptr,								// estado inicial do pipeline
        IID_PPV_ARGS(&commandList)));			// objeto lista de comandos

    // ---------------------------------------------------
    // Sincronização CPU/GPU
    // ---------------------------------------------------

    // cria cerca para sincronização
    ThrowIfFailed(device->CreateFence(
        0,                                      // valor inicial da cerca
        D3D12_FENCE_FLAG_NONE,                  // cerca padrão para uma GPU
        IID_PPV_ARGS(&fence)));                 // objeto representando a cerca

    // cria objeto para sinalização de eventos
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr)
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));

    // ---------------------------------------------------
    // Swap Chain
    // ---------------------------------------------------

    // descreve swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = window->Width();
    swapChainDesc.Height = window->Height();
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    
    // cria a swap chain
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        commandQueue,                           // fila de comandos da GPU 
        window->Id(),                           // identificador da janela
        &swapChainDesc,                         // descrição da swap chain
        nullptr,                                // swap chain para tela cheia
        nullptr,                                // restringir tela de saída
        &swapChain));                           // objeto swap chain

    // ---------------------------------------------------
    // Render Target (Heap e View/Descriptor)
    // ---------------------------------------------------

    // descreve e cria uma heap para o descritor tipo Render Target (RT)
    D3D12_DESCRIPTOR_HEAP_DESC renderTargetHeapDesc = {};
    renderTargetHeapDesc.NumDescriptors = bufferCount;
    renderTargetHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    renderTargetHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&renderTargetHeapDesc, IID_PPV_ARGS(&renderTargetHeap)));

    // pega um Handle para o início da Heap
    D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = renderTargetHeap->GetCPUDescriptorHandleForHeapStart();

    // valor a incrementar para acessar o próximo descritor dentro da Heap
    // o tamanho de um descritor depende do hardware gráfico e do tipo de heap utilizada
    rtDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // cria um descritor (view) de Render Target para cada buffer (front e back buffers)
    for (uint i = 0; i < bufferCount; ++i)
    {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
        device->CreateRenderTargetView(renderTargets[i], nullptr, rtHandle);
        rtHandle.ptr += rtDescriptorSize;
    }

    // ---------------------------------------------------
    // Depth/Stencil (Heap e View/Descriptor)
    // ---------------------------------------------------

    // descrição do buffer Depth/Stencil
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = window->Width();
    depthStencilDesc.Height = window->Height();
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // propriedades da Heap do buffer Depth/Stencil
    D3D12_HEAP_PROPERTIES dsHeapProperties = {};
    dsHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    dsHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    dsHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    dsHeapProperties.CreationNodeMask = 1;
    dsHeapProperties.VisibleNodeMask = 1;

    // descreve valores para limpeza do Depth/Stencil buffer
    D3D12_CLEAR_VALUE optmizedClear = {};
    optmizedClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optmizedClear.DepthStencil.Depth = 1.0f;
    optmizedClear.DepthStencil.Stencil = 0;

    // cria um buffer Depth/Stencil
    ThrowIfFailed(device->CreateCommittedResource(
        &dsHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optmizedClear,
        IID_PPV_ARGS(&depthStencil)));

    // descreve e cria uma heap para o descritor tipo Depth/Stencil (DS)
    D3D12_DESCRIPTOR_HEAP_DESC depthstencilHeapDesc = {};
    depthstencilHeapDesc.NumDescriptors = 1;
    depthstencilHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    depthstencilHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&depthstencilHeapDesc, IID_PPV_ARGS(&depthStencilHeap)));

    // pega um Handle para o início da Heap
    D3D12_CPU_DESCRIPTOR_HANDLE dsHandle = depthStencilHeap->GetCPUDescriptorHandleForHeapStart();

    // cria um descritor (view) de Depth/Stencil para o mip nível 0
    device->CreateDepthStencilView(depthStencil, nullptr, dsHandle);

    // faz a transição do estado inicial do recurso para ser usado como buffer de profundidade
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = depthStencil;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    // ---------------------------------------------------
    // Viewport e Retângulo de Recorte
    // ---------------------------------------------------

    // ajusta a viewport
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(window->Width());
    viewport.Height = static_cast<float>(window->Height());
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    // ajusta o retângulo de corte
    scissorRect = { 0, 0, window->Width(), window->Height() };

    // ---------------------------------------------------
    // Cor de Fundo do Backbuffer
    // ---------------------------------------------------

    // ajusta a cor de fundo do backbuffer
    // para a mesma cor de fundo da janela
    COLORREF color = window->Color();

    bgColor[0] = GetRValue(color)/255.0f;   // Red
    bgColor[1] = GetGValue(color)/255.0f;   // Green
    bgColor[2] = GetBValue(color)/255.0f;   // Blue
    bgColor[3] = 1.0f;                      // Alpha (1 = sólido)

    // ---------------------------------------------------
    // MSAA Render Target e Depth Stencil
    // ---------------------------------------------------

    // se anti-aliasing está ativado
    if (antialiasing)
    {
        // descrição da render target para MSAA
        D3D12_RESOURCE_DESC msaaRTDesc = {};
        msaaRTDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        msaaRTDesc.Alignment = 0;
        msaaRTDesc.Width = window->Width();
        msaaRTDesc.Height = window->Height();
        msaaRTDesc.DepthOrArraySize = 1;
        msaaRTDesc.MipLevels = 1;
        msaaRTDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        msaaRTDesc.SampleDesc.Count = samples;
        msaaRTDesc.SampleDesc.Quality = quality;
        msaaRTDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        msaaRTDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        // descreve valores para limpeza da render target
        D3D12_CLEAR_VALUE msaaOptmizedClear = {};
        msaaOptmizedClear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        msaaOptmizedClear.Color[0] = bgColor[0];
        msaaOptmizedClear.Color[1] = bgColor[1];
        msaaOptmizedClear.Color[2] = bgColor[2];
        msaaOptmizedClear.Color[3] = bgColor[3];

        // propriedades da heap da render target 
        D3D12_HEAP_PROPERTIES msaaHeapProperties = {};
        msaaHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        msaaHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        msaaHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        msaaHeapProperties.CreationNodeMask = 1;
        msaaHeapProperties.VisibleNodeMask = 1;

        // cria render target para MSAA
        ThrowIfFailed(device->CreateCommittedResource(
            &msaaHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &msaaRTDesc,
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
            &msaaOptmizedClear,
            IID_PPV_ARGS(&msaaRenderTarget)
        ));

        // descrição da heap de render target
        D3D12_DESCRIPTOR_HEAP_DESC msaaRenderTargetHeapDesc = {};
        msaaRenderTargetHeapDesc.NumDescriptors = 1;
        msaaRenderTargetHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        ThrowIfFailed(device->CreateDescriptorHeap(&msaaRenderTargetHeapDesc, IID_PPV_ARGS(&msaaRenderTargetHeap)));

        // descrição da view de render target
        D3D12_RENDER_TARGET_VIEW_DESC msaaRTVDesc = {};
        msaaRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        msaaRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

        // cria render target view
        device->CreateRenderTargetView(
            msaaRenderTarget,
            &msaaRTVDesc,
            msaaRenderTargetHeap->GetCPUDescriptorHandleForHeapStart());

        // descrição do depth stencil buffer para MSAA
        D3D12_RESOURCE_DESC msaaDSDesc = {};
        msaaDSDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        msaaDSDesc.Alignment = 0;
        msaaDSDesc.Width = window->Width();
        msaaDSDesc.Height = window->Height();
        msaaDSDesc.DepthOrArraySize = 1;
        msaaDSDesc.MipLevels = 1;
        msaaDSDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        msaaDSDesc.SampleDesc.Count = samples;
        msaaDSDesc.SampleDesc.Quality = quality;
        msaaDSDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        msaaDSDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        // descreve valores de limpeza do depth stencil buffer
        D3D12_CLEAR_VALUE msaaDepthOptimizedClearValue = {};
        msaaDepthOptimizedClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        msaaDepthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        msaaDepthOptimizedClearValue.DepthStencil.Stencil = 0;

        // cria depth stencil buffer para MSAA
        ThrowIfFailed(device->CreateCommittedResource(
            &msaaHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &msaaDSDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &msaaDepthOptimizedClearValue,
            IID_PPV_ARGS(&msaaDepthStencil)
        ));

        // descrição da heap de depth stencil
        D3D12_DESCRIPTOR_HEAP_DESC msaaDepthStencilHeapDesc = {};
        msaaDepthStencilHeapDesc.NumDescriptors = 1;
        msaaDepthStencilHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        ThrowIfFailed(device->CreateDescriptorHeap(&msaaDepthStencilHeapDesc, IID_PPV_ARGS(&msaaDepthStencilHeap)));

        // descrição da view de depth stencil
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;

        // cria depth stencil view
        device->CreateDepthStencilView(
            msaaDepthStencil,
            &dsvDesc,
            msaaDepthStencilHeap->GetCPUDescriptorHandleForHeapStart());
    }
}

// -----------------------------------------------------------------------------

void Graphics::Clear()
{
    if (antialiasing)
    {
        // transição de estado da render target
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = msaaRenderTarget;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        // renderiza a cena na render target de MSAA
        D3D12_CPU_DESCRIPTOR_HANDLE rtDescriptor = msaaRenderTargetHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE dsDescriptor = msaaDepthStencilHeap->GetCPUDescriptorHandleForHeapStart();

        // limpa a render target e o depth/stencil buffer 
        commandList->ClearRenderTargetView(rtDescriptor, bgColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsDescriptor, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        // liga render target e depth stencil ao estágio Ouput Merger
        commandList->OMSetRenderTargets(1, &rtDescriptor, FALSE, &dsDescriptor);
    }
    else
    {
        // transição de estado da render target 
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = renderTargets[bufferIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        // indica qual buffer será usado na renderização  
        D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = renderTargetHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE dsHandle = depthStencilHeap->GetCPUDescriptorHandleForHeapStart();
        rtHandle.ptr += SIZE_T(bufferIndex) * SIZE_T(rtDescriptorSize);

        // limpa a render target e o depth/stencil buffer
        commandList->ClearRenderTargetView(rtHandle, bgColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        // liga render target e depth stencil ao estágio Ouput Merger
        commandList->OMSetRenderTargets(1, &rtHandle, true, &dsHandle);
    }

    // ajusta a viewport e retângulo de corte
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
}

// -----------------------------------------------------------------------------

void Graphics::Present()
{
    if (antialiasing)
    {
        // transição de estado das render targets 
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[0].Transition.pResource = msaaRenderTarget;
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[1].Transition.pResource = renderTargets[bufferIndex];
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_DEST;
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        // executa transição de estados
        commandList->ResourceBarrier(2, barriers);

        // coloca resultado com anti-aliasing na render target da swap chain
        commandList->ResolveSubresource(
            renderTargets[bufferIndex],
            0,
            msaaRenderTarget,
            0,
            DXGI_FORMAT_R8G8B8A8_UNORM);

        // transição de estado da render target para apresentação
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = renderTargets[bufferIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }
    else
    {
        // transição de estado da render target para apresentação
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = renderTargets[bufferIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }

    // fecha lista de comandos
    commandList->Close();

    // envia os comandos da lista para execução na GPU
    ID3D12CommandList* cmdsLists[] = { commandList };
    commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // apresenta frame
    swapChain->Present(vSync, 0);

    // espera a liberação de um alocador
    SyncToGpu();
}

// ------------------------------------------------------------------------------

void Graphics::WaitForGpu()
{
    // insere uma cerca na fila de comandos da GPU
    ThrowIfFailed(commandQueue->Signal(fence, fenceValue[bufferIndex]));

    // espera a GPU completar todos os comandos anteriores
    ThrowIfFailed(fence->SetEventOnCompletion(fenceValue[bufferIndex], fenceEvent));
    WaitForSingleObject(fenceEvent, INFINITE);

    // incrementa o valor da cerca
    fenceValue[bufferIndex]++;
}

// ------------------------------------------------------------------------------

void Graphics::SyncToGpu()
{
    // insere uma cerca na fila de comandos da GPU
    const ullong currentFenceValue = fenceValue[bufferIndex];
    ThrowIfFailed(commandQueue->Signal(fence, currentFenceValue));

    // troca de buffer
    bufferIndex = (bufferIndex + 1) % bufferCount;

    // espera a GPU completar os comandos do alocador que desejamos reutilizar
    if (fence->GetCompletedValue() < fenceValue[bufferIndex])
    {
        // aciona evento quando a GPU atingir a cerca  
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue[bufferIndex], fenceEvent));

        // espera até o evento ser acionado
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    // ajusta o valor da cerca para o próximo frame
    fenceValue[bufferIndex] = currentFenceValue + 1;

    // reutiliza alocador de comandos 
    commandAlloc[bufferIndex]->Reset();

    // reutiliza lista de comandos
    commandList->Reset(commandAlloc[bufferIndex], nullptr);
}

// -----------------------------------------------------------------------------

void Graphics::Allocate(uint type, ullong sizeInBytes, ID3D12Resource** resource)
{
    // propriedades da memória
    D3D12_HEAP_PROPERTIES bufferProp = {};
    bufferProp.Type = (type == GPU) ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;
    bufferProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    bufferProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    bufferProp.CreationNodeMask = 1;
    bufferProp.VisibleNodeMask = 1;

    // descrição do buffer 
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = sizeInBytes;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // estado inicial do recurso
    D3D12_RESOURCE_STATES initState =
        (type == GPU) ?
        D3D12_RESOURCE_STATE_COMMON :
        D3D12_RESOURCE_STATE_GENERIC_READ;

    // cria um buffer para o recurso
    ThrowIfFailed(device->CreateCommittedResource(
        &bufferProp,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        initState,
        nullptr,
        IID_PPV_ARGS(resource)));
}

// -----------------------------------------------------------------------------

void Graphics::Copy(const void* data, ullong sizeInBytes, ID3D12Resource* bufferUpload, ID3D12Resource* bufferGPU)
{
    // ----------------------------------------------------------------------------------
    // Copia dados para o buffer padrão da GPU (Memória de Vídeo ou Compartilhada)
    // ----------------------------------------------------------------------------------
    //
    //  Para copiar dados para a GPU:
    //  - primeiro copia-se os dados para a heap intermediária de upload
    //  - depois usando ID3D12CommandList::CopyBufferRegion copia-se de upload para a GPU
    //
    // ----------------------------------------------------------------------------------

    // descreve os dados que serão copiados
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = data;
    subResourceData.RowPitch = sizeInBytes;
    subResourceData.SlicePitch = sizeInBytes;

    // descreve o layout da memória de vídeo (GPU)
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts;
    uint numRows;
    ullong rowSizesInBytes;
    ullong requiredSize = 0;

    D3D12_RESOURCE_DESC bufferGPUDesc = bufferGPU->GetDesc();

    // pega layout da memória de vídeo
    device->GetCopyableFootprints(
        &bufferGPUDesc,
        0, 1, 0, &layouts, &numRows,
        &rowSizesInBytes, &requiredSize);

    // --------------------------------------
    // Copia dados no Upload Buffer
    // --------------------------------------

    // trava memória do upload buffer para acesso exclusivo 
    BYTE* pData;
    bufferUpload->Map(0, nullptr, (void**)&pData);

    // descreve o destino de uma operação de cópia
    D3D12_MEMCPY_DEST DestData =
    {
        pData + layouts.Offset,
        layouts.Footprint.RowPitch,
        layouts.Footprint.RowPitch * ullong(numRows)
    };

    // copia dados no upload buffer
    for (uint z = 0; z < layouts.Footprint.Depth; ++z)
    {
        // endereço de destino
        BYTE * destSlice = (BYTE*)(DestData.pData) + DestData.SlicePitch * z;

        // endereço da fonte
        const BYTE* srcSlice = (const BYTE*)(subResourceData.pData) + subResourceData.SlicePitch * z;
        
        // faz cópia linha a linha
        for (uint y = 0; y < numRows; ++y)
            memcpy(destSlice + DestData.RowPitch * y,
                   srcSlice + subResourceData.RowPitch * y,
                   (size_t) min(rowSizesInBytes, sizeInBytes));
    }

    // libera trava de memória do upload buffer 
    bufferUpload->Unmap(0, nullptr);

    // ----------------------------------------
    // Copia dados do Upload para o GPU Buffer
    // ----------------------------------------

    // altera estado da memória da GPU (de leitura para escrita)
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = bufferGPU;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    // copia dados do upload buffer para o buffer da GPU
    commandList->CopyBufferRegion(
        bufferGPU,
        0,
        bufferUpload,
        layouts.Offset,
        layouts.Footprint.Width);

    // altera estado da memória da GPU (de escrita para leitura)
    barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = bufferGPU;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);
}

// -----------------------------------------------------------------------------

void Graphics::Copy(D3D12_SUBRESOURCE_DATA* subresource, ullong sizeInBytes, ID3D12Resource* bufferUpload, ID3D12Resource* bufferGPU)
{
    // ----------------------------------------------------------------------------------
    //  Copia textura para a memória de vídeo
    // ----------------------------------------------------------------------------------
    //  - primeiro copia-se os dados da textura (subresource) para o buffer de upload
    //  - depois usando ID3D12CommandList::CopyTextureRegion copia-se de upload para GPU
    // ----------------------------------------------------------------------------------

    // descritor do buffer na GPU
    D3D12_RESOURCE_DESC bufferGPUDesc = bufferGPU->GetDesc();

    // número de subrecursos (mipmaps)
    const UINT NumSubresources = bufferGPUDesc.DepthOrArraySize * bufferGPUDesc.MipLevels;

    // tamanho da memória para guardar informações de Layout, numRows e rowSizesInBytes
    UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;

    // aloca memória para guardar informações de Layout, numRows e rowSizesInBytes
    void* layoutMemory = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
    if (layoutMemory == NULL)
    {
        return;
    }

    // descreve o layout da memória de vídeo (GPU)
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(layoutMemory);
    UINT64* rowSizesInBytes = reinterpret_cast<UINT64*>(layouts + NumSubresources);
    UINT* numRows = reinterpret_cast<UINT*>(rowSizesInBytes + NumSubresources);

    // tamanho da alocação no Upload Buffer
    ullong requiredSize = 0;

    // pega layout da memória de vídeo
    device->GetCopyableFootprints(
        &bufferGPUDesc,
        0, NumSubresources, 0, layouts, numRows,
        rowSizesInBytes, &requiredSize);

    // --------------------------------------
    // Copia Textura no Upload Buffer
    // --------------------------------------

    // trava memória do upload buffer para acesso exclusivo 
    BYTE* pData;
    bufferUpload->Map(0, nullptr, (void**)&pData);

    // copia cada subrecurso para memória de upload
    for (UINT i = 0; i < NumSubresources; ++i)
    {
        // tamanho da linha ultrapassa o tamanho máximo
        if (rowSizesInBytes[i] > (SIZE_T)-1) return;

        // descreve o destino de uma operação de cópia
        D3D12_MEMCPY_DEST DestData =
        {
            pData + layouts[i].Offset,
            layouts[i].Footprint.RowPitch,
            layouts[i].Footprint.RowPitch * numRows[i]
        };

        // copia dados do subresource no upload buffer
        for (uint z = 0; z < layouts[i].Footprint.Depth; ++z)
        {
            // endereço de destino
            BYTE* destSlice = (BYTE*)(DestData.pData) + DestData.SlicePitch * z;

            // endereço da fonte
            const BYTE* srcSlice = (const BYTE*)(subresource[i].pData) + subresource[i].SlicePitch * z;

            // faz cópia linha a linha
            for (uint y = 0; y < numRows[i]; ++y)
                memcpy(destSlice + DestData.RowPitch * y,
                    srcSlice + subresource[i].RowPitch * y,
                    (size_t)rowSizesInBytes[i]);
        }
    }

    // libera trava de memória do upload buffer 
    bufferUpload->Unmap(0, nullptr);

    // --------------------------------------
    // Copia dados do Upload para GPU Buffer
    // --------------------------------------

    // altera estado da memória da GPU (de leitura para escrita)
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = bufferGPU;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    for (UINT i = 0; i < NumSubresources; ++i)
    {
        // define destino da operação de cópia
        D3D12_TEXTURE_COPY_LOCATION Dst = {};
        Dst.pResource = bufferGPU;
        Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        Dst.SubresourceIndex = i;

        // define origem da operação de cópia
        D3D12_TEXTURE_COPY_LOCATION Src = {};
        Src.pResource = bufferUpload;
        Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        Src.PlacedFootprint = layouts[i];

        // copia textura do upload buffer para a GPU
        commandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
    }

    // altera estado da memória da GPU (de escrita para leitura)
    barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = bufferGPU;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    HeapFree(GetProcessHeap(), 0, layoutMemory);
}

// -----------------------------------------------------------------------------

void Graphics::Resize(int width, int height)
{
    // espera a GPU finalizar comandos pendentes
    WaitForGpu();

    // ------------------------------
    // Swap Chain / Render Target
    // ------------------------------

     // libera render targets buffers
    for (uint i = 0; i < bufferCount; ++i)
    {
        if (renderTargets[i])
            renderTargets[i]->Release();

        // reseta cercas para o valor atual
        fenceValue[i] = fenceValue[bufferIndex];
    }

    // descrição da swap chain atual
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChain->GetDesc(&swapChainDesc);

    // redimensiona a swap chain
    ThrowIfFailed(swapChain->ResizeBuffers(
        bufferCount,
        width,
        height,
        swapChainDesc.BufferDesc.Format,
        swapChainDesc.Flags));

    // reinicia sempre no primeiro buffer
    bufferIndex = 0;

    // pega um Handle para o início da Heap
    D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = renderTargetHeap->GetCPUDescriptorHandleForHeapStart();

    // recria descritor (view) para cada Render Target 
    for (uint i = 0; i < bufferCount; ++i)
    {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
        device->CreateRenderTargetView(renderTargets[i], nullptr, rtHandle);
        rtHandle.ptr += rtDescriptorSize;
    }

    // ------------------------------
    // Depth Stencil 
    // ------------------------------

    // configuração depth/stencil atual
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc = depthStencil->GetDesc();

    // atualiza tamanho do buffer Depth/Stencil
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = height;

    // propriedades da Heap do buffer Depth/Stencil
    D3D12_HEAP_PROPERTIES dsHeapProperties = {};
    dsHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    dsHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    dsHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    dsHeapProperties.CreationNodeMask = 1;
    dsHeapProperties.VisibleNodeMask = 1;

    // valores para limpeza do Depth/Stencil buffer
    D3D12_CLEAR_VALUE optmizedClear = {};
    optmizedClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optmizedClear.DepthStencil.Depth = 1.0f;
    optmizedClear.DepthStencil.Stencil = 0;

    // libera buffer 
    if (depthStencil)
        depthStencil->Release();

    // recria um buffer Depth/Stencil
    ThrowIfFailed(device->CreateCommittedResource(
        &dsHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optmizedClear,
        IID_PPV_ARGS(&depthStencil)));

    // pega um Handle para o início da Heap
    D3D12_CPU_DESCRIPTOR_HANDLE dsHandle = depthStencilHeap->GetCPUDescriptorHandleForHeapStart();

    // cria um descritor (view) de Depth/Stencil para o mip nível 0
    device->CreateDepthStencilView(depthStencil, nullptr, dsHandle);

    // ---------------------------------------------------
    // Viewport e Scissor Rectangle
    // ---------------------------------------------------

    // atualiza a viewport
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);

    // atualiza o retângulo de corte
    scissorRect.right = static_cast<LONG>(width);
    scissorRect.bottom = static_cast<LONG>(height);

    // ---------------------------------------------------
    // Multi-Sample Anti-Aliasing (MSAA)
    // ---------------------------------------------------

    if (antialiasing)
    {
        // descrição da render target para MSAA
        D3D12_RESOURCE_DESC msaaRTDesc = {};
        msaaRTDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        msaaRTDesc.Alignment = 0;
        msaaRTDesc.Width = width;
        msaaRTDesc.Height = height;
        msaaRTDesc.DepthOrArraySize = 1;
        msaaRTDesc.MipLevels = 1;
        msaaRTDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        msaaRTDesc.SampleDesc.Count = samples;
        msaaRTDesc.SampleDesc.Quality = quality;
        msaaRTDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        msaaRTDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        // descreve valores para limpeza da render target
        D3D12_CLEAR_VALUE msaaOptmizedClear = {};
        msaaOptmizedClear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        msaaOptmizedClear.Color[0] = bgColor[0];
        msaaOptmizedClear.Color[1] = bgColor[1];
        msaaOptmizedClear.Color[2] = bgColor[2];
        msaaOptmizedClear.Color[3] = bgColor[3];

        // propriedades da heap da render target 
        D3D12_HEAP_PROPERTIES msaaHeapProperties = {};
        msaaHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        msaaHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        msaaHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        msaaHeapProperties.CreationNodeMask = 1;
        msaaHeapProperties.VisibleNodeMask = 1;

        // libera buffer
        if (msaaRenderTarget)
            msaaRenderTarget->Release();

        // recria render target para MSAA
        ThrowIfFailed(device->CreateCommittedResource(
            &msaaHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &msaaRTDesc,
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
            &msaaOptmizedClear,
            IID_PPV_ARGS(&msaaRenderTarget)
        ));

        // descrição da view de render target
        D3D12_RENDER_TARGET_VIEW_DESC msaaRTVDesc = {};
        msaaRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        msaaRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

        // recria render target view
        device->CreateRenderTargetView(
            msaaRenderTarget,
            &msaaRTVDesc,
            msaaRenderTargetHeap->GetCPUDescriptorHandleForHeapStart());

        // descrição do depth stencil buffer para MSAA
        D3D12_RESOURCE_DESC msaaDSDesc = {};
        msaaDSDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        msaaDSDesc.Alignment = 0;
        msaaDSDesc.Width = width;
        msaaDSDesc.Height = height;
        msaaDSDesc.DepthOrArraySize = 1;
        msaaDSDesc.MipLevels = 1;
        msaaDSDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        msaaDSDesc.SampleDesc.Count = samples;
        msaaDSDesc.SampleDesc.Quality = quality;
        msaaDSDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        msaaDSDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        // valores de limpeza do depth stencil buffer
        D3D12_CLEAR_VALUE msaaDepthOptimizedClearValue = {};
        msaaDepthOptimizedClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        msaaDepthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        msaaDepthOptimizedClearValue.DepthStencil.Stencil = 0;

        // libera buffer
        if (msaaDepthStencil)
            msaaDepthStencil->Release();

        // recria depth stencil buffer para MSAA
        ThrowIfFailed(device->CreateCommittedResource(
            &msaaHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &msaaDSDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &msaaDepthOptimizedClearValue,
            IID_PPV_ARGS(&msaaDepthStencil)
        ));

        // descrição da view de depth stencil
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;

        // recria depth stencil view
        device->CreateDepthStencilView(
            msaaDepthStencil,
            &dsvDesc,
            msaaDepthStencilHeap->GetCPUDescriptorHandleForHeapStart());
    }
}

// -----------------------------------------------------------------------------
