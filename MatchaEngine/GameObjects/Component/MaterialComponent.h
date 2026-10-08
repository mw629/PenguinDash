#pragma once
#include "Component.h"
#include "../MaterialFactory.h"
#include <memory>
#include "../../PSO/PipelineState.h"
#include "../../Resource/Texture.h"
#include "../../Math/Calculation.h"

class MaterialComponent : public Component
{
private:
    std::unique_ptr<MaterialFactory> materialFactory_;
    bool initialLighting_;
    float initialEnvCoeff_;

    ShaderName shader_ = "ObjectShader";
    BlendMode blend_ = kBlendModeNone;
    CullMode cullMode_ = kCullModeBack;

    std::string texturePath_ = "";
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};
    Transform uvTransform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };

public:
    MaterialComponent(bool lighting = false, float environmentCoefficient = 0.0f);
    ~MaterialComponent() override = default;

    void Initialize() override;
    void ImGui() override;

    MaterialFactory* GetMaterialFactory() const { return materialFactory_.get(); }

    ShaderName GetShader() const { return shader_; }
    void SetShader(ShaderName shader);

    BlendMode GetBlend() const { return blend_; }
    void SetBlend(BlendMode blend) { blend_ = blend; }

    CullMode GetCullMode() const { return cullMode_; }
    void SetCullMode(CullMode cull) { cullMode_ = cull; }

    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU() const { return textureSrvHandleGPU_; }
    void SetTexturePath(const std::string& path);
    void UpdateUVTransform();
};
