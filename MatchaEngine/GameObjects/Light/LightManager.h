#pragma once
#include <vector>
#include <memory>
#include <wrl.h>
#include <d3dx12.h>
#include "Core/VariableTypes.h"
#include <numbers>

class GameObject;

class LightManager
{
private:
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLightGroupData* directionalLightData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    PointLightGroupData* pointLightData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
    SpotLightGroupData* spotLightData_ = nullptr;

public:
    LightManager();
    void Initialize();
    void ImGui();
    void TransferData();
    void UpdateLights(const std::vector<std::shared_ptr<GameObject>>& objects);

    ID3D12Resource* GetDirectionalLightResource() { return directionalLightResource_.Get(); }
    ID3D12Resource* GetPointLightResource() { return pointLightResource_.Get(); }
    ID3D12Resource* GetSpotLightResource() { return spotLightResource_.Get(); }
};
