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

void LightManager::UpdateLights(const std::vector<std::shared_ptr<GameObject>>& objects)
{
    if (!directionalLightData_ || !pointLightData_ || !spotLightData_) return;

    int dirCount = 0;
    int pointCount = 0;
    int spotCount = 0;

    for (const auto& obj : objects) {
        if (!obj || !obj->GetIsActive()) continue;

        if (auto dirLight = std::dynamic_pointer_cast<DirectionalLight>(obj)) {
            if (dirCount < kMaxLightCount) {
                directionalLightData_->lights[dirCount].color = dirLight->color_;
                
                Matrix4x4 rot = Rotation(dirLight->GetTransform().rotate);
                Vector3 baseDir = { 0.0f, -1.0f, 0.0f };
                Vector3 dir = TransformMatrix(baseDir, rot);
                directionalLightData_->lights[dirCount].direction = Normalize(dir);
                
                directionalLightData_->lights[dirCount].intensity = dirLight->intensity_;
                directionalLightData_->lights[dirCount].active = 1;
                dirCount++;
            }
        }
        else if (auto pointLight = std::dynamic_pointer_cast<PointLight>(obj)) {
            if (pointCount < kMaxLightCount) {
                pointLightData_->lights[pointCount].color = pointLight->color_;
                pointLightData_->lights[pointCount].position = pointLight->GetTransform().translate;
                pointLightData_->lights[pointCount].intensity = pointLight->intensity_;
                pointLightData_->lights[pointCount].radius = pointLight->radius_;
                pointLightData_->lights[pointCount].decay = pointLight->decay_;
                pointLightData_->lights[pointCount].active = 1;
                pointCount++;
            }
        }
        else if (auto spotLight = std::dynamic_pointer_cast<SpotLight>(obj)) {
            if (spotCount < kMaxLightCount) {
                spotLightData_->lights[spotCount].color = spotLight->color_;
                spotLightData_->lights[spotCount].position = spotLight->GetTransform().translate;
                
                Matrix4x4 rot = Rotation(spotLight->GetTransform().rotate);
                Vector3 baseDir = { 0.0f, -1.0f, 0.0f };
                Vector3 dir = TransformMatrix(baseDir, rot);
                spotLightData_->lights[spotCount].direction = Normalize(dir);
                
                spotLightData_->lights[spotCount].intensity = spotLight->intensity_;
                spotLightData_->lights[spotCount].distance = spotLight->distance_;
                spotLightData_->lights[spotCount].decay = spotLight->decay_;
                spotLightData_->lights[spotCount].cosAngle = spotLight->cosAngle_;
                spotLightData_->lights[spotCount].cosFalloffStart = spotLight->cosFalloffStart_;
                spotLightData_->lights[spotCount].active = 1;
                spotCount++;
            }
        }
    }

    // Set remaining lights in the buffer to inactive
    directionalLightData_->numLights = dirCount;
    for (int i = dirCount; i < kMaxLightCount; ++i) {
        directionalLightData_->lights[i].active = 0;
    }

    pointLightData_->numLights = pointCount;
    for (int i = pointCount; i < kMaxLightCount; ++i) {
        pointLightData_->lights[i].active = 0;
    }

    spotLightData_->numLights = spotCount;
    for (int i = spotCount; i < kMaxLightCount; ++i) {
        spotLightData_->lights[i].active = 0;
    }
}
