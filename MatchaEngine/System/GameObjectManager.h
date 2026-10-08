#pragma once
#include <vector>
#include <memory>
#include "../GameObjects/Object/GameObject.h"

class GameObjectManager
{
private:
    std::vector<std::shared_ptr<GameObject>> objects_;

public:
    void AddObject(std::shared_ptr<GameObject> obj);
    void RemoveObject(std::shared_ptr<GameObject> obj);
    void CopyObject(std::shared_ptr<GameObject> obj);
    void Clear();

    void UpdateAll(Matrix4x4 view, float speedMultiplier = 1.0f);
    void DrawAll(class Draw& draw);
    void ImGuiAll();

    void SaveScene(const std::string& filepath);
    void LoadScene(const std::string& filepath);

    const std::vector<std::shared_ptr<GameObject>>& GetObjects() const { return objects_; }
};
