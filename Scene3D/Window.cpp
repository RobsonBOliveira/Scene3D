/**********************************************************************************
// Window (Código Fonte)
// 
// Criação:     19 Mai 2007
// Atualização: 26 Jul 2025
// Compilador:	Visual C++ 2022
//
// Descrição:   Abstrai os detalhes de configuração de uma janela
//
**********************************************************************************/

#include "Window.h"

// -------------------------------------------------------------------------------
// inicialização de membros estáticos da classe

void (*Window::inFocus)()     = nullptr;                                    // nenhuma ação ao ganhar foco
void (*Window::lostFocus)()   = nullptr;                                    // nenhuma ação ao perder foco
int Window::screenWidth       = 0;                                          // largura da tela
int Window::screenHeight      = 0;                                          // altura da tela

// -------------------------------------------------------------------------------
// construtor

Window::Window()
{
    // adiciona suporte a monitores com alto dpi
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    monitorX          = 0;                                                  // posição x do monitor no desktop virtual
    monitorY          = 0;                                                  // posição y do monitor no desktop virtual
    screenDpi         = GetDpiForSystem();                                  // dpi da tela
    windowScale       = screenDpi / float(USER_DEFAULT_SCREEN_DPI);         // escala da janela
    
    hdc               = { 0 };                                              // contexto do dispositivo  
    screenWidth       = GetSystemMetricsForDpi(SM_CXSCREEN, screenDpi);     // largura da tela
    screenHeight      = GetSystemMetricsForDpi(SM_CYSCREEN, screenDpi);     // altura da tela
    clientWidth       = screenWidth;                                        // janela inicia em tela cheia
    clientHeight      = screenHeight;                                       // janela inicia em tela cheia
    clientCenterX     = clientWidth/2;                                      // centro da área cliente no eixo x
    clientCenterY     = clientHeight/2;                                     // centro da área cliente no eixo y
    clientAspectRatio = clientWidth / float(clientHeight);                  // aspect ratio da área cliente
    clientRect        = { 0, 0, 0, 0 };                                     // área cliente da janela
                                                                            
    windowId          = 0;                                                  // id nulo porque a janela ainda não existe
    windowIcon        = LoadIcon(NULL, IDI_APPLICATION);                    // ícone padrão de uma aplicação
    windowCursor      = LoadCursor(NULL, IDC_ARROW);                        // cursor padrão de uma aplicação
    windowColor       = RGB(0,0,0);                                         // cor de fundo padrão é preta
    windowTitle       = string("Windows App");                              // título padrão da janela
    windowStyle       = WS_POPUP | WS_VISIBLE;                              // estilo para tela cheia
    windowMode        = FULLSCREEN;                                         // modo padrão é tela cheia
    windowPosX        = 0;                                                  // posição inicial da janela no eixo x
    windowPosY        = 0;                                                  // posição inicial da janela no eixo y
    windowWidth       = screenWidth;                                        // largura da janela
    windowHeight      = screenHeight;                                       // altura da janela
    windowAspectRatio = windowWidth / float(windowHeight);                  // aspect ratio da janela
    windowMinWidth    = windowWidth / 2;                                    // largura mínima da janela
    windowMinHeight   = windowHeight / 2;                                   // altura mínima da janela
                                                                            
    bordersWidth      = 0;                                                  // largura das bordas e barras da janela
    bordersHeight     = 0;                                                  // altura das bordas e barras da janela
}

// -------------------------------------------------------------------------------

Window::~Window()
{
    // libera contexto do dispositivo
    if (hdc) ReleaseDC(windowId, hdc);
}

// -------------------------------------------------------------------------------

void Window::Mode(int mode)
{
    windowMode = mode;

	switch (windowMode)
	{
	case FULLSCREEN:
		windowStyle = WS_EX_TOPMOST | WS_POPUP | WS_VISIBLE;
		break;
	case UNLOCKED:
		windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
		break;
	case LOCKED:
		windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_VISIBLE;
		break;
	case ASPECTRATIO:
		windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
		windowStyle -= WS_MAXIMIZEBOX;
		break;
	}
}

// -------------------------------------------------------------------------------

