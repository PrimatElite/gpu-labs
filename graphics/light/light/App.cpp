#include "pch.h"

#include "App.h"

App::App() :
    m_hWnd()
{};

HRESULT App::CreateDesktopWindow(HINSTANCE hInstance, int nCmdShow, WNDPROC pWndProc)
{
    const wchar_t CLASS_NAME[] = L"Lab2Class";
    WNDCLASSEX wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = pWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = CLASS_NAME;
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    m_hWnd = CreateWindow(CLASS_NAME, L"Lab2", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
    if (!m_hWnd)
        return HRESULT_FROM_WIN32(GetLastError());

    ShowWindow(m_hWnd, nCmdShow);

    return S_OK;
}

HRESULT App::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    m_pDeviceResources = std::shared_ptr<DeviceResources>(new DeviceResources());
    hr = m_pDeviceResources->CreateDeviceResources();
    if (FAILED(hr))
        return hr;

    hr = m_pDeviceResources->CreateWindowResources(m_hWnd);
    if (FAILED(hr))
        return hr;

    m_pRenderer = std::shared_ptr<Renderer>(new Renderer(m_pDeviceResources));
    hr = m_pRenderer->CreateDeviceDependentResources();
    if (FAILED(hr))
        return hr;

    hr = m_pRenderer->CreateWindowSizeDependentResources();

    return hr;
}

void App::Render()
{
    m_pDeviceResources->GetAnnotation()->BeginEvent(L"Start rendering");

    m_pRenderer->Update();
    m_pRenderer->Render();

    m_pDeviceResources->Present();

    m_pDeviceResources->GetAnnotation()->EndEvent();
}

void App::Run()
{
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }
}

HRESULT App::OnResize()
{
    HRESULT hr = S_OK;

    if (m_pDeviceResources && m_pDeviceResources->GetSwapChain())
    {
        hr = m_pDeviceResources->OnResize();
        if (SUCCEEDED(hr))
            hr = m_pRenderer->OnResize();
    }

    return hr;
}

App::~App()
{}
