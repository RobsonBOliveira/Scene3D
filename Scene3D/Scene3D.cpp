/**********************************************************************************
// Scene3D (C�digo Fonte)
//
// Cria��o:     02 Out 2023
// Atualiza��o: 18 Jul 2025
// Compilador:  Visual C++ 2022
//
// Descri��o:   Demonstra os MipMaps do formato DDS
//
**********************************************************************************/

#include "Scene3D.h"

// ------------------------------------------------------------------------------

Timer Scene3D::timer;

// ------------------------------------------------------------------------------

void Scene3D::Init()
{
    // -----------------------
    // Inicializa��es da Cena
    // -----------------------

    // posi��o inicial da c�mera
    orbitcam = { 0.0f, 1.2f, 8.0f };

    // zoom out m�ximo
    orbitcam.MaxRadius(500.0f);

    // inicializa a matriz de proje��o
    XMStoreFloat4x4(&Proj, XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.0f),
        window->AspectRatio(),
        1.0f, 1000.0f));

    // constant buffer
    sceneBuffer = new ConstantBuffer<Scene>();

    // ------------------------------
    // Geometria: V�rtices e �ndices
    // ------------------------------

    Grid grid(16.0f, 9.0f, 2, 2);
    Box box(7.0f, 0.1f, 4.0f);

    // ----------------------
    // Cria��o dos Materiais
    // ----------------------

    Material asfalt;
    asfalt.Albedo = XMFLOAT4(1.00f, 1.00f, 1.00f, 1.0f);
    asfalt.FresnelR0 = XMFLOAT3(0.15f, 0.15f, 0.15f);
    asfalt.Roughness = 0.8f;

    Material wood;
    wood.Albedo = XMFLOAT4(0.87f, 0.82f, 0.73f, 1.0f);
    wood.FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    wood.Roughness = 0.80f;

    Material plastic;
    plastic.Albedo = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    plastic.FresnelR0 = XMFLOAT3(0.04f, 0.04f, 0.04f);
    plastic.Roughness = 0.4f;

    // --------------------
    // Cria��o dos Objetos 
    // --------------------

    Object table;
    XMStoreFloat4x4(&table.world, 
        XMMatrixTranslation(0.0f, 1.0f, 2.05f));
    table.mesh = new Mesh(box);
    table.vbuffer = new VertexBuffer<Vertex>(box);
    table.ibuffer = new IndexBuffer<uint>(box);
    table.cbuffer = new ConstantBuffer<Constants>();
    table.material = new ConstantBuffer<Material>(&wood);
    table.texture = new Texture("Resources/Wood.jpg");
    scene.push_back(table);

    Object tv;
    XMStoreFloat4x4(&tv.world, 
        XMMatrixTranslation(0.0f, 0.3f, -3.3f) *
        XMMatrixRotationX(XMConvertToRadians(90.0f)) *
        XMMatrixScaling(0.8f, 0.8f, 0.8f));
    tv.mesh = new Mesh(box);
    tv.vbuffer = new VertexBuffer<Vertex>(box);
    tv.ibuffer = new IndexBuffer<uint>(box);
    tv.cbuffer = new ConstantBuffer<Constants>();
    tv.material = new ConstantBuffer<Material>(&plastic);
    tv.texture = new Texture("Resources/Plastic.jpg");
    scene.push_back(tv);

    Object panel;
    XMStoreFloat4x4(&panel.world, 
        XMMatrixTranslation(0.0f, 1.0f, 9.0f) *
        XMMatrixRotationX(XMConvertToRadians(90.0f)) *
        XMMatrixRotationZ(XMConvertToRadians(180.0f)) *
        XMMatrixScaling(0.3f, 0.3f, 0.3f));
    panel.mesh = new Mesh(grid);
    panel.vbuffer = new VertexBuffer<Vertex>(grid);
    panel.ibuffer = new IndexBuffer<uint>(grid);
    panel.cbuffer = new ConstantBuffer<Constants>();
    panel.material = new ConstantBuffer<Material>(&plastic);
    panel.texture = new Texture("Resources/Malenia.jpg");
    scene.push_back(panel);

    // ---------------------

    BuildRootSignature();
    BuildPipelineState();

    // ----------------------

    timer.Reset();
    timer.Stop();
}

