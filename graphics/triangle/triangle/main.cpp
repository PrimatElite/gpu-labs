#ifndef UNICODE
#define UNICODE
#endif

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <fstream>
#include <string>


struct Vertex
{
    DirectX::XMFLOAT3 Pos;
};


struct Transformation
{
    DirectX::XMMATRIX mWorld;
    DirectX::XMMATRIX mView;
    DirectX::XMMATRIX mProjection;
};


HWND g_hwnd = nullptr;
D3D_DRIVER_TYPE g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_0;
Microsoft::WRL::ComPtr<ID3D11Device> g_pd3dDevice;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> g_pd3dDeviceContext;
Microsoft::WRL::ComPtr<IDXGISwapChain> g_pSwapChain;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> g_pRenderTargetView;
Microsoft::WRL::ComPtr<ID3D11VertexShader> g_pVertexShader;
Microsoft::WRL::ComPtr<ID3D11PixelShader> g_pPixelShader;
Microsoft::WRL::ComPtr<ID3D11InputLayout> g_pInputLayout;
Microsoft::WRL::ComPtr<ID3D11Buffer> g_pVertexBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer> g_pTransformBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer> g_pIndexBuffer;
DirectX::XMMATRIX g_World;
DirectX::XMMATRIX g_View;
DirectX::XMMATRIX g_Projection;
Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> pAnnotation;


HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Render();


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR pCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);

    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return 0;

    if (FAILED(InitDevice()))
        return 0;

    // Run the message loop.

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

    return 0;
}


HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"Lab1Class";
    WNDCLASSEX wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
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
    g_hwnd = CreateWindow(CLASS_NAME, L"Lab1", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
    if (!g_hwnd)
        return E_FAIL;

    ShowWindow(g_hwnd, nCmdShow);

    return S_OK;
}


HRESULT ReadCompiledShader(const WCHAR* szFileName, BYTE** bytes, size_t &bufferSize)
{
    std::ifstream csoFile(szFileName, std::ios::in | std::ios::binary | std::ios::ate);

    if (csoFile.is_open())
    {
        bufferSize = (size_t)csoFile.tellg();
        *bytes = new BYTE[bufferSize];

        csoFile.seekg(0, std::ios::beg);
        csoFile.read(reinterpret_cast<char*>(*bytes), bufferSize);
        csoFile.close();

        return S_OK;
    }
    return HRESULT_FROM_WIN32(GetLastError());
}


HRESULT CreateViews(UINT width, UINT height)
{
    // Create a render target view
    Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer;
    HRESULT hr = g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(hr))
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &g_pRenderTargetView);
    if (FAILED(hr))
        return hr;

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pd3dDeviceContext->RSSetViewports(1, &vp);

	std::string textureName = "Back Buffer";
	pBackBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)textureName.size(), textureName.c_str());

    return S_OK;
}


HRESULT CreateShaders()
{
    BYTE* bytes = nullptr;
    HRESULT hr;
    size_t bufferSize;

#if defined(DEBUG) || defined(_DEBUG)
#ifdef ENVIRONMENT64
    hr = ReadCompiledShader(L"../Debug_x64/VertexShader.cso", &bytes, bufferSize);
#else
    hr = ReadCompiledShader(L"../Debug_Win32/VertexShader.cso", &bytes, bufferSize);
#endif // ENVIRONMENT64
#else
#ifdef ENVIRONMENT64
    hr = ReadCompiledShader(L"../Release_x64/VertexShader.cso", &bytes, bufferSize);
#else
    hr = ReadCompiledShader(L"../Release_Win32/VertexShader.cso", &bytes, bufferSize);
#endif // ENVIRONMENT64
#endif // DEBUG

    if (FAILED(hr))
        return hr;

    // Create the vertex shader
    hr = g_pd3dDevice->CreateVertexShader(bytes, bufferSize, nullptr, &g_pVertexShader);
    if (FAILED(hr))
    {
        delete[] bytes;
        return hr;
    }

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
    hr = g_pd3dDevice->CreateInputLayout(layout, numElements, bytes, bufferSize, &g_pInputLayout);
    delete[] bytes;
    if (FAILED(hr))
        return hr;

#if defined(DEBUG) || defined(_DEBUG)
#ifdef ENVIRONMENT64
    hr = ReadCompiledShader(L"../Debug_x64/PixelShader.cso", &bytes, bufferSize);
#else
    hr = ReadCompiledShader(L"../Debug_Win32/PixelShader.cso", &bytes, bufferSize);
#endif // ENVIRONMENT64
#else
#ifdef ENVIRONMENT64
    hr = ReadCompiledShader(L"../Release_x64/PixelShader.cso", &bytes, bufferSize);
#else
    hr = ReadCompiledShader(L"../Release_Win32/PixelShader.cso", &bytes, bufferSize);
#endif // ENVIRONMENT64
#endif // DEBUG

    if (FAILED(hr))
        return hr;

    // Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader(bytes, bufferSize, nullptr, &g_pPixelShader);
    delete[] bytes;
    if (FAILED(hr))
        return hr;

    return S_OK;
}


