#include "GameObjectManager.h"
#include <algorithm>
#include <imgui.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include "../GameObjects/Object/RenderObject.h"
#include "Resource/AssetManager.h"
#include "../GameObjects/Light/DirectionalLight.h"
#include "../GameObjects/Light/PointLight.h"
#include "../GameObjects/Light/SpotLight.h"
#include "Graphics/Render/Draw.h"

void GameObjectManager::AddObject(std::shared_ptr<GameObject> obj)
{
    if (obj) {
        objects_.push_back(obj);
    }
}

void GameObjectManager::RemoveObject(std::shared_ptr<GameObject> obj)
{
    auto it = std::remove(objects_.begin(), objects_.end(), obj);
    if (it != objects_.end()) {
        objects_.erase(it, objects_.end());
    }
}

void GameObjectManager::CopyObject(std::shared_ptr<GameObject> obj)
{
    if (!obj) return;

    nlohmann::json objJson;
    objJson["name"] = obj->GetName() + " (Copy)";
    objJson["isActive"] = obj->GetIsActive();
    objJson["isLocked"] = obj->GetIsLocked();
    
    const Transform& t = obj->GetTransform();
    objJson["transform"]["translate"] = { t.translate.x, t.translate.y, t.translate.z };
    objJson["transform"]["rotate"] = { t.rotate.x, t.rotate.y, t.rotate.z };
    objJson["transform"]["scale"] = { t.scale.x, t.scale.y, t.scale.z };
    
    std::string type = "";
    if (auto renderObj = std::dynamic_pointer_cast<RenderObject>(obj)) {
        type = "RenderObject";
        objJson["type"] = "RenderObject";
        objJson["modelFilePath"] = renderObj->modelFilePath_;
    }
    else if (auto dirLight = std::dynamic_pointer_cast<DirectionalLight>(obj)) {
        type = "DirectionalLight";
        objJson["type"] = "DirectionalLight";
        objJson["lightColor"] = { dirLight->color_.x, dirLight->color_.y, dirLight->color_.z, dirLight->color_.w };
        objJson["intensity"] = dirLight->intensity_;
    }
    else if (auto pointLight = std::dynamic_pointer_cast<PointLight>(obj)) {
        type = "PointLight";
        objJson["type"] = "PointLight";
        objJson["lightColor"] = { pointLight->color_.x, pointLight->color_.y, pointLight->color_.z, pointLight->color_.w };
        objJson["intensity"] = pointLight->intensity_;
        objJson["radius"] = pointLight->radius_;
        objJson["decay"] = pointLight->decay_;
    }
    else if (auto spotLight = std::dynamic_pointer_cast<SpotLight>(obj)) {
        type = "SpotLight";
        objJson["type"] = "SpotLight";
        objJson["lightColor"] = { spotLight->color_.x, spotLight->color_.y, spotLight->color_.z, spotLight->color_.w };
        objJson["intensity"] = spotLight->intensity_;
        objJson["distance"] = spotLight->distance_;
        objJson["decay"] = spotLight->decay_;
        objJson["cosAngle"] = spotLight->cosAngle_;
        objJson["cosFalloffStart"] = spotLight->cosFalloffStart_;
    }

    std::shared_ptr<GameObject> targetObj = nullptr;

    if (type == "RenderObject") {
        std::string filePath = objJson.value("modelFilePath", "");
        size_t lastSlash = filePath.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            std::string dirPath = filePath.substr(0, lastSlash);
            std::string fileName = filePath.substr(lastSlash + 1);
            try {
                ModelData modelData = AssetManager::LoadModel(dirPath, fileName);
                auto model = std::make_shared<Model>();
                model->Initialize(modelData);
                model->name_ = fileName;
                
                auto renderObj = std::make_shared<RenderObject>(model);
                renderObj->SetName(objJson["name"]);
                renderObj->modelFilePath_ = filePath;
                targetObj = renderObj;
                objects_.push_back(targetObj);
            } catch (...) {}
        }
    } else if (type == "DirectionalLight") {
        auto light = std::make_shared<DirectionalLight>();
        light->color_ = { objJson["lightColor"][0], objJson["lightColor"][1], objJson["lightColor"][2], objJson["lightColor"][3] };
        light->intensity_ = objJson["intensity"];
        targetObj = light;
        targetObj->SetName(objJson["name"]);
        objects_.push_back(targetObj);
    } else if (type == "PointLight") {
        auto light = std::make_shared<PointLight>();
        light->color_ = { objJson["lightColor"][0], objJson["lightColor"][1], objJson["lightColor"][2], objJson["lightColor"][3] };
        light->intensity_ = objJson["intensity"];
        light->radius_ = objJson["radius"];
        light->decay_ = objJson["decay"];
        targetObj = light;
        targetObj->SetName(objJson["name"]);
        objects_.push_back(targetObj);
    } else if (type == "SpotLight") {
        auto light = std::make_shared<SpotLight>();
        light->color_ = { objJson["lightColor"][0], objJson["lightColor"][1], objJson["lightColor"][2], objJson["lightColor"][3] };
        light->intensity_ = objJson["intensity"];
        light->distance_ = objJson["distance"];
        light->decay_ = objJson["decay"];
        light->cosAngle_ = objJson["cosAngle"];
        light->cosFalloffStart_ = objJson["cosFalloffStart"];
        targetObj = light;
        targetObj->SetName(objJson["name"]);
        objects_.push_back(targetObj);
    } else {
        targetObj = std::make_shared<GameObject>();
        targetObj->SetName(objJson["name"]);
        objects_.push_back(targetObj);
    }

    if (targetObj) {
        targetObj->SetIsActive(objJson["isActive"]);
        targetObj->SetIsLocked(objJson["isLocked"]);
        
        Transform t2;
        t2.translate.x = objJson["transform"]["translate"][0];
        t2.translate.y = objJson["transform"]["translate"][1];
        t2.translate.z = objJson["transform"]["translate"][2];
        
        t2.rotate.x = objJson["transform"]["rotate"][0];
        t2.rotate.y = objJson["transform"]["rotate"][1];
        t2.rotate.z = objJson["transform"]["rotate"][2];
        
        t2.scale.x = objJson["transform"]["scale"][0];
        t2.scale.y = objJson["transform"]["scale"][1];
        t2.scale.z = objJson["transform"]["scale"][2];
        
        targetObj->SetTransform(t2);
    }
}

