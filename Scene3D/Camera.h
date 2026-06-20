/**********************************************************************************
// OrbitCamera (Arquivo de Cabeçalho)
//
// Criação:     21 Jun 2025
// Atualização: 23 Jun 2025
// Compilador:  Visual C++ 2022
//
// Descrição:   Câmera que orbita em torno da origem
//
**********************************************************************************/

#ifndef DXUT_ORBIT_CAMERA_H
#define DXUT_ORBIT_CAMERA_H

#include <DirectXMath.h>
using namespace DirectX;

// --------------------------------------------------------------------------------

class OrbitCamera
{
private:
    float theta;                // coordenada esférica theta 
    float phi;                  // coordenada esférica phi
    float radius;               // coordenada esférica raio

    float minRadius;            // raio mínimo
    float maxRadius;            // raio máximo
    float moveSpeed;            // velocidade do deslocamento
    float zoomSpeed;            // velocidade do zoom
    
    float lastMouseX;           // última posição x do mouse
    float lastMouseY;           // última posição y do mouse
    short wheelMouse;           // rolagem do mouse

public:
    float x = 0;                // coordenada cartesiana x
    float y = 0;                // coordenada cartesiana y
    float z = 0;                // coordenada cartesiana z

    // construtor padrão
    OrbitCamera(float horz = 0, float vert = XM_PIDIV2, float radi = 5.0f);
    
    // atualiza com base no movimento do mouse
    void Update(); 

    void MinRadius(float min);  // define a distância mínima da origem
    void MaxRadius(float min);  // define a distância máxima da origem
    void MoveSpeed(float vel);  // define a velocidade de movimento
    void ZoomSpeed(float vel);  // define a velocidade do zoom
};

// ------------------------------------------------------------------------------
// Métodos Inline

inline void OrbitCamera::MinRadius(float min)
{ minRadius = min; }

inline void OrbitCamera::MaxRadius(float max)
{ maxRadius = max; }

inline void OrbitCamera::MoveSpeed(float vel)
{ moveSpeed = vel; }

inline void OrbitCamera::ZoomSpeed(float vel)
{ zoomSpeed = vel; }

// ------------------------------------------------------------------------------

#endif

