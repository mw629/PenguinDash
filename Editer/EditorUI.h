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

    /// <summary>
    /// マウスピッキングによるオブジェクト選択処理
    /// </summary>
    void ProcessMousePicking(GameObjectManager* gameObjectManager, const Matrix4x4& view, const Matrix4x4& projection);

    /// <summary>
    /// 選択オブジェクトに対するトランスフォームギズモの描画・操作処理
    /// </summary>
    void DrawGizmo(const Matrix4x4& view, const Matrix4x4& projection);

public:
    /// <summary>
    /// エディタUIおよびギズモの描画・更新
    /// </summary>
    /// <param name="gameObjectManager">対象オブジェクトマネージャー</param>
    /// <param name="view">ビュー行列</param>
    /// <param name="projection">プロジェクション行列</param>
    void Draw(GameObjectManager* gameObjectManager, const Matrix4x4& view, const Matrix4x4& projection);
    
    /// <summary>
    /// Sceneウィンドウ内から呼び出すギズモ描画
    /// </summary>
    /// <param name="view">ビュー行列</param>
    /// <param name="projection">プロジェクション行列</param>
    void DrawGizmoInScene(const Matrix4x4& view, const Matrix4x4& projection) { DrawGizmo(view, projection); }
    
    /// <summary>
    /// 現在選択されているオブジェクトの取得
    /// </summary>
    /// <returns>選択中オブジェクトのポインタ</returns>
    std::shared_ptr<GameObject> GetSelectedObject() const { return selectedObject_; }

    /// <summary>
    /// 選択オブジェクトの設定
    /// </summary>
    /// <param name="obj">選択するオブジェクト</param>
    void SetSelectedObject(std::shared_ptr<GameObject> obj) {
        selectedObject_ = obj;
        GameObject::SetSelectedObject(obj.get());
    }
};
