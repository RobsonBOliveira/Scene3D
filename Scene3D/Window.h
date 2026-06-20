/**********************************************************************************
// Window (Arquivo de Cabeçalho)
// 
// Criação:     19 Mai 2007
// Atualização: 26 Jul 2025
// Compilador:	Visual C++ 2022
//
// Descrição:   Abstrai os detalhes de configuração de uma janela 
//
**********************************************************************************/

#ifndef DXUT_WINDOW_H
#define DXUT_WINDOW_H

// ---------------------------------------------------------------------------------
// Inclusões

#define WIN32_LEAN_AND_MEAN

#include "Types.h"       // tipos personalizados da biblioteca
#include <windows.h>     // inclui funções do windows
#include <windowsx.h>    // inclui extensões do windows
#include <string>        // inclui a classe string
using std::string;       // permite usar o tipo string sem std::

// ---------------------------------------------------------------------------------
// Constantes globais e enumerações

enum WindowModes {FULLSCREEN, UNLOCKED, LOCKED, ASPECTRATIO };

// ---------------------------------------------------------------------------------

class Window
{
private:
    long         monitorX;                      // posição x do monitor no desktop virtual
    long         monitorY;                      // posição y do monitor no desktop virtual
    uint         screenDpi;                     // dpi atual da tela
    float        windowScale;                   // escala da janela para o dpi atual
    HDC          hdc;                           // contexto do dispositivo
                                                
    RECT         clientRect;                    // área cliente da janela
    int          clientWidth;                   // largura da área cliente da janela
    int          clientHeight;                  // altura da área cliente da janela
    int          clientCenterX;                 // centro da janela no eixo x
    int          clientCenterY;                 // centro da janela no eixo y
    float        clientAspectRatio;             // aspect ratio da janela
                                                
    HWND         windowId;                      // identificador da janela
    HICON        windowIcon;                    // ícone da janela
    HCURSOR      windowCursor;                  // cursor da janela
    COLORREF     windowColor;                   // cor de fundo da janela
    string       windowTitle;                   // nome da barra de título
    DWORD        windowStyle;                   // estilo da janela 
    int          windowMode;                    // modo tela cheia, em janela ou sem borda
    int          windowPosX;                    // posição inicial da janela no eixo x
    int          windowPosY;                    // posição inicial da janela no eixo y
    int          windowWidth;                   // largura da janela
    int          windowHeight;                  // altura da janela 
    int          windowMinWidth;                // largura mínima da janela
    int          windowMinHeight;               // altura mínima da janela
    float        windowAspectRatio;             // aspect ratio da janela
                                                
    int          bordersWidth;                  // largura das bordas e barras da janela
    int          bordersHeight;                 // altura das bordas e barras da janela
                                                
    static int   screenWidth;                   // largura da tela
    static int   screenHeight;                  // altura da tela
                                                
public:                                         
    static void (*inFocus)();                   // executar quando a janela ganhar de volta o foco
    static void (*lostFocus)();                 // executar quando a janela perder o foco

    Window();                                   // construtor
    ~Window();                                  // destrutor
                                                
    HWND Id();                                  // retorna identificador da janela
    int Width();                                // retorna largura da área cliente
    int Height();                               // retorna altura da área cliente
    int Mode() const;                           // retorna modo atual da janela
    float AspectRatio() const;                  // retorna aspect ratio da área cliente
    int CenterX() const;                        // retorna centro da área cliente no eixo x
    int CenterY() const;                        // retorna centro da área cliente no eixo y
    string Title() const;                       // retorna título da janela
    COLORREF Color();                           // retorna cor de fundo da janela
                                                
    int WindowWidth() const;                    // retorna largura da janela 
    int WindowHeight() const;                   // retorna altura da janela
    int MinWidth() const;                       // retorna largura mínima da janela
    int MinHeight() const;                      // retorna altura mínima da janela
    float WindowAspectRatio() const;            // retorna aspect ratio da janela
                                                
