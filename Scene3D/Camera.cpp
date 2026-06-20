/**********************************************************************************
// OrbitCamera (Código Fonte)
//
// Criação:     21 Jun 2025
// Atualização: 23 Jun 2025
// Compilador:  Visual C++ 2022
//
// Descrição:   Câmera que orbita em torno da origem
//
**********************************************************************************/

#include "Camera.h"
#include "Engine.h"

// ------------------------------------------------------------------------------

OrbitCamera::OrbitCamera(float horz, float vert, float radi)
    : theta(horz)
    , phi(vert)
    , radius(radi)
{
    // mantém phi na faixa ]0-180[
    phi = max(0.001f, min(XM_PI - 0.001f, phi));

    // raio mínimo e máximo
    minRadius = radius / 2.0f;
    maxRadius = 3.0f * radius;

    // velocidade de movimentação
    moveSpeed = 0.250f;
    zoomSpeed = 0.005f;

    // última posição do mouse
    lastMouseX = (float) Engine::input->MouseX();
    lastMouseY = (float) Engine::input->MouseY();
}

// ------------------------------------------------------------------------------

void OrbitCamera::Update()
{
    // posição atual do mouse
    float mouseX = (float) Engine::input->MouseX();
    float mouseY = (float) Engine::input->MouseY();

    // ---------------------
    // Movimento da Câmera
    // ---------------------

    if (Engine::input->KeyDown(VK_LBUTTON))
    {
        // cada pixel corresponde a 'moveSpeed' graus
        float dx = XMConvertToRadians(moveSpeed * (mouseX - lastMouseX));
        float dy = XMConvertToRadians(moveSpeed * (mouseY - lastMouseY));

        // atualiza ângulos com base no deslocamento do mouse
        theta += dx;
        phi += dy;

        // restringe o ângulo de phi ]0-180[ graus
        phi = max(0.001f, min(XM_PI - 0.001f, phi));
    }

    // ---------------------
    // Zoom da Câmera
    // ---------------------
    else if (Engine::input->KeyDown(VK_RBUTTON))
    {
        // cada pixel corresponde a 'zoomSpeed' unidades
        float dy = zoomSpeed * (mouseY - lastMouseY);

        // atualiza o raio da câmera com base no deslocamento do mouse 
        radius += dy;

        // restringe o raio entre minRadius e maxRadius
        radius = max(minRadius, min(radius, maxRadius));
    }
    else if (wheelMouse = Engine::input->MouseWheel())
    {
        // atualiza o raio da câmera com base na rolagem do mouse 
        radius -= zoomSpeed * (wheelMouse);

        // restringe o raio entre minRadius e maxRadius
        radius = max(minRadius, min(radius, maxRadius));
    }

    // ------------------------
    // Coordenadas cartesianas
    // ------------------------

    // converte coordenadas esféricas para cartesianas
    x = radius * sinf(phi) * cosf(theta);
    z = radius * sinf(phi) * sinf(theta);
    y = radius * cosf(phi);

    // atualiza última posição do mouse
    lastMouseX = mouseX;
    lastMouseY = mouseY;
}

// ------------------------------------------------------------------------------