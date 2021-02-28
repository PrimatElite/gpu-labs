#pragma once

#include <vector>

#include "RenderTexture.h"

class AverageLuminanceProcess
{
public:
    AverageLuminanceProcess();
    ~AverageLuminanceProcess();

    HRESULT CreateDeviceDependentResources(ID3D11Device* device);
    HRESULT CreateWindowSizeDependentResources(ID3D11Device* device, UINT width, UINT height);

    ID3D11ShaderResourceView* GetResultShaderResourceView() const;

    void Process(ID3D11DeviceContext* context, ID3D11ShaderResourceView* sourceTexture);

private:
    void CopyTexture(ID3D11DeviceContext* context, ID3D11ShaderResourceView* sourceTexture, RenderTexture& dst, ID3D11PixelShader* pixelShader);

    std::vector<RenderTexture> m_renderTextures;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pCopyPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pLuminancePixelShader;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerState;
};
