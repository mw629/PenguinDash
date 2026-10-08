#include "MaterialComponent.h"
#ifdef _USE_IMGUI
#include <imgui.h>
#endif
#include "../../../Editer/LanguageManager.h"
MaterialComponent::MaterialComponent(bool lighting, float environmentCoefficient)
    : initialLighting_(lighting), initialEnvCoeff_(environmentCoefficient)
{
    materialFactory_ = std::make_unique<MaterialFactory>();
}

void MaterialComponent::Initialize()
{
    if (materialFactory_) {
        materialFactory_->CreateMartial(initialLighting_, initialEnvCoeff_);
    }
}

void MaterialComponent::ImGui()
{
#ifdef _USE_IMGUI
    if (ImGui::TreeNodeEx(LanguageManager::Tr("Material"), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (materialFactory_) {
            materialFactory_->ImGui();
        }

        // Shader
        const char* shaders[] = { "ObjectShader", "WaterShader", "IceShader", "AnimationObj", "ParticleShader", "SkyBoxShader" };
        int current_shader = 0;
        for (int i = 0; i < IM_ARRAYSIZE(shaders); ++i) {
            if (shader_ == shaders[i]) {
                current_shader = i;
                break;
            }
        }
        if (ImGui::Combo(LanguageManager::Tr("Shader Selection"), &current_shader, shaders, IM_ARRAYSIZE(shaders))) {
            SetShader(shaders[current_shader]);
        }

        // Blend
        static const BlendMode blendModes[] = { kBlendModeNone, kBlendModeNormal, kBlendModeAdd, kBlendModeSubtract, kBlendModeMultiply, kBlendModeScreen };
        static const char* blendNames[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
        int current_blend = 0;
        for (int i = 0; i < IM_ARRAYSIZE(blendModes); ++i) {
            if (blend_ == blendModes[i]) { current_blend = i; break; }
        }
        if (ImGui::Combo(LanguageManager::Tr("Blend Selection"), &current_blend, blendNames, IM_ARRAYSIZE(blendNames))) {
            blend_ = blendModes[current_blend];
        }

        // Cull Mode
        static const CullMode cullModes[] = { kCullModeNone, kCullModeFront, kCullModeBack };
        static const char* cullNames[] = { "None (Both Sides)", "Front", "Back (Standard)" };
        int current_cull = 0;
        for (int i = 0; i < IM_ARRAYSIZE(cullModes); ++i) {
            if (cullMode_ == cullModes[i]) { current_cull = i; break; }
        }
        if (ImGui::Combo(LanguageManager::Tr("Cull Mode"), &current_cull, cullNames, IM_ARRAYSIZE(cullNames))) {
            cullMode_ = cullModes[current_cull];
        }
        
        ImGui::Separator();
        ImGui::Text(LanguageManager::Tr("Texture:"));
        std::string btnLabel = (texturePath_.empty() ? LanguageManager::Tr("None") : texturePath_) + "##TexDrop";
        ImGui::Button(btnLabel.c_str(), ImVec2(-FLT_MIN, 0)); // Button taking full width

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_FILE")) {
                std::string path((const char*)payload->Data, payload->DataSize - 1);
                if (path.find(".png") != std::string::npos || path.find(".jpg") != std::string::npos || path.find(".dds") != std::string::npos) {
                    SetTexturePath(path);
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::TextDisabled("(Drag and drop an image from the Resources window)");

        ImGui::Separator();
        ImGui::Text("UV Transform:");
        bool uvChanged = false;
        uvChanged |= ImGui::DragFloat3("UV Scale", &uvTransform_.scale.x, 0.01f);
        uvChanged |= ImGui::DragFloat3("UV Rotate", &uvTransform_.rotate.x, 0.01f);
        uvChanged |= ImGui::DragFloat3("UV Translate", &uvTransform_.translate.x, 0.01f);
        if (uvChanged) {
            UpdateUVTransform();
        }

        ImGui::TreePop();
    }
#endif
}

void MaterialComponent::SetTexturePath(const std::string& path)
{
    texturePath_ = path;
    Texture tex;
    textureSrvHandleGPU_ = tex.TextureData(texturePath_);
}

void MaterialComponent::UpdateUVTransform()
{
    if (materialFactory_) {
        Matrix4x4 uvMat = MakeAffineMatrix(uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
        materialFactory_->SetUVTransform(uvMat);
    }
}

void MaterialComponent::SetShader(ShaderName shader)
{
    shader_ = shader;
    if (shader_ == SkyBoxShader) {
        cullMode_ = kCullModeFront;
    }
}