void InitViewAndPerspective(UINT width, UINT height)
{
    // Initialize the view matrix
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 0.0f, 1.5f, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    g_View = DirectX::XMMatrixLookAtLH(Eye, At, Up);

    // Initialize the projection matrix
    g_Projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f);
}


void InitMatrices(UINT width, UINT height)
{
    // Initialize the world matrix
    g_World = DirectX::XMMatrixIdentity();
    
    InitViewAndPerspective(width, height);
}


HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(g_hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

	// Debug layer enabling
	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	// If the project is in a debug build, enable the debug layer.
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pd3dDeviceContext);

        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return hr;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;
    {
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
        hr = g_pd3dDevice.As(&dxgiDevice);
        if (SUCCEEDED(hr))
        {
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
                hr = adapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
        }
    }
    if (FAILED(hr))
        return hr;

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hr = dxgiFactory->CreateSwapChain(g_pd3dDevice.Get(), &sd, &g_pSwapChain);

    dxgiFactory->MakeWindowAssociation(g_hwnd, DXGI_MWA_NO_ALT_ENTER);

    if (FAILED(hr))
        return hr;

    hr = CreateViews(width, height);
    if (FAILED(hr))
        return hr;

    // Create the vertex and pixel shaders
    hr = CreateShaders();
    if (FAILED(hr))
        return hr;

    // Create vertex buffer
    Vertex vertices[] =
    {
        DirectX::XMFLOAT3( 0.0f,  0.5f,  0.0f),
        DirectX::XMFLOAT3( 0.5f, -0.5f,  0.0f),
        DirectX::XMFLOAT3(-0.5f, -0.5f,  0.0f),
    };
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * 3;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
    if (FAILED(hr))
        return hr;
    
    // Create index buffer
    WORD indices[] =
    {
        0, 1, 2,
        2, 1, 0,
    };
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * 6;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
    if (FAILED(hr))
        return hr;
    
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Transformation);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pTransformBuffer);
    if (FAILED(hr))
        return hr;
    
    InitMatrices(width, height);
    
	hr = g_pd3dDeviceContext->QueryInterface(IID_PPV_ARGS(&pAnnotation));
	if (FAILED(hr))
		return hr;

    return S_OK;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;
    switch (uMsg)
    {
    case WM_SIZE:
        if (g_pSwapChain)
        {
            g_pd3dDeviceContext->OMSetRenderTargets(0, 0, 0);
            g_pRenderTargetView = nullptr;
            g_pd3dDeviceContext->Flush();
            hr = g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            InitViewAndPerspective(LOWORD(lParam), HIWORD(lParam));
            if (SUCCEEDED(hr))
                hr = CreateViews(LOWORD(lParam), HIWORD(lParam));
            if (FAILED(hr))
                PostQuitMessage(0);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void Render()
{
	pAnnotation->BeginEvent(L"Start rendering");
    // Clear the back buffer 
    float background_colour[4] = { 0.3f, 0.5f, 0.7f, 1.0f };
    g_pd3dDeviceContext->ClearRenderTargetView(g_pRenderTargetView.Get(), background_colour);

    g_pd3dDeviceContext->OMSetRenderTargets(1, g_pRenderTargetView.GetAddressOf(), nullptr);

    // Set vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pd3dDeviceContext->IASetVertexBuffers(0, 1, g_pVertexBuffer.GetAddressOf(), &stride, &offset);
    
    // Set index buffer
    g_pd3dDeviceContext->IASetIndexBuffer(g_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    
    // Set primitive topology
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    static float t = 0.0f;
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0)
        timeStart = timeCur;
    t = (timeCur - timeStart) / 1000.0f;
    g_World = DirectX::XMMatrixRotationY(t);
    
    Transformation tr;
    tr.mWorld = XMMatrixTranspose(g_World);
    tr.mView = XMMatrixTranspose(g_View);
    tr.mProjection = XMMatrixTranspose(g_Projection);
    g_pd3dDeviceContext->UpdateSubresource(g_pTransformBuffer.Get(), 0, nullptr, &tr, 0, 0);

    g_pd3dDeviceContext->IASetInputLayout(g_pInputLayout.Get());
   
    // Render a triangle
    g_pd3dDeviceContext->VSSetShader(g_pVertexShader.Get(), nullptr, 0);
    g_pd3dDeviceContext->VSSetConstantBuffers(0, 1, g_pTransformBuffer.GetAddressOf());
    g_pd3dDeviceContext->PSSetShader(g_pPixelShader.Get(), nullptr, 0);
    g_pd3dDeviceContext->DrawIndexed(6, 0, 0);

    // Present the information rendered to the back buffer to the front buffer (the screen)
    g_pSwapChain->Present(1, 0);

	pAnnotation->EndEvent();
}
