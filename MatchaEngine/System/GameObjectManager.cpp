#include "GameObjectManager.h"
#include <algorithm>
#include <imgui.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include "GameObjectFactory.h"
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
    auto copy = obj->Clone();
    if (copy) {
        copy->SetName(obj->GetName() + " (Copy)");
        objects_.push_back(copy);
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

    std::vector<std::shared_ptr<GameObject>> opaqueObjects;
    std::vector<std::pair<float, std::shared_ptr<GameObject>>> transparentObjects;

    Vector3 camPos = { 0.0f, 0.0f, 0.0f };
    if (draw.GetCamera()) {
        camPos = draw.GetCamera()->GetTransform().translate;
    }

    for (auto& obj : objects_) {
        if (!obj || !obj->GetIsActive()) continue;

        if (obj->IsTransparent()) {
            Vector3 objPos = obj->GetTransform().translate;
            float dx = objPos.x - camPos.x;
            float dy = objPos.y - camPos.y;
            float dz = objPos.z - camPos.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            transparentObjects.push_back({ distSq, obj });
        } else {
            opaqueObjects.push_back(obj);
        }
    }

    // 1. 不透明オブジェクトを先に描画（深度バッファに書き込まれ、背景が確立される）
    for (auto& obj : opaqueObjects) {
        obj->Draw(draw);
    }

    // 2. 半透明オブジェクトをカメラから遠い順（降順: Back-to-Front）にソートして描画
    std::sort(transparentObjects.begin(), transparentObjects.end(),
        [](const auto& a, const auto& b) {
            return a.first > b.first;
        });

    for (auto& pair : transparentObjects) {
        pair.second->Draw(draw);
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
        obj->Serialize(objJson);
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
            std::string name = objJson.value("name", "");
            std::string type = objJson.value("type", "");
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
                targetObj = GameObjectFactory::Create(type);
                if (targetObj) {
                    targetObj->SetName(name);
                    objects_.push_back(targetObj);
                    usedObjects.push_back(targetObj);
                }
            }

            if (targetObj) {
                targetObj->Deserialize(objJson);
            }
        }
    }
}
