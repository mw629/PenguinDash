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
    void BeginUpdate();
    void EndUpdate();
    void AddDirectionalLight(const Vector4& color, const Vector3& direction, float intensity);
    void AddPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay);
    void AddSpotLight(const Vector4& color, const Vector3& position, const Vector3& direction, float intensity, float distance, float decay, float cosAngle, float cosFalloffStart);

    ID3D12Resource* GetDirectionalLightResource() { return directionalLightResource_.Get(); }
    ID3D12Resource* GetPointLightResource() { return pointLightResource_.Get(); }
    ID3D12Resource* GetSpotLightResource() { return spotLightResource_.Get(); }

private:
    int dirCount_ = 0;
    int pointCount_ = 0;
    int spotCount_ = 0;
};