// ------------------------------------------------------------------------------

void Scene3D::Update()
{
    // sai com o pressionamento do ESC
    if (input->KeyPress(VK_ESCAPE))
        window->Close();

    // -----------------------
    // Controles da Aplica��o
    // -----------------------

    // rota��o do objeto
    if (input->KeyPress('R'))
    {
        rotating = !rotating;

        if (rotating)
            timer.Start();
        else
            timer.Stop();
    }

    // modo s�lido/wireframe
    if (input->KeyPress('S'))
    {
        solid = !solid;

        if (solid)
            pipelineState = pipelineSolid;
        else
            pipelineState = pipelineWire;
    }

    // ------------------
    // C�mera e Proje��o 
    // ------------------

    // movimenta a c�mera com o mouse
    orbitcam.Update();

    // constr�i a matriz de visualiza��o
    XMVECTOR pos = XMVectorSet(orbitcam.x, orbitcam.y, orbitcam.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

    // carrega matriz de proje��o
    XMMATRIX proj = XMLoadFloat4x4(&Proj);

    // constr�i matriz combinada
    XMMATRIX ViewProj = view * proj;

    // vetor na dire��o do observador
    XMFLOAT3 Eye = XMFLOAT3(orbitcam.x, orbitcam.y, orbitcam.z);

    // -------------------------
    // Controles de Ilumina��o
    // -------------------------

    if (input->KeyPress('A'))
        Amb = !Amb;
    if (input->KeyPress('D'))
        Dif = !Dif;
    if (input->KeyPress('E'))
        Spe = !Spe;
    if (input->KeyPress('L'))
        Lam = !Lam;
    if (input->KeyPress('M'))
        Mat = !Mat;
    if (input->KeyPress('T'))
        Tex = !Tex;    

    float delta = float(2.0 * frameTime);

    // regula intensidade da luz
    if (input->KeyDown(VK_OEM_PLUS))
    {
        Strength += delta;
        if (Strength > 5.0f)
            Strength = 5.0f;
    }
    if (input->KeyDown(VK_OEM_MINUS))
    {
        Strength -= delta;
        if (Strength < 0.2f)
            Strength = 0.2f;
    }

    // movimenta luz direcional
    if (input->KeyDown(VK_LEFT))
        thetaL -= delta;
    if (input->KeyDown(VK_RIGHT))
        thetaL += delta;
    if (input->KeyDown(VK_UP))
        phiL -= delta;
    if (input->KeyDown(VK_DOWN))
        phiL += delta;

    // restringe o �ngulo phi da luz entre ]0-180[ graus
    phiL = max(0.001f, min(XM_PI - 0.001f, phiL));

    // converte coordenadas esf�ricas para cartesianas
    float x = radiusL * sinf(phiL) * cosf(thetaL);
    float z = radiusL * sinf(phiL) * sinf(thetaL);
    float y = radiusL * cosf(phiL);

    // define dire��o da luz
    XMVECTOR lightDir = XMVector3Normalize(XMVectorSet(x, y, z, 0.0f));

    // ------------------
    // Constant Buffers 
    // ------------------

    Light light;
    XMStoreFloat3(&light.Direction, lightDir);
    light.Strength = XMFLOAT3(Strength, Strength, Strength);

    Scene sceneConstants;
    XMStoreFloat4x4(&sceneConstants.ViewProj, XMMatrixTranspose(ViewProj));
    sceneConstants.Ambient = XMFLOAT4(0.00f, 0.00f, 0.00f, 1.0f);
    sceneConstants.Lights = light;
    sceneConstants.Eye = Eye;
    sceneConstants.Amb = Amb;
    sceneConstants.Dif = Dif;
    sceneConstants.Spe = Spe;
    sceneConstants.Lam = Lam;
    sceneConstants.Mat = Mat;
    sceneConstants.Tex = Tex;

    // atualiza constant buffer da cena
    sceneBuffer->Copy(&sceneConstants);

    // atualiza constant buffer de cada objeto
    int i = 0;
    for (auto& obj : scene)
    {
        Constants constants;
        XMMATRIX World = XMLoadFloat4x4(&obj.world);
        XMStoreFloat4x4(&constants.World, XMMatrixTranspose(World));
        XMMATRIX TexTransform = XMMatrixTranslation(float(-0.25f * timer.Elapsed()), 0.0f, 0.0f);
        XMStoreFloat4x4(&constants.TexTransform, XMMatrixTranspose(TexTransform));
        constants.ObjIndex = i++;
        obj.cbuffer->Copy(&constants);
    }
}

// ------------------------------------------------------------------------------


void Scene3D::Draw()
{
    // limpa o backbuffer
    graphics->Clear();
    
    // ajustes do pipeline para desenho do objeto
    graphics->CommandList()->SetPipelineState(pipelineState);
    graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
    graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (auto& obj : scene)
    {
        graphics->CommandList()->IASetVertexBuffers(0, 1, obj.vbuffer->View());
        graphics->CommandList()->IASetIndexBuffer(obj.ibuffer->View());
        graphics->CommandList()->SetDescriptorHeaps(1, obj.texture->Heap());
        graphics->CommandList()->SetGraphicsRootDescriptorTable(0, obj.texture->Table());
        graphics->CommandList()->SetGraphicsRootConstantBufferView(1, sceneBuffer->View());
        graphics->CommandList()->SetGraphicsRootConstantBufferView(2, obj.cbuffer->View());
        graphics->CommandList()->SetGraphicsRootConstantBufferView(3, obj.material->View());

        // desenha objeto
        graphics->CommandList()->DrawIndexedInstanced(
            obj.mesh->indexCount, 1,
            obj.mesh->startIndex,
            obj.mesh->baseVertex,
            0);
    }
 
    // apresenta o backbuffer na tela
    graphics->Present();    
}

// ------------------------------------------------------------------------------

void Scene3D::Finalize()
{
    // espera GPU finalizar comandos pendentes
    graphics->WaitForGpu();

    // libera mem�ria alocada
    rootSignature->Release();
    pipelineWire->Release();
    pipelineSolid->Release();

    for (auto& obj : scene)
    {
        delete obj.mesh;
        delete obj.vbuffer;
        delete obj.ibuffer;
        delete obj.cbuffer;
        delete obj.material;
        delete obj.texture;
    }

    delete sceneBuffer;
}

// ------------------------------------------------------------------------------
//                                  WinMain                                      
// ------------------------------------------------------------------------------

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    try
    {
        // cria motor
        Engine* engine = new Engine();

        // configura janela
        engine->window->Mode(ASPECTRATIO);
        engine->window->Size(1024, 720);
        engine->window->Color(25, 25, 25);
        engine->window->Title("Scene3D");
        engine->window->Icon("Icon");
        engine->window->Cursor("Cursor");
        engine->window->LostFocus(Scene3D::Pause);
        engine->window->InFocus(Scene3D::Resume);

        // configura gr�ficos
        engine->graphics->VSync(false);
        engine->graphics->MSaa(8);

        // executa a aplica��o
        engine->Start(new Scene3D());

        // finaliza motor
        delete engine;
    }
    catch (Error& e)
    {
        // exibe mensagem em caso de erro
        MessageBox(nullptr, e.ToString().data(), "Scene3D", MB_OK);
    }

    return 0;
}

// ----------------------------------------------------------------------------