    void Icon(const char* icon);                // define o ícone da janela
    void Cursor(const char* cursor);            // define o cursor da janela
    void Title(const string title);             // define título da janela 
    void Size(int width, int height);           // define tamanho da janela

    void ClientResize(int width, int height);   // ajusta tamanho da área cliente
    void ChangeDpi(uint dpi, RECT* rect);       // ajusta janela para o novo dpi
    void Sizing(RECT* rect, WPARAM wParam);     // ajusta tamanho da janela
    void Mode(int mode);                        // define modo da janela
    void Color(int r, int g, int b);            // define cor de fundo da janela
    void HideCursor(bool hide);                 // habilita ou desabilita a exbição do cursor
                                                
    void Close();                               // fecha janela
    void Clear();                               // limpa área cliente
    bool Create();                              // cria janela com a configuração estabelecida
                                                
    void InFocus(void(*func)());                // altera função executada ao ganhar foco
    void LostFocus(void(*func)());              // altera função executada na perda do foco

    // trata eventos do Windows
    static LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

// ---------------------------------------------------------------------------------

// Funções Membro Inline

// retorna o identificador da janela do jogo
inline HWND Window::Id()
{ return windowId; }

// retorna largura da área cliente
inline int Window::Width() 
{ return clientWidth;  }

// retorna altura da área cliente
inline int Window::Height() 
{ return clientHeight; }

// retorna o modo atual da janela
inline int Window::Mode() const
{ return windowMode; }

// retorna o centro da área cliente no eixo horizontal
inline int Window::CenterX() const
{ return clientCenterX; }

// retorna o centro da área cliente no eixo vertical
inline int Window::CenterY() const
{ return clientCenterY; }

// retorna título da janela
inline string Window::Title() const
{ return windowTitle; }

// retorna a cor de fundo da janela
inline COLORREF Window::Color()
{ return windowColor; }

// retorna largura da janela 
inline int Window::WindowWidth() const
{ return windowWidth; }

// retorna altura da janela 
inline int Window::WindowHeight() const
{ return windowHeight; }

// retorna largura mínima da janela
inline int Window::MinWidth() const
{ return windowMinWidth; }

// retorna altura mínima da janela
inline int Window::MinHeight() const
{ return windowMinHeight; }

// retorna aspect ratio da área cliente da janela
inline float Window::AspectRatio() const
{ return clientAspectRatio; }

// retorna aspect ratio da janela
inline float Window::WindowAspectRatio() const
{ return windowAspectRatio; }

// ----------------------------------------------------------

// define o ícone da janela
inline void Window::Icon(const char * icon)    
{ windowIcon = LoadIcon(GetModuleHandle(NULL), icon); }

// define o cursor da janela
inline void Window::Cursor(const char * cursor)
{ windowCursor = LoadCursor(GetModuleHandle(NULL), cursor); }

// define o título da janela 
inline void Window::Title(const string title)
{ windowTitle = title; }

// define a cor de fundo da janela
inline void Window::Color(int r, int g, int b)    
{ windowColor = RGB(r,g,b); }

// habilita ou desabilita a exbição do cursor do mouse
inline void Window::HideCursor(bool hide)
{ ShowCursor(!hide); }

// ----------------------------------------------------------

// fecha a janela e sai da aplicação 
inline void Window::Close()
{ PostMessage(windowId, WM_DESTROY, 0, 0); }

// limpa a área cliente
inline void Window::Clear()
{ FillRect(hdc, &clientRect, CreateSolidBrush(Color())); }

// ----------------------------------------------------------

// altera função executada ao ganhar o foco
inline void Window::InFocus(void(*func)())                
{ inFocus = func; }

// altera função executada ao perder o foco
inline void Window::LostFocus(void(*func)())
{ lostFocus = func; }

// ---------------------------------------------------------------------------------

#endif