void GameObjectManager::Clear()
{
    objects_.clear();
}

void GameObjectManager::UpdateAll(Matrix4x4 view, float speedMultiplier)
{
    for (auto& obj : objects_) {
        if (obj && obj->GetIsActive()) {
            obj->Update(view, speedMultiplier);
        }
    }
}

void GameObjectManager::DrawAll(class Draw& draw)
{
    if (draw.GetLightManager()) {
        draw.GetLightManager()->UpdateLights(objects_);
    }

    for (auto& obj : objects_) {
        if (obj && obj->GetIsActive()) {
            obj->Draw(draw);
        }
    }
}

void GameObjectManager::ImGuiAll()
{
#ifdef _USE_IMGUI
    if (ImGui::TreeNode("GameObjectManager")) {
        for (auto& obj : objects_) {
            if (obj) {
                if (ImGui::TreeNode(obj->GetName().c_str())) {
                    obj->ImGui();
                    ImGui::TreePop();
                }
            }
        }
        ImGui::TreePop();
    }
#endif
}

void GameObjectManager::SaveScene(const std::string& filepath)
{
    nlohmann::json root;
    nlohmann::json objectsArray = nlohmann::json::array();

    for (const auto& obj : objects_) {
        if (!obj) continue;
        nlohmann::json objJson;
        objJson["name"] = obj->GetName();
        objJson["isActive"] = obj->GetIsActive();
        objJson["isLocked"] = obj->GetIsLocked();
        
        const Transform& t = obj->GetTransform();
        objJson["transform"]["translate"] = { t.translate.x, t.translate.y, t.translate.z };
        objJson["transform"]["rotate"] = { t.rotate.x, t.rotate.y, t.rotate.z };
        objJson["transform"]["scale"] = { t.scale.x, t.scale.y, t.scale.z };
        
        if (auto renderObj = std::dynamic_pointer_cast<RenderObject>(obj)) {
            objJson["type"] = "RenderObject";
            objJson["modelFilePath"] = renderObj->modelFilePath_;
        }
        else if (auto dirLight = std::dynamic_pointer_cast<DirectionalLight>(obj)) {
            objJson["type"] = "DirectionalLight";
            objJson["lightColor"] = { dirLight->color_.x, dirLight->color_.y, dirLight->color_.z, dirLight->color_.w };
            objJson["intensity"] = dirLight->intensity_;
        }
        else if (auto pointLight = std::dynamic_pointer_cast<PointLight>(obj)) {
            objJson["type"] = "PointLight";
            objJson["lightColor"] = { pointLight->color_.x, pointLight->color_.y, pointLight->color_.z, pointLight->color_.w };
            objJson["intensity"] = pointLight->intensity_;
            objJson["radius"] = pointLight->radius_;
            objJson["decay"] = pointLight->decay_;
        }
        else if (auto spotLight = std::dynamic_pointer_cast<SpotLight>(obj)) {
            objJson["type"] = "SpotLight";
            objJson["lightColor"] = { spotLight->color_.x, spotLight->color_.y, spotLight->color_.z, spotLight->color_.w };
            objJson["intensity"] = spotLight->intensity_;
            objJson["distance"] = spotLight->distance_;
            objJson["decay"] = spotLight->decay_;
            objJson["cosAngle"] = spotLight->cosAngle_;
            objJson["cosFalloffStart"] = spotLight->cosFalloffStart_;
        }
        
        objectsArray.push_back(objJson);
    }
    
    root["objects"] = objectsArray;

    std::ofstream file(filepath);
    if (file.is_open()) {
        file << root.dump(4);
    }
}

