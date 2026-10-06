#include "Emitter.h"
#include "Calculation.h"
#include "Draw.h"
#include "Input.h"
#include <functional>

#ifdef _USE_IMGUI
#include <imgui.h>
#include <sstream>
#endif // _USE_IMGUI

#include "../../../Editer/LanguageManager.h"
#include "Core/LogHandler.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

void Emitter::SetTexturePath(const std::string &path) {
  texturePath_ = path;
  effectDefinition_->SetTexturePath(path);
}

void Emitter::SetShape(EffectShape shape) {
  shape_ = shape;
  effectDefinition_->SetShape(shape, shapeData_);
}

void Emitter::SetShapeData(const EffectShapeData &data) {
  shapeData_ = data;
  effectDefinition_->SetShapeData(data);
}

void Emitter::ImGui() {
#ifdef _USE_IMGUI

  ImGui::PushID(this);
  if (ImGui::CollapsingHeader(name_.c_str())) {
    if (ImGui::TreeNode(LanguageManager::Tr("Emitter Settings"))) {
      const char *emitterTypes[] = {"Box", "Sphere", "Circle", "Cone"};
      int currentType = static_cast<int>(emitterType_);
      if (ImGui::Combo(LanguageManager::Tr("Emitter Type"), &currentType,
                       emitterTypes, IM_ARRAYSIZE(emitterTypes))) {
        emitterType_ = static_cast<EmitterType>(currentType);
      }

      if (emitterType_ == EmitterType::Box) {
        if (ImGui::TreeNode(LanguageManager::Tr("Transform"))) {
          ImGui::DragFloat3(LanguageManager::Tr("Position"),
                            &emitter_.transform.translate.x, 0.01f, -FLT_MAX,
                            FLT_MAX, "%.2f");
          ImGui::DragFloat3(LanguageManager::Tr("Rotation"),
                            &emitter_.transform.rotate.x, 0.01f, -FLT_MAX,
                            FLT_MAX, "%.2f");
          ImGui::DragFloat3(LanguageManager::Tr("Spawn Area Scale"),
                            &emitter_.transform.scale.x, 0.01f, -FLT_MAX,
                            FLT_MAX, "%.2f");
          ImGui::TreePop();
        }
        ImGui::DragInt(LanguageManager::Tr("Count"),
                       reinterpret_cast<int *>(&emitter_.count), 1, 0, 10000);
        ImGui::DragFloat(LanguageManager::Tr("Frequency"), &emitter_.frequency,
                         0.01f, 0.0f, 10.0f, "%.2f");
        ImGui::DragFloat(LanguageManager::Tr("Frequency Time"),
                         &emitter_.frequencyTime, 0.01f, 0.0f, 9999.0f, "%.2f");
      } else if (emitterType_ == EmitterType::Sphere) {
        ImGui::DragFloat3(LanguageManager::Tr("Position"),
                          &emitterSphere_.translate.x, 0.01f, -FLT_MAX, FLT_MAX,
                          "%.2f");
        ImGui::DragFloat(LanguageManager::Tr("Radius"), &emitterSphere_.radius,
                         0.01f, 0.0f, FLT_MAX, "%.2f");
        ImGui::DragInt(LanguageManager::Tr("Count"),
                       reinterpret_cast<int *>(&emitterSphere_.count), 1, 0,
                       10000);
        ImGui::DragFloat(LanguageManager::Tr("Frequency"),
                         &emitterSphere_.frequency, 0.01f, 0.0f, 10.0f, "%.2f");
        ImGui::DragFloat(LanguageManager::Tr("Frequency Time"),
                         &emitterSphere_.frequencyTime, 0.01f, 0.0f, 9999.0f,
                         "%.2f");
      } else if (emitterType_ == EmitterType::Circle) {
        ImGui::DragFloat3(LanguageManager::Tr("Position"),
                          &emitterCircle_.translate.x, 0.01f, -FLT_MAX, FLT_MAX,
                          "%.2f");
        ImGui::DragFloat3(LanguageManager::Tr("Rotation"),
                          &emitterCircle_.rotate.x, 0.01f, -FLT_MAX, FLT_MAX,
                          "%.2f");
        ImGui::DragFloat(LanguageManager::Tr("Outer Radius"),
                         &emitterCircle_.outerRadius, 0.01f, 0.0f, FLT_MAX,
                         "%.2f");
        ImGui::DragFloat(LanguageManager::Tr("Inner Radius"),
                         &emitterCircle_.innerRadius, 0.01f, 0.0f,
                         emitterCircle_.outerRadius, "%.2f");
        ImGui::DragFloat(LanguageManager::Tr("Radial Velocity"),
                         &emitterCircle_.radialVelocity, 0.01f, -FLT_MAX,
                         FLT_MAX, "%.2f");
        ImGui::DragInt(LanguageManager::Tr("Count"),
                       reinterpret_cast<int *>(&emitterCircle_.count), 1, 0,
                       10000);
        ImGui::DragFloat(LanguageManager::Tr("Frequency"),
                         &emitterCircle_.frequency, 0.01f, 0.0f, 10.0f, "%.2f");
      } else if (emitterType_ == EmitterType::Cone) {
        ImGui::DragFloat3(LanguageManager::Tr("Position"),
                          &emitterCone_.translate.x, 0.01f, -FLT_MAX, FLT_MAX,
                          "%.2f");
        ImGui::DragFloat3(LanguageManager::Tr("Rotation"),
                          &emitterCone_.rotate.x, 0.01f, -FLT_MAX, FLT_MAX,
                          "%.2f");
        ImGui::DragFloat(LanguageManager::Tr("Radius"), &emitterCone_.radius,
                         0.01f, 0.0f, FLT_MAX, "%.2f");
        ImGui::SliderAngle(LanguageManager::Tr("Spread Angle"),
                           &emitterCone_.angle, 0.0f, 90.0f);
        ImGui::DragFloat(LanguageManager::Tr("Speed"), &emitterCone_.speed,
                         0.01f, -FLT_MAX, FLT_MAX, "%.2f");
        ImGui::DragInt(LanguageManager::Tr("Count"),
                       reinterpret_cast<int *>(&emitterCone_.count), 1, 0,
                       10000);
        ImGui::DragFloat(LanguageManager::Tr("Frequency"),
                         &emitterCone_.frequency, 0.01f, 0.0f, 10.0f, "%.2f");
      }
      ImGui::TreePop();
    }

    if (ImGui::TreeNode(LanguageManager::Tr("Spawn / Burst Mode"))) {
      ImGui::Checkbox(LanguageManager::Tr("Loop Emission"), &isLoop_);
      ImGui::DragInt(LanguageManager::Tr("Burst Count"),
                     reinterpret_cast<int *>(&burstCount_), 1, 1, 10000);
      if (ImGui::Button(LanguageManager::Tr("Trigger Burst"))) {
        TriggerBurst();
      }
      ImGui::TreePop();
    }

    if (ImGui::TreeNode(LanguageManager::Tr("Particle Initial Settings"))) {
      ImGui::DragFloat3(LanguageManager::Tr("Base Position"),
                        &SetEffectDefinitionData_.transform.translate.x, 0.01f,
                        -FLT_MAX, FLT_MAX, "%.2f");
      ImGui::DragFloat3(LanguageManager::Tr("Base Size (Scale)"),
                        &SetEffectDefinitionData_.transform.scale.x, 0.01f,
                        -FLT_MAX, FLT_MAX, "%.2f");
      ImGui::DragFloat3(LanguageManager::Tr("Base Rotation"),
                        &SetEffectDefinitionData_.transform.rotate.x, 0.01f,
                        -FLT_MAX, FLT_MAX, "%.2f");
      ImGui::Checkbox(LanguageManager::Tr("Align to Velocity"),
                      &alignToVelocity_);
      ImGui::ColorEdit4(LanguageManager::Tr("Base Color"),
                        &SetEffectDefinitionData_.color.x);
      ImGui::DragFloat(LanguageManager::Tr("LifeTime"),
                       &SetEffectDefinitionData_.lifeTime, 0.01f, 0.0f, FLT_MAX,
                       "%.2f");
      ImGui::TreePop();
    }

    if (ImGui::TreeNode(LanguageManager::Tr("Scale Over Lifetime"))) {
      ImGui::Checkbox(LanguageManager::Tr("Enable Scale Over Lifetime"),
                      &enableScaleOverLifetime_);
      if (enableScaleOverLifetime_) {
        const char *curves[] = {"Linear", "BellCurve (0 -> 1 -> 0)"};
        ImGui::Combo(LanguageManager::Tr("Curve Type"), &scaleCurveType_,
                     curves, IM_ARRAYSIZE(curves));
        if (scaleCurveType_ == 0) {
          ImGui::DragFloat3(LanguageManager::Tr("Start Scale"), &startScale_.x,
                            0.01f, 0.0f, FLT_MAX, "%.2f");
          ImGui::DragFloat3(LanguageManager::Tr("End Scale"), &endScale_.x,
                            0.01f, 0.0f, FLT_MAX, "%.2f");
        } else {
          ImGui::DragFloat3(LanguageManager::Tr("Peak Scale"), &startScale_.x,
                            0.01f, 0.0f, FLT_MAX, "%.2f");
        }
      }
      ImGui::TreePop();
    }

    if (ImGui::TreeNode(LanguageManager::Tr("Color Over Lifetime"))) {
      ImGui::Checkbox(LanguageManager::Tr("Enable Color Over Lifetime"),
                      &enableColorOverLifetime_);
      if (enableColorOverLifetime_) {
        ImGui::ColorEdit4(LanguageManager::Tr("Start Color"), &startColor_.x);
        ImGui::ColorEdit4(LanguageManager::Tr("End Color"), &endColor_.x);
      }
      ImGui::TreePop();
    }

    if (ImGui::TreeNode(LanguageManager::Tr("Particle Movement Settings"))) {
      ImGui::DragFloat3(LanguageManager::Tr("Base Velocity"),
                        &movementData_.baseVelocity.x, 0.001f, -FLT_MAX,
                        FLT_MAX, "%.3f");
      ImGui::DragFloat3(LanguageManager::Tr("Velocity Variance"),
                        &movementData_.velocityVariance.x, 0.001f, 0.0f,
                        FLT_MAX, "%.3f");
      ImGui::DragFloat(LanguageManager::Tr("Radial Speed"),
                       &movementData_.radialSpeed, 0.001f, -FLT_MAX, FLT_MAX,
                       "%.3f");
      ImGui::DragFloat(LanguageManager::Tr("Radial Speed Variance"),
                       &movementData_.radialSpeedVariance, 0.001f, 0.0f,
                       FLT_MAX, "%.3f");
      ImGui::DragFloat3(LanguageManager::Tr("Acceleration (Gravity)"),
                        &movementData_.acceleration.x, 0.0001f, -FLT_MAX,
                        FLT_MAX, "%.4f");
      ImGui::DragFloat3(LanguageManager::Tr("Size Variance"),
                        &movementData_.sizeVariance.x, 0.01f, 0.0f, FLT_MAX,
                        "%.2f");
      ImGui::DragFloat3(LanguageManager::Tr("Size Multiplier"),
                        &movementData_.sizeDelta.x, 0.001f, 0.0f, FLT_MAX,
                        "%.3f");

      if (ImGui::TreeNode(LanguageManager::Tr("Field Settings (GPU Only)"))) {
        const char *fieldTypes[] = {"None", "PointGravity (Attractor)",
                                    "Vortex"};
        int currentFieldType = static_cast<int>(fieldData_.type);
        if (ImGui::Combo(LanguageManager::Tr("Field Type"), &currentFieldType,
                         fieldTypes, IM_ARRAYSIZE(fieldTypes))) {
          fieldData_.type = static_cast<FieldType>(currentFieldType);
        }
        if (fieldData_.type != FieldType::None) {
          ImGui::DragFloat(LanguageManager::Tr("Field Strength"),
                           &fieldData_.strength, 0.01f, -FLT_MAX, FLT_MAX,
                           "%.3f");
          ImGui::DragFloat3(LanguageManager::Tr("Field Position"),
                            &fieldData_.position.x, 0.01f, -FLT_MAX, FLT_MAX,
                            "%.2f");
        }
        ImGui::TreePop();
      }

      ImGui::TreePop();
    }

    if (ImGui::TreeNode(LanguageManager::Tr("Visual Settings"))) {
      bool useGpu =
          effectDefinition_ ? effectDefinition_->GetUseGpuParticle() : true;
      if (ImGui::Checkbox(
              LanguageManager::Tr("Use GPU Particle (Compute Shader)"),
              &useGpu)) {
        SetUseGpuParticle(useGpu);
      }

      bool isBillboard = GetBillboard();
      if (ImGui::Checkbox(LanguageManager::Tr("Billboard"), &isBillboard)) {
        SetBillboard(isBillboard);
      }

      const char *shapes[] = {"Plane", "Cylinder", "Ring", "Cube"};
      int currentShape = static_cast<int>(shape_);
      if (ImGui::Combo(LanguageManager::Tr("Shape"), &currentShape, shapes,
                       IM_ARRAYSIZE(shapes))) {
        SetShape(static_cast<EffectShape>(currentShape));
      }

      const char *blendModes[] = {"None",     "Normal",   "Add",
                                  "Subtract", "Multiply", "Screen"};
      int currentBlend = static_cast<int>(GetBlend());
      if (ImGui::Combo(LanguageManager::Tr("Blend Mode"), &currentBlend,
                       blendModes, IM_ARRAYSIZE(blendModes))) {
        SetBlend(static_cast<BlendMode>(currentBlend));
      }

      std::vector<std::string> shaderNames;
      std::filesystem::path shaderDir = "Resources/Shader/ParticleShader";
      if (std::filesystem::exists(shaderDir)) {
        for (const auto &entry :
             std::filesystem::directory_iterator(shaderDir)) {
          std::string filename = entry.path().filename().string();
          if (filename.find(".PS.hlsl") != std::string::npos) {
            std::string sName =
                filename.substr(0, filename.find(".PS.hlsl")) + "Shader";
            shaderNames.push_back(sName);
          }
        }
      }

      if (ImGui::BeginCombo(LanguageManager::Tr("Shader"),
                            shaderName_.c_str())) {
        for (const auto &shader : shaderNames) {
          bool is_selected = (shaderName_ == shader);
          if (ImGui::Selectable(shader.c_str(), is_selected)) {
            SetShader(shader);
          }
          if (is_selected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      bool shapeChanged = false;
      if (shape_ == EffectShape::Cylinder) {
        shapeChanged |=
            ImGui::DragInt(LanguageManager::Tr("Cylinder Divide"),
                           &shapeData_.cylinderDivide, 1.0f, 3, 128);
        shapeChanged |= ImGui::DragFloat(
            LanguageManager::Tr("Cylinder Top Radius"),
            &shapeData_.cylinderTopRadius, 0.05f, 0.0f, 100.0f);
        shapeChanged |= ImGui::DragFloat(
            LanguageManager::Tr("Cylinder Bottom Radius"),
            &shapeData_.cylinderBottomRadius, 0.05f, 0.0f, 100.0f);
        shapeChanged |=
            ImGui::DragFloat(LanguageManager::Tr("Cylinder Height"),
                             &shapeData_.cylinderHeight, 0.05f, 0.0f, 100.0f);
      } else if (shape_ == EffectShape::Ring) {
        shapeChanged |= ImGui::DragInt(LanguageManager::Tr("Ring Divide"),
                                       &shapeData_.ringDivide, 1.0f, 3, 128);
        shapeChanged |=
            ImGui::DragFloat(LanguageManager::Tr("Ring Outer Radius"),
                             &shapeData_.ringOuterRadius, 0.05f, 0.0f, 100.0f);
        shapeChanged |=
            ImGui::DragFloat(LanguageManager::Tr("Ring Inner Radius"),
                             &shapeData_.ringInnerRadius, 0.05f, 0.0f, 100.0f);
      } else if (shape_ == EffectShape::Cube) {
        shapeChanged |=
            ImGui::DragFloat3(LanguageManager::Tr("Cube Size"),
                              &shapeData_.cubeSize.x, 0.05f, 0.0f, 100.0f);
      }
      if (shapeChanged) {
        SetShapeData(shapeData_);
      }

      ImGui::Text(LanguageManager::Tr("Texture:"));
      std::string btnLabel = texturePath_ + "##TexDrop";
      ImGui::Button(btnLabel.c_str(),
                    ImVec2(-FLT_MIN, 0)); // Button taking full width

      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload =
                ImGui::AcceptDragDropPayload("RESOURCE_FILE")) {
          std::string path((const char *)payload->Data, payload->DataSize - 1);
          if (path.find(".png") != std::string::npos ||
              path.find(".jpg") != std::string::npos ||
              path.find(".dds") != std::string::npos) {
            SetTexturePath(path);
          }
        }
        ImGui::EndDragDropTarget();
      }
      ImGui::TextDisabled(LanguageManager::Tr(
          "(Drag and drop an image from the Resources window)"));

      ImGui::TreePop();
    }

    if (ImGui::Button(LanguageManager::Tr("Add Particle"))) {
      Emit();
    }
    ImGui::Checkbox(LanguageManager::Tr("stop"), &isStop_);
    ImGui::Checkbox(LanguageManager::Tr("IsHit"), &isHit_);
    ImGui::Separator();
    ImGui::Text(LanguageManager::Tr("Active Particles: %d"),
                effectDefinition_.get()->GetEffectDefinitionNum());

    ImGui::Separator();
    ImGui::InputText(LanguageManager::Tr("File Name"), saveFileName_,
                     sizeof(saveFileName_));

    std::filesystem::path dir = "Resources/Json/Particle";
    std::vector<std::string> jsonFiles;
    if (std::filesystem::exists(dir)) {
      for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() == ".json") {
          jsonFiles.push_back(entry.path().filename().string());
        }
      }
    }

    if (ImGui::BeginCombo(LanguageManager::Tr("Saved Particles"),
                          saveFileName_)) {
      for (const auto &file : jsonFiles) {
        std::string nameWithoutExt = file;
        size_t extPos = nameWithoutExt.find(".json");
        if (extPos != std::string::npos) {
          nameWithoutExt = nameWithoutExt.substr(0, extPos);
        }

        bool is_selected = (std::string(saveFileName_) == nameWithoutExt);
        if (ImGui::Selectable(file.c_str(), is_selected)) {
          strncpy_s(saveFileName_, sizeof(saveFileName_),
                    nameWithoutExt.c_str(), _TRUNCATE);
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    if (ImGui::Button(LanguageManager::Tr("Save JSON"))) {
      SaveToJson(saveFileName_);
    }
    ImGui::SameLine();
    if (ImGui::Button(LanguageManager::Tr("Load JSON"))) {
      LoadFromJson(saveFileName_);
    }
  }
  ImGui::PopID();
#endif // _USE_IMGUI
}

void Emitter::SaveToJson(const std::string &name) {
  std::filesystem::path dir = "Resources/Json/Particle";
  if (!std::filesystem::exists(dir)) {
    std::filesystem::create_directories(dir);
  }

  std::string filepath = dir.string() + "/" + name;
  if (filepath.find(".json") == std::string::npos) {
    filepath += ".json";
  }

  nlohmann::json root;

  root["emitter"]["type"] = static_cast<int>(emitterType_);
  root["emitter"]["transform"]["translate"] = {emitter_.transform.translate.x,
                                               emitter_.transform.translate.y,
                                               emitter_.transform.translate.z};
  root["emitter"]["transform"]["rotate"] = {emitter_.transform.rotate.x,
                                            emitter_.transform.rotate.y,
                                            emitter_.transform.rotate.z};
  root["emitter"]["transform"]["scale"] = {emitter_.transform.scale.x,
                                           emitter_.transform.scale.y,
                                           emitter_.transform.scale.z};
  root["emitter"]["count"] = emitter_.count;
  root["emitter"]["frequency"] = emitter_.frequency;

  root["emitter"]["sphere"]["translate"] = {emitterSphere_.translate.x,
                                            emitterSphere_.translate.y,
                                            emitterSphere_.translate.z};
  root["emitter"]["sphere"]["radius"] = emitterSphere_.radius;
  root["emitter"]["sphere"]["count"] = emitterSphere_.count;
  root["emitter"]["sphere"]["frequency"] = emitterSphere_.frequency;

  root["emitter"]["circle"]["translate"] = {emitterCircle_.translate.x,
                                            emitterCircle_.translate.y,
                                            emitterCircle_.translate.z};
  root["emitter"]["circle"]["rotate"] = {emitterCircle_.rotate.x,
                                         emitterCircle_.rotate.y,
                                         emitterCircle_.rotate.z};
  root["emitter"]["circle"]["outerRadius"] = emitterCircle_.outerRadius;
  root["emitter"]["circle"]["innerRadius"] = emitterCircle_.innerRadius;
  root["emitter"]["circle"]["radialVelocity"] = emitterCircle_.radialVelocity;
  root["emitter"]["circle"]["count"] = emitterCircle_.count;
  root["emitter"]["circle"]["frequency"] = emitterCircle_.frequency;

  root["emitter"]["cone"]["translate"] = {emitterCone_.translate.x,
                                          emitterCone_.translate.y,
                                          emitterCone_.translate.z};
  root["emitter"]["cone"]["rotate"] = {
      emitterCone_.rotate.x, emitterCone_.rotate.y, emitterCone_.rotate.z};
  root["emitter"]["cone"]["radius"] = emitterCone_.radius;
  root["emitter"]["cone"]["angle"] = emitterCone_.angle;
  root["emitter"]["cone"]["speed"] = emitterCone_.speed;
  root["emitter"]["cone"]["count"] = emitterCone_.count;
  root["emitter"]["cone"]["frequency"] = emitterCone_.frequency;

  root["spawn"]["isLoop"] = isLoop_;
  root["spawn"]["burstCount"] = burstCount_;

  root["particle"]["transform"]["translate"] = {
      SetEffectDefinitionData_.transform.translate.x,
      SetEffectDefinitionData_.transform.translate.y,
      SetEffectDefinitionData_.transform.translate.z};
  root["particle"]["transform"]["scale"] = {
      SetEffectDefinitionData_.transform.scale.x,
      SetEffectDefinitionData_.transform.scale.y,
      SetEffectDefinitionData_.transform.scale.z};
  root["particle"]["transform"]["rotate"] = {
      SetEffectDefinitionData_.transform.rotate.x,
      SetEffectDefinitionData_.transform.rotate.y,
      SetEffectDefinitionData_.transform.rotate.z};
  root["particle"]["color"] = {
      SetEffectDefinitionData_.color.x, SetEffectDefinitionData_.color.y,
      SetEffectDefinitionData_.color.z, SetEffectDefinitionData_.color.w};
  root["particle"]["lifeTime"] = SetEffectDefinitionData_.lifeTime;
  root["particle"]["alignToVelocity"] = alignToVelocity_;

  root["scaleOverLifetime"]["enable"] = enableScaleOverLifetime_;
  root["scaleOverLifetime"]["curveType"] = scaleCurveType_;
  root["scaleOverLifetime"]["startScale"] = {startScale_.x, startScale_.y,
                                             startScale_.z};
  root["scaleOverLifetime"]["endScale"] = {endScale_.x, endScale_.y,
                                           endScale_.z};

  root["colorOverLifetime"]["enable"] = enableColorOverLifetime_;
  root["colorOverLifetime"]["startColor"] = {startColor_.x, startColor_.y,
                                             startColor_.z, startColor_.w};
  root["colorOverLifetime"]["endColor"] = {endColor_.x, endColor_.y,
                                           endColor_.z, endColor_.w};

  root["movement"]["baseVelocity"] = {movementData_.baseVelocity.x,
                                      movementData_.baseVelocity.y,
                                      movementData_.baseVelocity.z};
  root["movement"]["velocityVariance"] = {movementData_.velocityVariance.x,
                                          movementData_.velocityVariance.y,
                                          movementData_.velocityVariance.z};
  root["movement"]["radialSpeed"] = movementData_.radialSpeed;
  root["movement"]["radialSpeedVariance"] = movementData_.radialSpeedVariance;
  root["movement"]["acceleration"] = {movementData_.acceleration.x,
                                      movementData_.acceleration.y,
                                      movementData_.acceleration.z};
  root["movement"]["sizeVariance"] = {movementData_.sizeVariance.x,
                                      movementData_.sizeVariance.y,
                                      movementData_.sizeVariance.z};
  root["movement"]["sizeDelta"] = {movementData_.sizeDelta.x,
                                   movementData_.sizeDelta.y,
                                   movementData_.sizeDelta.z};

  root["field"]["type"] = static_cast<int>(fieldData_.type);
  root["field"]["strength"] = fieldData_.strength;
  root["field"]["position"] = {fieldData_.position.x, fieldData_.position.y,
                               fieldData_.position.z};

  root["visual"]["texturePath"] = texturePath_;
  root["visual"]["shape"] = static_cast<int>(shape_);
  root["visual"]["blendMode"] = static_cast<int>(GetBlend());
  root["visual"]["isBillboard"] = GetBillboard();
  root["visual"]["shader"] = shaderName_;
  root["visual"]["useGpuParticle"] = GetUseGpuParticle();

  root["visual"]["cylinderDivide"] = shapeData_.cylinderDivide;
  root["visual"]["cylinderTopRadius"] = shapeData_.cylinderTopRadius;
  root["visual"]["cylinderBottomRadius"] = shapeData_.cylinderBottomRadius;
  root["visual"]["cylinderHeight"] = shapeData_.cylinderHeight;

  root["visual"]["ringDivide"] = shapeData_.ringDivide;
  root["visual"]["ringOuterRadius"] = shapeData_.ringOuterRadius;
  root["visual"]["ringInnerRadius"] = shapeData_.ringInnerRadius;

  root["visual"]["cubeSize"] = {shapeData_.cubeSize.x, shapeData_.cubeSize.y,
                                shapeData_.cubeSize.z};

  std::ofstream file(filepath);
  if (file.is_open()) {
    file << root.dump(4);
  }
}

void Emitter::SyncGpuParticleParameters(bool emitNow) {
  if (!effectDefinition_)
    return;

  effectDefinition_->SetIsBoxEmitter(emitterType_ == EmitterType::Box);

  static float globalTime = 0.0f;
  globalTime += 1.0f / 60.0f;

  PerFrameForGPU perFrame{};
  perFrame.deltaTime = 1.0f / 60.0f;
  perFrame.time = globalTime;
  perFrame.acceleration = movementData_.acceleration;
  perFrame.sizeDelta = movementData_.sizeDelta;

  perFrame.fieldType = static_cast<uint32_t>(fieldData_.type);
  perFrame.fieldStrength = fieldData_.strength;
  perFrame.fieldPosition = fieldData_.position;

  effectDefinition_->SetGpuPerFrameData(perFrame);

  if (emitterType_ == EmitterType::Box) {
    EmitterBoxForGPU box{};
    box.translate = emitter_.transform.translate +
                    SetEffectDefinitionData_.transform.translate;
    box.size = emitter_.transform.scale;
    box.count = static_cast<float>(emitter_.count);
    box.frequency = emitter_.frequency;
    box.emit = emitNow ? 1 : 0;
    box.baseScale = SetEffectDefinitionData_.transform.scale;
    box.sizeVariance = movementData_.sizeVariance;
    box.baseVelocity = movementData_.baseVelocity;
    box.velocityVariance = movementData_.velocityVariance;
    box.color = SetEffectDefinitionData_.color;
    box.lifeTime = SetEffectDefinitionData_.lifeTime;
    box.baseRotate =
        SetEffectDefinitionData_.transform.rotate + emitter_.transform.rotate;
    effectDefinition_->SetGpuEmitterBoxData(box);
  } else if (emitterType_ == EmitterType::Sphere) {
    EmitterSphereForGPU sphere{};
    sphere.translate =
        emitterSphere_.translate + SetEffectDefinitionData_.transform.translate;
    sphere.radius = emitterSphere_.radius;
    sphere.count = static_cast<float>(emitterSphere_.count);
    sphere.frequency = emitterSphere_.frequency;
    sphere.emit = emitNow ? 1 : 0;
    sphere.baseScale = SetEffectDefinitionData_.transform.scale;
    sphere.sizeVariance = movementData_.sizeVariance;
    sphere.baseVelocity = movementData_.baseVelocity;
    sphere.velocityVariance = movementData_.velocityVariance;
    sphere.color = SetEffectDefinitionData_.color;
    sphere.lifeTime = SetEffectDefinitionData_.lifeTime;
    sphere.baseRotate = SetEffectDefinitionData_.transform.rotate;
    effectDefinition_->SetGpuEmitterSphereData(sphere);
  } else {
    // Circle や Cone は GPU コンピュートシェーダーが存在しないため、CPU
    // パーティクルに自動切り替え
    SetUseGpuParticle(false);
  }
}

void Emitter::LoadFromJson(const std::string &name) {
  std::string filepath = name;
  if (filepath.find("Resources/Json/Particle/") == std::string::npos &&
      filepath.find("Resources\\Json\\Particle\\") == std::string::npos) {
    filepath = "Resources/Json/Particle/" + filepath;
  }
  if (filepath.find(".json") == std::string::npos) {
    filepath += ".json";
  }

  std::ifstream file(filepath);
  if (!file.is_open()) {
    LOG_ERROR(std::format("Emitter::LoadFromJson: Failed to open JSON file '{}'", filepath));
    return;
  }

  nlohmann::json root;
  try {
    file >> root;
  } catch (const std::exception& e) {
    LOG_ERROR(std::format("Emitter::LoadFromJson: JSON parse error in '{}': {}", filepath, e.what()));
    return;
  }

  if (root.contains("emitter")) {
    if (root["emitter"].contains("type")) {
      emitterType_ =
          static_cast<EmitterType>(root["emitter"]["type"].get<int>());
    }
    if (root["emitter"].contains("transform")) {
      emitter_.transform.translate.x =
          root["emitter"]["transform"]["translate"][0];
      emitter_.transform.translate.y =
          root["emitter"]["transform"]["translate"][1];
      emitter_.transform.translate.z =
          root["emitter"]["transform"]["translate"][2];

      emitter_.transform.rotate.x = root["emitter"]["transform"]["rotate"][0];
      emitter_.transform.rotate.y = root["emitter"]["transform"]["rotate"][1];
      emitter_.transform.rotate.z = root["emitter"]["transform"]["rotate"][2];

      emitter_.transform.scale.x = root["emitter"]["transform"]["scale"][0];
      emitter_.transform.scale.y = root["emitter"]["transform"]["scale"][1];
      emitter_.transform.scale.z = root["emitter"]["transform"]["scale"][2];
    }
    if (root["emitter"].contains("count"))
      emitter_.count = root["emitter"]["count"];
    if (root["emitter"].contains("frequency"))
      emitter_.frequency = root["emitter"]["frequency"];

    if (root["emitter"].contains("sphere")) {
      if (root["emitter"]["sphere"].contains("translate")) {
        emitterSphere_.translate.x = root["emitter"]["sphere"]["translate"][0];
        emitterSphere_.translate.y = root["emitter"]["sphere"]["translate"][1];
        emitterSphere_.translate.z = root["emitter"]["sphere"]["translate"][2];
      }
      if (root["emitter"]["sphere"].contains("radius"))
        emitterSphere_.radius = root["emitter"]["sphere"]["radius"];
      if (root["emitter"]["sphere"].contains("count"))
        emitterSphere_.count = root["emitter"]["sphere"]["count"];
      if (root["emitter"]["sphere"].contains("frequency"))
        emitterSphere_.frequency = root["emitter"]["sphere"]["frequency"];
    }

    if (root["emitter"].contains("circle")) {
      if (root["emitter"]["circle"].contains("translate")) {
        emitterCircle_.translate.x = root["emitter"]["circle"]["translate"][0];
        emitterCircle_.translate.y = root["emitter"]["circle"]["translate"][1];
        emitterCircle_.translate.z = root["emitter"]["circle"]["translate"][2];
      }
      if (root["emitter"]["circle"].contains("rotate")) {
        emitterCircle_.rotate.x = root["emitter"]["circle"]["rotate"][0];
        emitterCircle_.rotate.y = root["emitter"]["circle"]["rotate"][1];
        emitterCircle_.rotate.z = root["emitter"]["circle"]["rotate"][2];
      }
      if (root["emitter"]["circle"].contains("outerRadius"))
        emitterCircle_.outerRadius = root["emitter"]["circle"]["outerRadius"];
      if (root["emitter"]["circle"].contains("innerRadius"))
        emitterCircle_.innerRadius = root["emitter"]["circle"]["innerRadius"];
      if (root["emitter"]["circle"].contains("radialVelocity"))
        emitterCircle_.radialVelocity =
            root["emitter"]["circle"]["radialVelocity"];
      if (root["emitter"]["circle"].contains("count"))
        emitterCircle_.count = root["emitter"]["circle"]["count"];
      if (root["emitter"]["circle"].contains("frequency"))
        emitterCircle_.frequency = root["emitter"]["circle"]["frequency"];
    }

    if (root["emitter"].contains("cone")) {
      if (root["emitter"]["cone"].contains("translate")) {
        emitterCone_.translate.x = root["emitter"]["cone"]["translate"][0];
        emitterCone_.translate.y = root["emitter"]["cone"]["translate"][1];
        emitterCone_.translate.z = root["emitter"]["cone"]["translate"][2];
      }
      if (root["emitter"]["cone"].contains("rotate")) {
        emitterCone_.rotate.x = root["emitter"]["cone"]["rotate"][0];
        emitterCone_.rotate.y = root["emitter"]["cone"]["rotate"][1];
        emitterCone_.rotate.z = root["emitter"]["cone"]["rotate"][2];
      }
      if (root["emitter"]["cone"].contains("radius"))
        emitterCone_.radius = root["emitter"]["cone"]["radius"];
      if (root["emitter"]["cone"].contains("angle"))
        emitterCone_.angle = root["emitter"]["cone"]["angle"];
      if (root["emitter"]["cone"].contains("speed"))
        emitterCone_.speed = root["emitter"]["cone"]["speed"];
      if (root["emitter"]["cone"].contains("count"))
        emitterCone_.count = root["emitter"]["cone"]["count"];
      if (root["emitter"]["cone"].contains("frequency"))
        emitterCone_.frequency = root["emitter"]["cone"]["frequency"];
    }
  }

  if (root.contains("spawn")) {
    if (root["spawn"].contains("isLoop"))
      isLoop_ = root["spawn"]["isLoop"].get<bool>();
    if (root["spawn"].contains("burstCount"))
      burstCount_ = root["spawn"]["burstCount"].get<uint32_t>();
  }

  if (root.contains("scaleOverLifetime")) {
    if (root["scaleOverLifetime"].contains("enable"))
      enableScaleOverLifetime_ =
          root["scaleOverLifetime"]["enable"].get<bool>();
    if (root["scaleOverLifetime"].contains("curveType"))
      scaleCurveType_ = root["scaleOverLifetime"]["curveType"].get<int>();
    if (root["scaleOverLifetime"].contains("startScale")) {
      startScale_.x = root["scaleOverLifetime"]["startScale"][0];
      startScale_.y = root["scaleOverLifetime"]["startScale"][1];
      startScale_.z = root["scaleOverLifetime"]["startScale"][2];
    }
    if (root["scaleOverLifetime"].contains("endScale")) {
      endScale_.x = root["scaleOverLifetime"]["endScale"][0];
      endScale_.y = root["scaleOverLifetime"]["endScale"][1];
      endScale_.z = root["scaleOverLifetime"]["endScale"][2];
    }
  }

  if (root.contains("colorOverLifetime")) {
    if (root["colorOverLifetime"].contains("enable"))
      enableColorOverLifetime_ =
          root["colorOverLifetime"]["enable"].get<bool>();
    if (root["colorOverLifetime"].contains("startColor")) {
      startColor_.x = root["colorOverLifetime"]["startColor"][0];
      startColor_.y = root["colorOverLifetime"]["startColor"][1];
      startColor_.z = root["colorOverLifetime"]["startColor"][2];
      startColor_.w = root["colorOverLifetime"]["startColor"][3];
    }
    if (root["colorOverLifetime"].contains("endColor")) {
      endColor_.x = root["colorOverLifetime"]["endColor"][0];
      endColor_.y = root["colorOverLifetime"]["endColor"][1];
      endColor_.z = root["colorOverLifetime"]["endColor"][2];
      endColor_.w = root["colorOverLifetime"]["endColor"][3];
    }
  }

  if (root.contains("particle")) {
    if (root["particle"].contains("transform")) {
      if (root["particle"]["transform"].contains("translate")) {
        SetEffectDefinitionData_.transform.translate.x =
            root["particle"]["transform"]["translate"][0];
        SetEffectDefinitionData_.transform.translate.y =
            root["particle"]["transform"]["translate"][1];
        SetEffectDefinitionData_.transform.translate.z =
            root["particle"]["transform"]["translate"][2];
      }
      if (root["particle"]["transform"].contains("scale")) {
        SetEffectDefinitionData_.transform.scale.x =
            root["particle"]["transform"]["scale"][0];
        SetEffectDefinitionData_.transform.scale.y =
            root["particle"]["transform"]["scale"][1];
        SetEffectDefinitionData_.transform.scale.z =
            root["particle"]["transform"]["scale"][2];
      }
      if (root["particle"]["transform"].contains("rotate")) {
        SetEffectDefinitionData_.transform.rotate.x =
            root["particle"]["transform"]["rotate"][0];
        SetEffectDefinitionData_.transform.rotate.y =
            root["particle"]["transform"]["rotate"][1];
        SetEffectDefinitionData_.transform.rotate.z =
            root["particle"]["transform"]["rotate"][2];
      }
    }
    if (root["particle"].contains("color")) {
      SetEffectDefinitionData_.color.x = root["particle"]["color"][0];
      SetEffectDefinitionData_.color.y = root["particle"]["color"][1];
      SetEffectDefinitionData_.color.z = root["particle"]["color"][2];
      SetEffectDefinitionData_.color.w = root["particle"]["color"][3];
    }
    if (root["particle"].contains("lifeTime")) {
      SetEffectDefinitionData_.lifeTime = root["particle"]["lifeTime"];
    }
    if (root["particle"].contains("alignToVelocity")) {
      alignToVelocity_ = root["particle"]["alignToVelocity"].get<bool>();
    }
  }

  if (root.contains("movement")) {
    if (root["movement"].contains("baseVelocity")) {
      movementData_.baseVelocity.x = root["movement"]["baseVelocity"][0];
      movementData_.baseVelocity.y = root["movement"]["baseVelocity"][1];
      movementData_.baseVelocity.z = root["movement"]["baseVelocity"][2];
    }
    if (root["movement"].contains("velocityVariance")) {
      movementData_.velocityVariance.x =
          root["movement"]["velocityVariance"][0];
      movementData_.velocityVariance.y =
          root["movement"]["velocityVariance"][1];
      movementData_.velocityVariance.z =
          root["movement"]["velocityVariance"][2];
    }
    if (root["movement"].contains("radialSpeed")) {
      movementData_.radialSpeed = root["movement"]["radialSpeed"].get<float>();
    }
    if (root["movement"].contains("radialSpeedVariance")) {
      movementData_.radialSpeedVariance =
          root["movement"]["radialSpeedVariance"].get<float>();
    }
    if (root["movement"].contains("acceleration")) {
      movementData_.acceleration.x = root["movement"]["acceleration"][0];
      movementData_.acceleration.y = root["movement"]["acceleration"][1];
      movementData_.acceleration.z = root["movement"]["acceleration"][2];
    }
    if (root["movement"].contains("sizeVariance")) {
      movementData_.sizeVariance.x = root["movement"]["sizeVariance"][0];
      movementData_.sizeVariance.y = root["movement"]["sizeVariance"][1];
      movementData_.sizeVariance.z = root["movement"]["sizeVariance"][2];
    }
    if (root["movement"].contains("sizeDelta")) {
      if (root["movement"]["sizeDelta"].is_array()) {
        movementData_.sizeDelta.x = root["movement"]["sizeDelta"][0];
        movementData_.sizeDelta.y = root["movement"]["sizeDelta"][1];
        movementData_.sizeDelta.z = root["movement"]["sizeDelta"][2];
      } else {
        // Fallback for old save format
        float delta = root["movement"]["sizeDelta"];
        movementData_.sizeDelta = {delta, delta, delta};
      }
    }
  } else if (root.contains("particle") &&
             root["particle"].contains("velocity")) {
    // Fallback for old save format
    movementData_.baseVelocity.x = root["particle"]["velocity"][0];
    movementData_.baseVelocity.y = root["particle"]["velocity"][1];
    movementData_.baseVelocity.z = root["particle"]["velocity"][2];
  }

  if (root.contains("field")) {
    if (root["field"].contains("type"))
      fieldData_.type =
          static_cast<FieldType>(root["field"]["type"].get<int>());
    if (root["field"].contains("strength"))
      fieldData_.strength = root["field"]["strength"];
    if (root["field"].contains("position")) {
      fieldData_.position.x = root["field"]["position"][0];
      fieldData_.position.y = root["field"]["position"][1];
      fieldData_.position.z = root["field"]["position"][2];
    }
  }

  if (root.contains("visual")) {
    if (root["visual"].contains("texturePath")) {
      SetTexturePath(root["visual"]["texturePath"].get<std::string>());
    }

    if (root["visual"].contains("cylinderDivide")) {
      shapeData_.cylinderDivide = root["visual"]["cylinderDivide"];
      shapeData_.cylinderTopRadius = root["visual"]["cylinderTopRadius"];
      shapeData_.cylinderBottomRadius = root["visual"]["cylinderBottomRadius"];
      shapeData_.cylinderHeight = root["visual"]["cylinderHeight"];
      shapeData_.ringDivide = root["visual"]["ringDivide"];
      shapeData_.ringOuterRadius = root["visual"]["ringOuterRadius"];
      shapeData_.ringInnerRadius = root["visual"]["ringInnerRadius"];
    }

    if (root["visual"].contains("cubeSize")) {
      shapeData_.cubeSize.x = root["visual"]["cubeSize"][0];
      shapeData_.cubeSize.y = root["visual"]["cubeSize"][1];
      shapeData_.cubeSize.z = root["visual"]["cubeSize"][2];
    }

    if (root["visual"].contains("shape")) {
      SetShape(static_cast<EffectShape>(root["visual"]["shape"].get<int>()));
    }
    if (root["visual"].contains("blendMode")) {
      SetBlend(static_cast<BlendMode>(root["visual"]["blendMode"].get<int>()));
    }
    if (root["visual"].contains("isBillboard")) {
      SetBillboard(root["visual"]["isBillboard"].get<bool>());
    }
    if (root["visual"].contains("shader")) {
      SetShader(root["visual"]["shader"].get<std::string>());
    }
    if (root["visual"].contains("useGpuParticle")) {
      SetUseGpuParticle(root["visual"]["useGpuParticle"].get<bool>());
    }
  }

  // ロード完了後のリセットと即時プレビュー射出
  ClearParticles();
  emitter_.frequencyTime = 0.0f;
  emitterSphere_.frequencyTime = 0.0f;
  emitterCircle_.frequencyTime = 0.0f;
  emitterCone_.frequencyTime = 0.0f;
  isStop_ = false;

  if (isLoop_) {
    Emit();
  } else {
    TriggerBurst();
  }
  if (GetUseGpuParticle()) {
    SyncGpuParticleParameters(true);
  }
  LOG_INFO(std::format("Emitter::LoadFromJson: Successfully loaded particle from '{}' (isLoop={}, type={}, activeParticles={})",
                       filepath, isLoop_, static_cast<int>(emitterType_), effectDefinitionData_.size()));
}

void Emitter::Initialize(EffectShape shape) {
  shape_ = shape;
  randomEngine.seed(seedGenerator_());
  effectDefinition_.get()->Initialize(shape_);
  effectDefinition_->SetShader(shaderName_);
}

void Emitter::Initialize(EmitterData emitter, EffectShape shape) {
  shape_ = shape;
  emitter_ = emitter;
  randomEngine.seed(seedGenerator_());
  effectDefinition_.get()->Initialize(shape_);
  effectDefinition_->SetShader(shaderName_);
}

void Emitter::Initialize(EmitterData emitter,
                         EffectDefinitionData effectDefinitionData,
                         EffectShape shape) {
  shape_ = shape;
  emitter_ = emitter;
  SetEffectDefinitionData_ = effectDefinitionData;
  randomEngine.seed(seedGenerator_());
  effectDefinition_.get()->Initialize(shape_);
  effectDefinition_->SetShader(shaderName_);
}

void Emitter::Initialize(EmitterData emitter, EffectDefinitionData particleData,
                         int TextureHandle, EffectShape shape) {
  emitter_ = emitter;
  emitterType_ = EmitterType::Box;
  SetEffectDefinitionData_ = particleData;
  randomEngine.seed(seedGenerator_());
  effectDefinition_.get()->Initialize(TextureHandle, shape);
  effectDefinition_->SetShader(shaderName_);
}

void Emitter::Initialize(EmitterSphere emitterSphere, EffectShape shape) {
  shape_ = shape;
  emitterSphere_ = emitterSphere;
  emitterType_ = EmitterType::Sphere;
  randomEngine.seed(seedGenerator_());
  effectDefinition_.get()->Initialize(shape_);
  effectDefinition_->SetShader(shaderName_);
}

void Emitter::Initialize(EmitterSphere emitterSphere,
                         EffectDefinitionData effectDefinitionData,
                         EffectShape shape) {
  shape_ = shape;
  emitterSphere_ = emitterSphere;
  emitterType_ = EmitterType::Sphere;
  SetEffectDefinitionData_ = effectDefinitionData;
  randomEngine.seed(seedGenerator_());
  effectDefinition_.get()->Initialize(shape_);
  effectDefinition_->SetShader(shaderName_);
}

void Emitter::Initialize(EmitterSphere emitterSphere,
                         EffectDefinitionData particleData, int TextureHandle,
                         EffectShape shape) {
  emitterSphere_ = emitterSphere;
  emitterType_ = EmitterType::Sphere;
  SetEffectDefinitionData_ = particleData;
  randomEngine.seed(seedGenerator_());
  effectDefinition_.get()->Initialize(TextureHandle, shape);
  effectDefinition_->SetShader(shaderName_);
}

void Emitter::Update(Matrix4x4 viewMatrix) {

  for (std::list<EffectDefinitionData>::iterator particleIterator =
           effectDefinitionData_.begin();
       particleIterator != effectDefinitionData_.end();) {

    if (particleIterator->currentTime >= particleIterator->lifeTime) {
      particleIterator = effectDefinitionData_.erase(particleIterator);
      continue;
    }

    particleIterator->velocity += movementData_.acceleration;
    particleIterator->transform.translate += particleIterator->velocity;

    float t = particleIterator->currentTime / particleIterator->lifeTime;
    t = std::clamp(t, 0.0f, 1.0f);

    if (particleIterator->enableScaleOverLifetime) {
      if (particleIterator->scaleCurveType == 1) { // BellCurve
        float bell = std::sin(t * 3.1415926535f);
        particleIterator->transform.scale = particleIterator->baseScale * bell;
      } else { // Linear
        particleIterator->transform.scale = {
            particleIterator->baseScale.x +
                (particleIterator->endScale.x - particleIterator->baseScale.x) *
                    t,
            particleIterator->baseScale.y +
                (particleIterator->endScale.y - particleIterator->baseScale.y) *
                    t,
            particleIterator->baseScale.z +
                (particleIterator->endScale.z - particleIterator->baseScale.z) *
                    t};
      }
    } else {
      particleIterator->transform.scale.x *= movementData_.sizeDelta.x;
      particleIterator->transform.scale.y *= movementData_.sizeDelta.y;
      particleIterator->transform.scale.z *= movementData_.sizeDelta.z;
    }

    if (particleIterator->alignToVelocity) {
      float lenSq =
          particleIterator->velocity.x * particleIterator->velocity.x +
          particleIterator->velocity.y * particleIterator->velocity.y +
          particleIterator->velocity.z * particleIterator->velocity.z;
      if (lenSq > 0.000001f) {
        particleIterator->transform.rotate.y = std::atan2(
            particleIterator->velocity.x, particleIterator->velocity.z);
        float horizDist = std::sqrt(
            particleIterator->velocity.x * particleIterator->velocity.x +
            particleIterator->velocity.z * particleIterator->velocity.z);
        particleIterator->transform.rotate.x =
            -std::atan2(particleIterator->velocity.y, horizDist);
      }
    }

    if (particleIterator->enableColorOverLifetime) {
      particleIterator->color = {
          particleIterator->startColor.x +
              (particleIterator->endColor.x - particleIterator->startColor.x) *
                  t,
          particleIterator->startColor.y +
              (particleIterator->endColor.y - particleIterator->startColor.y) *
                  t,
          particleIterator->startColor.z +
              (particleIterator->endColor.z - particleIterator->startColor.z) *
                  t,
          particleIterator->startColor.w +
              (particleIterator->endColor.w - particleIterator->startColor.w) *
                  t};
    } else {
      particleIterator->color.w -= 1.0f / (particleIterator->lifeTime * 60.0f);
    }

    particleIterator->currentTime += 1.0f / 60.0f;
    ++particleIterator;
  }

  bool emitFrame = false;
  if (manualEmitTriggered_) {
    emitFrame = true;
    manualEmitTriggered_ = false;
  }
  if (!isStop_ && isLoop_) {
    float *pFreqTime = &emitter_.frequencyTime;
    float freq = emitter_.frequency;
    switch (emitterType_) {
    case EmitterType::Sphere:
      pFreqTime = &emitterSphere_.frequencyTime;
      freq = emitterSphere_.frequency;
      break;
    case EmitterType::Circle:
      pFreqTime = &emitterCircle_.frequencyTime;
      freq = emitterCircle_.frequency;
      break;
    case EmitterType::Cone:
      pFreqTime = &emitterCone_.frequencyTime;
      freq = emitterCone_.frequency;
      break;
    case EmitterType::Box:
    default:
      break;
    }

    *pFreqTime += 1.0f / 60.0f;
    if (freq <= 0.0f || freq <= *pFreqTime) {
      Emit();
      emitFrame = true;
      manualEmitTriggered_ = false;
      if (freq > 0.0f)
        *pFreqTime -= freq;
    }
  }

  effectDefinition_.get()->Updata(viewMatrix, effectDefinitionData_);
  if (GetUseGpuParticle()) {
    SyncGpuParticleParameters(emitFrame);
  }
}

void Emitter::Update(
    Matrix4x4 viewMatrix,
    std::function<EffectDefinitionData(const EffectDefinitionData &)>
        moveBehavior) {
  for (std::list<EffectDefinitionData>::iterator particleIterator =
           effectDefinitionData_.begin();
       particleIterator != effectDefinitionData_.end();) {

    if (particleIterator->currentTime >= particleIterator->lifeTime) {
      particleIterator = effectDefinitionData_.erase(particleIterator);
      continue;
    }

    EffectDefinitionData updated = *particleIterator;
    if (moveBehavior) {
      updated = moveBehavior(*particleIterator);
    }
    particleIterator->velocity = updated.velocity + movementData_.acceleration;

    if (OnCollision(*particleIterator) && isHit_) {
      (*particleIterator).velocity += accelerationFiled_.acceleration / 60.0f;
    }

    particleIterator->transform.translate =
        updated.transform.translate + particleIterator->velocity;
    particleIterator->transform.rotate = updated.transform.rotate;

    float t = particleIterator->currentTime / particleIterator->lifeTime;
    t = std::clamp(t, 0.0f, 1.0f);

    if (particleIterator->enableScaleOverLifetime) {
      if (particleIterator->scaleCurveType == 1) {
        float bell = std::sin(t * 3.1415926535f);
        particleIterator->transform.scale = particleIterator->baseScale * bell;
      } else {
        particleIterator->transform.scale = {
            particleIterator->baseScale.x +
                (particleIterator->endScale.x - particleIterator->baseScale.x) *
                    t,
            particleIterator->baseScale.y +
                (particleIterator->endScale.y - particleIterator->baseScale.y) *
                    t,
            particleIterator->baseScale.z +
                (particleIterator->endScale.z - particleIterator->baseScale.z) *
                    t};
      }
    } else {
      particleIterator->transform.scale.x =
          updated.transform.scale.x * movementData_.sizeDelta.x;
      particleIterator->transform.scale.y =
          updated.transform.scale.y * movementData_.sizeDelta.y;
      particleIterator->transform.scale.z =
          updated.transform.scale.z * movementData_.sizeDelta.z;
    }

    if (particleIterator->alignToVelocity) {
      float lenSq =
          particleIterator->velocity.x * particleIterator->velocity.x +
          particleIterator->velocity.y * particleIterator->velocity.y +
          particleIterator->velocity.z * particleIterator->velocity.z;
      if (lenSq > 0.000001f) {
        particleIterator->transform.rotate.y = std::atan2(
            particleIterator->velocity.x, particleIterator->velocity.z);
        float horizDist = std::sqrt(
            particleIterator->velocity.x * particleIterator->velocity.x +
            particleIterator->velocity.z * particleIterator->velocity.z);
        particleIterator->transform.rotate.x =
            -std::atan2(particleIterator->velocity.y, horizDist);
      }
    }

    if (particleIterator->enableColorOverLifetime) {
      particleIterator->color = {
          particleIterator->startColor.x +
              (particleIterator->endColor.x - particleIterator->startColor.x) *
                  t,
          particleIterator->startColor.y +
              (particleIterator->endColor.y - particleIterator->startColor.y) *
                  t,
          particleIterator->startColor.z +
              (particleIterator->endColor.z - particleIterator->startColor.z) *
                  t,
          particleIterator->startColor.w +
              (particleIterator->endColor.w - particleIterator->startColor.w) *
                  t};
    } else {
      particleIterator->color.w -= 1.0f / (particleIterator->lifeTime * 60.0f);
    }

    particleIterator->currentTime += 1.0f / 60.0f;
    ++particleIterator;
  }

  bool emitFrame = false;
  if (manualEmitTriggered_) {
    emitFrame = true;
    manualEmitTriggered_ = false;
  }
  if (!isStop_ && isLoop_) {
    float *pFreqTime = &emitter_.frequencyTime;
    float freq = emitter_.frequency;
    switch (emitterType_) {
    case EmitterType::Sphere:
      pFreqTime = &emitterSphere_.frequencyTime;
      freq = emitterSphere_.frequency;
      break;
    case EmitterType::Circle:
      pFreqTime = &emitterCircle_.frequencyTime;
      freq = emitterCircle_.frequency;
      break;
    case EmitterType::Cone:
      pFreqTime = &emitterCone_.frequencyTime;
      freq = emitterCone_.frequency;
      break;
    case EmitterType::Box:
    default:
      break;
    }

    *pFreqTime += 1.0f / 60.0f;
    if (freq <= 0.0f || freq <= *pFreqTime) {
      Emit();
      emitFrame = true;
      manualEmitTriggered_ = false;
      if (freq > 0.0f)
        *pFreqTime -= freq;
    }
  }

  effectDefinition_.get()->Updata(viewMatrix, effectDefinitionData_);
  if (GetUseGpuParticle()) {
    SyncGpuParticleParameters(emitFrame);
  }
}

void Emitter::Update(
    EmitterData emitter, Matrix4x4 viewMatrix,
    std::function<EffectDefinitionData(const EffectDefinitionData &)>
        moveBehavior) {
  emitter_ = emitter;
  emitterType_ = EmitterType::Box;
  Update(viewMatrix, moveBehavior);
}

void Emitter::Update(Matrix4x4 viewMatrix, Vector3 scale) {
  SetEffectDefinitionData_.transform.scale = scale;
  Update(viewMatrix);
}

void Emitter::EditorUpdate(Matrix4x4 viewMatrix) {
  effectDefinition_.get()->Updata(viewMatrix, effectDefinitionData_);
  if (GetUseGpuParticle()) {
    SyncGpuParticleParameters(false);
  }
}

void Emitter::Draw(class Draw &draw) {
  draw.DrawParticle(effectDefinition_.get());
}

void Emitter::TriggerBurst() {
  for (uint32_t i = 0; i < burstCount_; ++i) {
    effectDefinitionData_.push_back(MakeNewParticle());
  }
  manualEmitTriggered_ = true;
}

EffectDefinitionData Emitter::MakeNewParticle() {
  std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
  std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

  EffectDefinitionData data = SetEffectDefinitionData_;

  Vector3 possion = {0.0f, 0.0f, 0.0f};
  Vector3 radialDir = {0.0f, 0.0f, 0.0f};
  float emitterRadialVel = 0.0f;

  if (emitterType_ == EmitterType::Box) {
    possion = {distribution(randomEngine), distribution(randomEngine),
               distribution(randomEngine)};
    data.transform.translate = SetEffectDefinitionData_.transform.translate +
                               possion * (emitter_.transform.scale * 0.5f) +
                               emitter_.transform.translate;
    float len = Length(possion);
    if (len > 0.0001f) {
      radialDir = Normalize(possion);
    }
  } else if (emitterType_ == EmitterType::Sphere) {
    do {
      possion = {distribution(randomEngine), distribution(randomEngine),
                 distribution(randomEngine)};
    } while (possion.x * possion.x + possion.y * possion.y +
                 possion.z * possion.z >
             1.0f);
    float len = Length(possion);
    if (len > 0.0001f) {
      radialDir = Normalize(possion);
    }
    data.transform.translate = SetEffectDefinitionData_.transform.translate +
                               possion * emitterSphere_.radius +
                               emitterSphere_.translate;
  } else if (emitterType_ == EmitterType::Circle) {
    float angle = dist01(randomEngine) * 2.0f * kPi;
    float rMin =
        (std::min)(emitterCircle_.innerRadius, emitterCircle_.outerRadius);
    float rMax =
        (std::max)(emitterCircle_.innerRadius, emitterCircle_.outerRadius);
    float u = dist01(randomEngine);
    float r = (rMin == rMax)
                  ? rMax
                  : std::sqrt(rMin * rMin + u * (rMax * rMax - rMin * rMin));
    Vector3 localCircle = {r * std::cos(angle), 0.0f, r * std::sin(angle)};
    Vector3 localDir = {std::cos(angle), 0.0f, std::sin(angle)};

    Vector3 rotatedPos = localCircle;
    radialDir = localDir;
    if (emitterCircle_.rotate.x != 0.0f || emitterCircle_.rotate.y != 0.0f ||
        emitterCircle_.rotate.z != 0.0f) {
      Matrix4x4 rotMat = Rotation(emitterCircle_.rotate);
      rotatedPos = TransformMatrix(localCircle, rotMat);
      radialDir = TransformMatrix(localDir, rotMat);
    }
    data.transform.translate = SetEffectDefinitionData_.transform.translate +
                               rotatedPos + emitterCircle_.translate;
    emitterRadialVel = emitterCircle_.radialVelocity;
  } else if (emitterType_ == EmitterType::Cone) {
    float r = emitterCone_.radius * std::sqrt(dist01(randomEngine));
    float circleAngle = dist01(randomEngine) * 2.0f * kPi;
    Vector3 localDisk = {r * std::cos(circleAngle), 0.0f,
                         r * std::sin(circleAngle)};

    float coneAngle = dist01(randomEngine) * emitterCone_.angle;
    float phi = dist01(randomEngine) * 2.0f * kPi;
    Vector3 coneDir = {std::sin(coneAngle) * std::cos(phi), std::cos(coneAngle),
                       std::sin(coneAngle) * std::sin(phi)};

    Vector3 rotatedDisk = localDisk;
    radialDir = coneDir;
    if (emitterCone_.rotate.x != 0.0f || emitterCone_.rotate.y != 0.0f ||
        emitterCone_.rotate.z != 0.0f) {
      Matrix4x4 rotMat = Rotation(emitterCone_.rotate);
      rotatedDisk = TransformMatrix(localDisk, rotMat);
      radialDir = TransformMatrix(coneDir, rotMat);
    }
    data.transform.translate = SetEffectDefinitionData_.transform.translate +
                               rotatedDisk + emitterCone_.translate;
    emitterRadialVel = emitterCone_.speed;
  }

  data.transform.scale.x +=
      distribution(randomEngine) * movementData_.sizeVariance.x;
  data.transform.scale.y +=
      distribution(randomEngine) * movementData_.sizeVariance.y;
  data.transform.scale.z +=
      distribution(randomEngine) * movementData_.sizeVariance.z;

  Vector3 baseVel = {
      movementData_.baseVelocity.x +
          distribution(randomEngine) * movementData_.velocityVariance.x,
      movementData_.baseVelocity.y +
          distribution(randomEngine) * movementData_.velocityVariance.y,
      movementData_.baseVelocity.z +
          distribution(randomEngine) * movementData_.velocityVariance.z};
  float radSpeed =
      movementData_.radialSpeed +
      distribution(randomEngine) * movementData_.radialSpeedVariance +
      emitterRadialVel;
  data.velocity = baseVel + radialDir * radSpeed;

  data.baseScale = data.transform.scale;
  data.endScale = endScale_;
  data.startColor = startColor_;
  data.endColor = endColor_;
  data.scaleCurveType = scaleCurveType_;
  data.enableColorOverLifetime = enableColorOverLifetime_;
  data.enableScaleOverLifetime = enableScaleOverLifetime_;
  data.alignToVelocity = alignToVelocity_;

  if (enableColorOverLifetime_) {
    data.color = startColor_;
  }
  if (enableScaleOverLifetime_) {
    if (scaleCurveType_ == 1) {
      data.transform.scale = {0.0f, 0.0f, 0.0f};
      if (startScale_.x > 0.0f || startScale_.y > 0.0f || startScale_.z > 0.0f) {
        data.baseScale = startScale_;
      } else if (data.baseScale.x <= 0.0f && data.baseScale.y <= 0.0f && data.baseScale.z <= 0.0f) {
        if (endScale_.x > 0.0f || endScale_.y > 0.0f || endScale_.z > 0.0f) {
          data.baseScale = endScale_;
        }
      }
    } else {
      data.transform.scale = startScale_;
      data.baseScale = startScale_;
    }
  }

  if (alignToVelocity_) {
    float lenSq = data.velocity.x * data.velocity.x +
                  data.velocity.y * data.velocity.y +
                  data.velocity.z * data.velocity.z;
    if (lenSq > 0.000001f) {
      data.transform.rotate.y = std::atan2(data.velocity.x, data.velocity.z);
      float horizDist = std::sqrt(data.velocity.x * data.velocity.x +
                                  data.velocity.z * data.velocity.z);
      data.transform.rotate.x = -std::atan2(data.velocity.y, horizDist);
    }
  }

  if (generatorBehavior) {
    generatorBehavior(data);
  }

  return data;
}

EffectDefinitionData Emitter::MakeNewParticle(Vector3 scale) {
  EffectDefinitionData data = MakeNewParticle();
  data.transform.scale = scale;
  data.baseScale = scale;
  return data;
}

EffectDefinitionData Emitter::particleMove(EffectDefinitionData p) {
  p.velocity = {0.0f, 0.0f, 0.0f};
  p.velocity.y -= 0.01f;
  return p;
}

EffectDefinitionData Emitter::particleMoveFire(EffectDefinitionData p) {
  p.velocity.y += 0.01f / 60.0f;
  float randX = ((float)rand() / RAND_MAX - 0.5f) * 0.02f / 60.0f;
  float randZ = ((float)rand() / RAND_MAX - 0.5f) * 0.02f / 60.0f;
  p.velocity.x += randX;
  p.velocity.z += randZ;
  p.transform.scale.x *= 0.999f;
  p.transform.scale.y *= 0.999f;
  p.transform.scale.z *= 0.999f;
  return p;
}

void Emitter::EmitSize() {
  if (Input::PressKey(DIK_A)) {
    emitter_.transform.scale.x -= 0.01f;
  }
  if (Input::PressKey(DIK_D)) {
    emitter_.transform.scale.x += 0.01f;
  }
  if (emitter_.transform.scale.x < 0) {
    emitter_.transform.scale.x = 0;
  }
}

bool Emitter::OnCollision(EffectDefinitionData particleData) {
  AABB aabb1 = accelerationFiled_.area;
  AABB aabb2 = GetAABB(particleData.transform, particleData.transform.scale.x,
                       particleData.transform.scale.z);
  if (aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x &&
      aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y &&
      aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z) {
    return true;
  }
  return false;
}

void Emitter::Emit() {
  uint32_t count = emitter_.count;
  switch (emitterType_) {
  case EmitterType::Sphere:
    count = emitterSphere_.count;
    break;
  case EmitterType::Circle:
    count = emitterCircle_.count;
    break;
  case EmitterType::Cone:
    count = emitterCone_.count;
    break;
  case EmitterType::Box:
  default:
    break;
  }
  if (!GetUseGpuParticle()) {
    for (uint32_t i = 0; i < count; ++i) {
      effectDefinitionData_.push_back(MakeNewParticle());
    }
  }
  manualEmitTriggered_ = true;
}
