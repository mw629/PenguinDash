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