void GameObjectManager::LoadScene(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) return;

    nlohmann::json root;
    file >> root;

    if (root.contains("objects")) {
        std::vector<std::shared_ptr<GameObject>> usedObjects;

        for (const auto& objJson : root["objects"]) {
            std::string name = objJson["name"];
            std::string type = objJson.contains("type") ? objJson["type"].get<std::string>() : "";
            std::shared_ptr<GameObject> targetObj = nullptr;

            for (auto& obj : objects_) {
                if (obj && obj->GetName() == name) {
                    if (std::find(usedObjects.begin(), usedObjects.end(), obj) == usedObjects.end()) {
                        targetObj = obj;
                        usedObjects.push_back(obj);
                        break;
                    }
                }
            }

            if (!targetObj) {
                if (type == "RenderObject") {
                    std::string filePath = objJson.value("modelFilePath", "");
                    size_t lastSlash = filePath.find_last_of("/\\");
                    if (lastSlash != std::string::npos) {
                        std::string dirPath = filePath.substr(0, lastSlash);
                        std::string fileName = filePath.substr(lastSlash + 1);
                        try {
                            ModelData modelData = AssetManager::LoadModel(dirPath, fileName);
                            auto model = std::make_shared<Model>();
                            model->Initialize(modelData);
                            model->name_ = fileName;
                            
                            auto renderObj = std::make_shared<RenderObject>(model);
                            renderObj->SetName(name);
                            renderObj->modelFilePath_ = filePath;
                            targetObj = renderObj;
                            objects_.push_back(targetObj);
                            usedObjects.push_back(targetObj);
                        } catch (...) {}
                    }
                }
                else if (type == "DirectionalLight") {
                    auto light = std::make_shared<DirectionalLight>();
                    light->SetName(name);
                    if (objJson.contains("lightColor")) {
                        light->color_ = { objJson["lightColor"][0], objJson["lightColor"][1], objJson["lightColor"][2], objJson["lightColor"][3] };
                    }
                    if (objJson.contains("intensity")) light->intensity_ = objJson["intensity"];
                    targetObj = light;
                    objects_.push_back(targetObj);
                    usedObjects.push_back(targetObj);
                }
                else if (type == "PointLight") {
                    auto light = std::make_shared<PointLight>();
                    light->SetName(name);
                    if (objJson.contains("lightColor")) {
                        light->color_ = { objJson["lightColor"][0], objJson["lightColor"][1], objJson["lightColor"][2], objJson["lightColor"][3] };
                    }
                    if (objJson.contains("intensity")) light->intensity_ = objJson["intensity"];
                    if (objJson.contains("radius")) light->radius_ = objJson["radius"];
                    if (objJson.contains("decay")) light->decay_ = objJson["decay"];
                    targetObj = light;
                    objects_.push_back(targetObj);
                    usedObjects.push_back(targetObj);
                }
                else if (type == "SpotLight") {
                    auto light = std::make_shared<SpotLight>();
                    light->SetName(name);
                    if (objJson.contains("lightColor")) {
                        light->color_ = { objJson["lightColor"][0], objJson["lightColor"][1], objJson["lightColor"][2], objJson["lightColor"][3] };
                    }
                    if (objJson.contains("intensity")) light->intensity_ = objJson["intensity"];
                    if (objJson.contains("distance")) light->distance_ = objJson["distance"];
                    if (objJson.contains("decay")) light->decay_ = objJson["decay"];
                    if (objJson.contains("cosAngle")) light->cosAngle_ = objJson["cosAngle"];
                    if (objJson.contains("cosFalloffStart")) light->cosFalloffStart_ = objJson["cosFalloffStart"];
                    targetObj = light;
                    objects_.push_back(targetObj);
                    usedObjects.push_back(targetObj);
                }
            }

            if (targetObj) {
                targetObj->SetIsActive(objJson["isActive"]);
                if (objJson.contains("isLocked")) targetObj->SetIsLocked(objJson["isLocked"]);
                
                if (objJson.contains("transform")) {
                    Transform t;
                    t.translate.x = objJson["transform"]["translate"][0];
                    t.translate.y = objJson["transform"]["translate"][1];
                    t.translate.z = objJson["transform"]["translate"][2];
                    
                    t.rotate.x = objJson["transform"]["rotate"][0];
                    t.rotate.y = objJson["transform"]["rotate"][1];
                    t.rotate.z = objJson["transform"]["rotate"][2];
                    
                    t.scale.x = objJson["transform"]["scale"][0];
                    t.scale.y = objJson["transform"]["scale"][1];
                    t.scale.z = objJson["transform"]["scale"][2];
                    
                    targetObj->SetTransform(t);
                }
            }
        }
    }
}
