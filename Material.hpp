#pragma once
#include "Framework.hpp"

class Material
{
public:
    ShaderSet* shaders = nullptr;  // [C26495 수정] nullptr로 명시적 초기화

    std::vector<Texture*> textureList;
    std::vector<std::vector<uint8_t>> constantDataList;
    std::vector<ID3D11Buffer*> constantBuffers;

    Material() : shaders(nullptr) {}  // [C26495 수정] 기본 생성자에서도 명시적 초기화
    Material(ShaderSet* s) : shaders(s) {}
    virtual ~Material()
    {
        for (auto cb : constantBuffers) if (cb) cb->Release();
    }

    void SetShaderSet(ShaderSet* shaderSet)
    {
        shaders = shaderSet;
    }

    void AddTexture(Texture* tex)
    {
        textureList.push_back(tex);
    }

    template<typename T>
    void AddConstantData(const T& data)
    {
        size_t originalSize = sizeof(T);
        size_t alignedSize = (originalSize + 15) & ~15;
        std::vector<uint8_t> byteBuffer(alignedSize, 0);
        memcpy(byteBuffer.data(), &data, originalSize);
        constantDataList.push_back(byteBuffer);
    }

    virtual void Bind(ID3D11DeviceContext* context)
    {
        if (!shaders) return;

        for (int i = 0; i < (int)constantDataList.size(); ++i)
        {
            if (i >= (int)constantBuffers.size())
                constantBuffers.push_back(nullptr);

            if (constantBuffers[i] == nullptr)
            {
                ID3D11Device* device = nullptr;
                context->GetDevice(&device);

                D3D11_BUFFER_DESC desc = {};
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.ByteWidth = (UINT)constantDataList[i].size();
                desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

                D3D11_SUBRESOURCE_DATA initData = {};
                initData.pSysMem = constantDataList[i].data();

                device->CreateBuffer(&desc, &initData, &constantBuffers[i]);
                device->Release();
            }
        }

        context->IASetInputLayout(shaders->layout);
        context->VSSetShader(shaders->vs, nullptr, 0);
        context->PSSetShader(shaders->ps, nullptr, 0);

        for (int i = 0; i < (int)textureList.size(); ++i)
        {
            ID3D11ShaderResourceView* srv = textureList[i]->pSRV;
            context->VSSetShaderResources(i, 1, &srv);
            context->PSSetShaderResources(i, 1, &srv);

            if (textureList[i]->pSampler)
            {
                context->VSSetSamplers(0, 1, &textureList[i]->pSampler);
                context->PSSetSamplers(0, 1, &textureList[i]->pSampler);
            }
        }

        for (int i = 0; i < (int)constantBuffers.size(); ++i)
        {
            context->VSSetConstantBuffers(i + 1, 1, &constantBuffers[i]);
            context->PSSetConstantBuffers(i + 1, 1, &constantBuffers[i]);
        }
    }

    template<typename T>
    void UpdateConstantData(ID3D11DeviceContext* context, int index, const T& data)
    {
        if (index >= (int)constantBuffers.size()) return;
        context->UpdateSubresource(constantBuffers[index], 0, nullptr, &data, 0, 0);
    }
};