void Window::Size(int width, int height)
{ 
    // tamanho da área cliente
    clientWidth = int(width * windowScale);
    clientHeight = int(height * windowScale);

    // posição central da área cliente
    clientCenterX = clientWidth / 2;
    clientCenterY = clientHeight / 2;

    // aspect ratio da área cliente
    clientAspectRatio = clientWidth / float(clientHeight);

    // tamanho da área cliente e da janela iniciam iguais
    // janela será redimensionada no momento da sua criação
    windowWidth = clientWidth;
    windowHeight = clientHeight;

    // tamanho mínimo da janela
    windowMinWidth = windowWidth / 2;
    windowMinHeight = windowHeight / 2;

    // aspect ratio da janela
    windowAspectRatio = windowWidth / float(windowHeight);

    // ajusta a posição da janela para o centro da tela
    windowPosX = (screenWidth/2) - (windowWidth /2);
    windowPosY = (screenHeight/2) - (windowHeight /2);
}

// -------------------------------------------------------------------------------

void Window::ClientResize(int width, int height)
{
    // tamanho da área cliente
    clientWidth = width;
    clientHeight = height;

    // posição do central da área cliente
    clientCenterX = clientWidth / 2;
    clientCenterY = clientHeight / 2;

    // tamanho da janela
    windowWidth = width + bordersWidth;
    windowHeight = height + bordersHeight;

    // atualiza aspect ratio se necessário
    if (windowMode != ASPECTRATIO)
    {
         // aspect ratio da área cliente
        clientAspectRatio = clientWidth / float(clientHeight);

        // aspect ratio da janela
        windowAspectRatio = windowWidth / float(windowHeight);
    }
}

// -------------------------------------------------------------------------------

