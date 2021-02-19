#include "pch.h"

#include "DeviceResources.h"

DeviceResources::DeviceResources() :
    m_featureLevel(D3D_FEATURE_LEVEL_11_0),
    m_backBufferDesc()
{};

HRESULT DeviceResources::CreateDeviceResources()
{
    HRESULT hr = S_OK;

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

    Microsoft::WRL::ComPtr<ID3D11Device>        device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        hr = D3D11CreateDevice(nullptr, driverTypes[driverTypeIndex], nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &device, &m_featureLevel, &context);
        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return hr;

    device.As(&m_pd3dDevice);
    context.As(&m_pd3dDeviceContext);

    hr = m_pd3dDeviceContext->QueryInterface(IID_PPV_ARGS(&m_pAnnotation));

    return hr;
}

HRESULT DeviceResources::CreateWindowResources(HWND hWnd)
{
    HRESULT hr = S_OK;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;
    {
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
        hr = m_pd3dDevice.As(&dxgiDevice);
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
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hr = dxgiFactory->CreateSwapChain(m_pd3dDevice.Get(), &sd, &m_pSwapChain);
    if (FAILED(hr))
        return hr;

    dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(hr))
        return hr;

    // Configure the back buffer and viewport.
    hr = ConfigureBackBuffer();

    return hr;
}

HRESULT DeviceResources::ConfigureBackBuffer()
{
    HRESULT hr = S_OK;

    // Create a render target view
    Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer;
    hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(hr))
        return hr;

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRenderTargetView);
    if (FAILED(hr))
        return hr;

    pBackBuffer->GetDesc(&m_backBufferDesc);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)m_backBufferDesc.Width;
    vp.Height = (FLOAT)m_backBufferDesc.Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_pd3dDeviceContext->RSSetViewports(1, &vp);

    std::string textureName("Back Buffer");
    hr = pBackBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)textureName.size(), textureName.c_str());

    return hr;
}

HRESULT DeviceResources::OnResize()
{
    HRESULT hr = S_OK;

    m_pd3dDeviceContext->OMSetRenderTargets(0, 0, 0);
    m_pRenderTargetView.Reset();
    m_pd3dDeviceContext->Flush();

    hr = m_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

    hr = ConfigureBackBuffer();

    return hr;
}

float DeviceResources::GetAspectRatio()
{
    return static_cast<float>(m_backBufferDesc.Width) / static_cast<float>(m_backBufferDesc.Height);
}

void DeviceResources::Present()
{
    m_pSwapChain->Present(1, 0);
}

DeviceResources::~DeviceResources()
{}