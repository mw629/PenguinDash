#pragma once
#include <memory>
#include "System/GameObjectManager.h"
#include "Calculation.h"
#include "../MatchaEngine/Core/VariableTypes/Matrix4x4.h"

#include "../MatchaEngine/Core/WindowConfig.h"

class EditorUI {
private:
    std::shared_ptr<GameObject> selectedObject_ = nullptr;
    
    // ウィンドウサイズ (デフォルト解像度)
    float windowWidth_ = static_cast<float>(WindowConfig::kDefaultClientWidth);
    float windowHeight_ = static_cast<float>(WindowConfig::kDefaultClientHeight);

    void ProcessMousePicking(GameObjectManager* gameObjectManager, const Matrix4x4& view, const Matrix4x4& projection);
    void DrawGizmo(const Matrix4x4& view, const Matrix4x4& projection);

public:
    void Draw(GameObjectManager* gameObjectManager, const Matrix4x4& view, const Matrix4x4& projection);
    
    // SceneウィンドウのBegin/Endの間から呼ぶギズモ描画関数
    void DrawGizmoInScene(const Matrix4x4& view, const Matrix4x4& projection) { DrawGizmo(view, projection); }
    
    std::shared_ptr<GameObject> GetSelectedObject() const { return selectedObject_; }
    void SetSelectedObject(std::shared_ptr<GameObject> obj) {
        selectedObject_ = obj;
        GameObject::selectedObject_ = obj.get();
    }
};