void Window::ChangeDpi(uint dpi, RECT* rect)
{
    // escala para novo dpi
    screenDpi = dpi; 
    windowScale = screenDpi / float(USER_DEFAULT_SCREEN_DPI); 

    // tamanho da janela
    windowWidth = rect->right - rect->left;
    windowHeight = rect->bottom - rect->top;

    // tamanho mínimo da janela
    windowMinWidth = windowWidth / 2;
    windowMinHeight = windowHeight / 2;

    // aspect ratio da janela
    windowAspectRatio = windowWidth / float(windowHeight);

    // redimensiona janela com uma chamada a MoveWindow
    MoveWindow(
        windowId,       // identificador da janela
        rect->left,     // posição x
        rect->top,      // posição y
        windowWidth,    // largura
        windowHeight,   // altura
        TRUE);          // repintar

    // captura informações do monitor que contém a janela
    HMONITOR hMonitor = MonitorFromWindow(windowId, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
    GetMonitorInfo(hMonitor, &monitorInfo);

    // atualiza tamanho da tela
    screenWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    screenHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

    // atualiza coordenadas de origem do monitor 
    monitorX = monitorInfo.rcMonitor.left;
    monitorY = monitorInfo.rcMonitor.top;
}

// -------------------------------------------------------------------------------

void Window::Sizing(RECT * rect, WPARAM wParam)
{
    // manter as proporções originais da janela
    if (windowMode == ASPECTRATIO)
    {
        // novas dimensões da janela
        long width = rect->right - rect->left;
        long height = rect->bottom - rect->top;

        // qual borda está sendo usada para redimensionar
        switch (wParam)
        {
        case WMSZ_TOPLEFT:
        case WMSZ_TOPRIGHT:
            // altura para manter a proporção da janela
            height = long(width / windowAspectRatio);

            // obedece a altura mínima da janela
            height = height < windowMinHeight ? windowMinHeight : height;

            // novo retângulo da janela
            rect->top = rect->bottom - height;

            // se a coordenada saiu da tela
            if (rect->top - monitorY < 0)
            {
                // limita o topo
                rect->top = monitorY + 0;

                // altura com o novo limite
                height = rect->bottom - rect->top;
            }

            // largura para manter a proporção da janela
            width = long(height * windowAspectRatio);

            // ajusta retângulo de acordo com o lado modificado
            if (wParam == WMSZ_TOPLEFT)
                rect->left = rect->right - width;
            else
                rect->right = rect->left + width;
            break;

        case WMSZ_BOTTOMLEFT:
        case WMSZ_BOTTOMRIGHT:
        case WMSZ_LEFT:
        case WMSZ_RIGHT:
            // altura para manter a proporção da janela
            height = long(width / windowAspectRatio);

            // obedece a altura mínima da janela
            height = height < windowMinHeight ? windowMinHeight : height;

            // novo retângulo da janela
            rect->bottom = rect->top + height;

            // se a coordenada saiu da tela
            if (rect->bottom - monitorY > screenHeight)
            {
                // limita a coordenada inferior
                rect->bottom = monitorY + screenHeight;

                // altura com o novo limite
                height = rect->bottom - rect->top;
            }

            // largura para manter a proporção da janela
            width = long(height * windowAspectRatio);

            // ajusta retângulo de acordo com o lado modificado
            if (wParam == WMSZ_LEFT || wParam == WMSZ_BOTTOMLEFT)
                rect->left = rect->right - width;
            else
                rect->right = rect->left + width;
            break;

        case WMSZ_BOTTOM:
        case WMSZ_TOP:
            // largura para manter a proporção da janela
            width = long(height * windowAspectRatio);

            // obedece a largura mínima da janela
            width = width < windowMinWidth ? windowMinWidth : width;
            
            // novo retângulo da janela
            rect->right = rect->left + width;

            // se a coordenada saiu da tela
            if (rect->right - monitorX > screenWidth)
            {
                // limita a coordenada direita
                rect->right = monitorX + screenWidth;

                // largura com o novo limite
                width = rect->right - rect->left;                
            }

            // altura para manter a proporção da janela
            height = long(width / windowAspectRatio);

            // ajusta retângulo de acordo com o lado modificado
            if (wParam == WMSZ_TOP)
                rect->top = rect->bottom - height;
            else
                rect->bottom = rect->top + height;
            break;
        }
    }
}

// -------------------------------------------------------------------------------

bool Window::Create()
{
    // identificador da aplicação
    HINSTANCE appId = GetModuleHandle(NULL);
    
    // definindo uma classe de janela
    WNDCLASSEX wndClass;     
    wndClass.cbSize        = sizeof(WNDCLASSEX);
    wndClass.style         = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc   = Window::WinProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = 0;
    wndClass.hInstance     = appId;
    wndClass.hIcon         = windowIcon;
    wndClass.hCursor       = windowCursor; 
    wndClass.hbrBackground = (HBRUSH) CreateSolidBrush(windowColor);
    wndClass.lpszMenuName  = NULL;
    wndClass.lpszClassName = "AppWindow";
    wndClass.hIconSm       = windowIcon;

    // registrando classe "AppWindow"
    if (!RegisterClassEx(&wndClass))
        return false;

    // criando uma janela baseada na classe "AppWindow" 
    windowId = CreateWindowEx(
        NULL,                           // estilos extras
        "AppWindow",                    // nome da classe da janela
        windowTitle.c_str(),            // título da janela
        windowStyle,                    // estilo da janela
        windowPosX, windowPosY,         // posição (x,y) inicial
        clientWidth, clientHeight,      // largura e altura da janela
        NULL,                           // identificador da janela pai
        NULL,                           // identificador do menu
        appId,                          // identificador da aplicação
        NULL);                          // parâmetros de criação

    // Ao usar o modo em janela é preciso levar em conta que as barras 
    // e bordas ocupam espaço na janela. O código abaixo ajusta o tamanho
    // da janela de forma que a área cliente fique com o tamanho requisitado
    // na função principal

    if (windowMode != FULLSCREEN)
    {
        // retângulo com o tamanho da área cliente desejada
        RECT rect = {0, 0, clientWidth, clientHeight};

        // ajusta o tamanho do retângulo
        AdjustWindowRectEx(&rect,
            GetWindowStyle(windowId),
            GetMenu(windowId) != NULL,
            GetWindowExStyle(windowId));

        // novas dimensões da janela
        windowWidth = rect.right - rect.left;
        windowHeight = rect.bottom - rect.top;

        // dimensões das bordas e barras da janela
        bordersWidth = windowWidth - clientWidth;
        bordersHeight = windowHeight - clientHeight;

        // ajusta a posição da janela para o centro da tela
        windowPosX = (screenWidth/2) - (windowWidth/2);
        windowPosY = (screenHeight/2) - (windowHeight/2);

        // aspect ratio da janela
        windowAspectRatio = windowWidth / float(windowHeight);

        // redimensiona janela com uma chamada a MoveWindow
        MoveWindow(
            windowId,       // identificador da janela
            windowPosX,     // posição x
            windowPosY,     // posição y
            windowWidth,    // largura
            windowHeight,   // altura
            TRUE);          // repintar
    }

    // captura contexto do dispositivo
    hdc = GetDC(windowId);

    // pega tamanho da área cliente
    GetClientRect(windowId, &clientRect);

    // retorna estado da inicialização (bem sucedida ou não)
    return (windowId ? true : false);
}

// -------------------------------------------------------------------------------

LRESULT CALLBACK Window::WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    // janela perdeu o foco
    case WM_KILLFOCUS:
        if (lostFocus)
            lostFocus();
        return 0;

    // janela recebeu o foco
    case WM_SETFOCUS:
        if (inFocus)
            inFocus();
        return 0;

    // a janela foi destruida
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// -----------------------------------------------------------------------------
