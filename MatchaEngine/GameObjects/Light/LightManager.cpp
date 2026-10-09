#include "LightManager.h"
#include "Graphics/GraphicsDevice.h"
#include <imgui.h>
#include <string>
#include <algorithm>
#include "Math/Calculation.h"
#include "../../Editer/LanguageManager.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"

LightManager::LightManager()
{
}

void LightManager::Initialize()
{
    // DirectionalLightResource
    directionalLightResource_ = GraphicsDevice::CreateBufferResource(sizeof(DirectionalLightGroupData));
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));

    // PointLightResource
    pointLightResource_ = GraphicsDevice::CreateBufferResource(sizeof(PointLightGroupData));
    pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));

    // SpotLightResource
    spotLightResource_ = GraphicsDevice::CreateBufferResource(sizeof(SpotLightGroupData));
    spotLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData_));

    // Initialize with 0 lights
    directionalLightData_->numLights = 0;
    for (int i = 0; i < kMaxLightCount; ++i) directionalLightData_->lights[i].active = 0;

    pointLightData_->numLights = 0;
    for (int i = 0; i < kMaxLightCount; ++i) pointLightData_->lights[i].active = 0;

    spotLightData_->numLights = 0;
    for (int i = 0; i < kMaxLightCount; ++i) spotLightData_->lights[i].active = 0;
}

void LightManager::ImGui()
{
#ifdef _USE_IMGUI
    ImGui::Begin(LanguageManager::Tr("Light Settings"));
    ImGui::Text("Active Directional Lights: %d", directionalLightData_ ? directionalLightData_->numLights : 0);
    ImGui::Text("Active Point Lights: %d", pointLightData_ ? pointLightData_->numLights : 0);
    ImGui::Text("Active Spot Lights: %d", spotLightData_ ? spotLightData_->numLights : 0);
    ImGui::End();
#endif
}

void LightManager::TransferData()
{
}

#include "BaseLight.h"

void LightManager::BeginUpdate()
{
    dirCount_ = 0;
    pointCount_ = 0;
    spotCount_ = 0;
}

void LightManager::EndUpdate()
{
    if (!directionalLightData_ || !pointLightData_ || !spotLightData_) return;

    directionalLightData_->numLights = dirCount_;
    for (int i = dirCount_; i < kMaxLightCount; ++i) {
        directionalLightData_->lights[i].active = 0;
    }

    pointLightData_->numLights = pointCount_;
    for (int i = pointCount_; i < kMaxLightCount; ++i) {
        pointLightData_->lights[i].active = 0;
    }

    spotLightData_->numLights = spotCount_;
    for (int i = spotCount_; i < kMaxLightCount; ++i) {
        spotLightData_->lights[i].active = 0;
    }
}

void LightManager::AddDirectionalLight(const Vector4& color, const Vector3& direction, float intensity)
{
    if (!directionalLightData_ || dirCount_ >= kMaxLightCount) return;
    directionalLightData_->lights[dirCount_].color = color;
    directionalLightData_->lights[dirCount_].direction = direction;
    directionalLightData_->lights[dirCount_].intensity = intensity;
    directionalLightData_->lights[dirCount_].active = 1;
    dirCount_++;
}

void LightManager::AddPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay)
{
    if (!pointLightData_ || pointCount_ >= kMaxLightCount) return;
    pointLightData_->lights[pointCount_].color = color;
    pointLightData_->lights[pointCount_].position = position;
    pointLightData_->lights[pointCount_].intensity = intensity;
    pointLightData_->lights[pointCount_].radius = radius;
    pointLightData_->lights[pointCount_].decay = decay;
    pointLightData_->lights[pointCount_].active = 1;
    pointCount_++;
}

void LightManager::AddSpotLight(const Vector4& color, const Vector3& position, const Vector3& direction, float intensity, float distance, float decay, float cosAngle, float cosFalloffStart)
{
    if (!spotLightData_ || spotCount_ >= kMaxLightCount) return;
    spotLightData_->lights[spotCount_].color = color;
    spotLightData_->lights[spotCount_].position = position;
    spotLightData_->lights[spotCount_].direction = direction;
    spotLightData_->lights[spotCount_].intensity = intensity;
    spotLightData_->lights[spotCount_].distance = distance;
    spotLightData_->lights[spotCount_].decay = decay;
    spotLightData_->lights[spotCount_].cosAngle = cosAngle;
    spotLightData_->lights[spotCount_].cosFalloffStart = cosFalloffStart;
    spotLightData_->lights[spotCount_].active = 1;
    spotCount_++;
}

void LightManager::UpdateLights(const std::vector<std::shared_ptr<GameObject>>& objects)
{
    if (!directionalLightData_ || !pointLightData_ || !spotLightData_) return;

    BeginUpdate();
    for (const auto& obj : objects) {
        if (!obj || !obj->GetIsActive()) continue;

        if (auto light = std::dynamic_pointer_cast<BaseLight>(obj)) {
            light->ApplyToLightManager(*this);
        }
    }
    EndUpdate();
}
