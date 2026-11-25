#pragma once
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include "../../../Editer/LanguageManager.h"
#include "VariableTypes.h"
#include "../Component/Component.h"
#include "../Component/MaterialComponent.h"
#include "../Component/ColliderComponent.h"
#include <nlohmann/json.hpp>
#include "../../Math/Calculation.h"

#ifdef _USE_IMGUI
#include <imgui.h>
#endif

class GameObject
{
private:
    static inline GameObject* selectedObject_ = nullptr;

protected:
    std::string name_ = "GameObject";
    Transform transform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    bool isActive_ = true;
    bool isLocked_ = false;
    std::vector<std::shared_ptr<Component>> components_;

    // --- Dirty Flag パターン ---
    // Transformが変更された時のみフラグを立て、GetWorldMatrix()呼び出し時に遅延評価で再計算する
    mutable Matrix4x4 worldMatrix_ = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    mutable bool isWorldMatrixDirty_ = true;

    // シーングラフ（親子関係）: 親のTransform変更時に子のDirty Flagを再帰的に伝播
    GameObject* parent_ = nullptr;
    std::vector<GameObject*> children_;

public:
    /// <summary>
    /// エディタ等で現在選択されているGameObjectを取得します。
    /// </summary>
    static GameObject* GetSelectedObject() { return selectedObject_; }

    /// <summary>
    /// エディタ等で選択中のGameObjectを設定します。
    /// </summary>
    static void SetSelectedObject(GameObject* obj) { selectedObject_ = obj; }

public:
    GameObject() {}
    virtual ~GameObject() = default;

    /// <summary>
    /// 新しいコンポーネントを生成してGameObjectに追加します。
    /// </summary>
    /// <typeparam name="T">追加するコンポーネントの型</typeparam>
    /// <typeparam name="Args">コンポーネントのコンストラクタ引数</typeparam>
    /// <returns>生成されたコンポーネントのshared_ptr</returns>
    template <class T, class... Args>
    std::shared_ptr<T> AddComponent(Args&&... args) {
        std::shared_ptr<T> component = std::make_shared<T>(std::forward<Args>(args)...);
        component->SetGameObject(this);
        components_.push_back(component);
        component->Initialize();
        return component;
    }

    /// <summary>
    /// アタッチされている指定型のコンポーネントを取得します。
    /// </summary>
    /// <typeparam name="T">取得対象のコンポーネント型</typeparam>
    /// <returns>見つかったコンポーネントのshared_ptr（存在しない場合はnullptr）</returns>
    template <class T>
    std::shared_ptr<T> GetComponent() const {
        for (const auto& component : components_) {
            std::shared_ptr<T> target = std::dynamic_pointer_cast<T>(component);
            if (target) {
                return target;
            }
        }
        return nullptr;
    }

    /// <summary>
    /// 指定型のコンポーネントを削除します。
    /// </summary>
    /// <typeparam name="T">削除対象のコンポーネント型</typeparam>
    template <class T>
    void RemoveComponent() {
        components_.erase(std::remove_if(components_.begin(), components_.end(),
            [](const std::shared_ptr<Component>& component) {
                return std::dynamic_pointer_cast<T>(component) != nullptr;
            }), components_.end());
    }

    /// <summary>
    /// GameObjectおよびアタッチされている全コンポーネントを初期化します。
    /// </summary>
    virtual void Initialize() {
        for (auto& component : components_) {
            component->Initialize();
        }
    }

    /// <summary>
    /// GameObjectおよびアタッチされている全コンポーネントを更新します。
    /// </summary>
    /// <param name="view">ビュー行列</param>
    /// <param name="speedMultiplier">更新速度倍率</param>
    virtual void Update(Matrix4x4 view, float speedMultiplier = 1.0f) {
        for (auto& component : components_) {
            component->Update(view, speedMultiplier);
        }
    }

    /// <summary>
    /// 衝突検知時のコールバック関数です。
    /// </summary>
    /// <param name="other">衝突した相手のGameObject</param>
    virtual void OnCollision(GameObject* /*other*/) {}

    /// <summary>
    /// オブジェクトの型名識別文字列を取得します。
    /// </summary>
    /// <returns>型名</returns>
    virtual std::string GetTypeName() const { return "GameObject"; }

    /// <summary>
    /// 自身の複製（クローン）をディープコピーして生成します。
    /// </summary>
    /// <returns>新しいGameObjectのshared_ptr</returns>
    virtual std::shared_ptr<GameObject> Clone() const {
        auto copy = std::make_shared<GameObject>();
        copy->SetName(name_);
        copy->SetTransform(transform_);
        copy->SetIsActive(isActive_);
        copy->SetIsLocked(isLocked_);
        return copy;
    }

    /// <summary>
    /// JSONオブジェクトへオブジェクトの基本情報（名前、Transform等）をシリアライズします。
    /// </summary>
    /// <param name="out">出力先JSON</param>
    virtual void Serialize(nlohmann::json& out) const {
        out["name"] = name_;
        out["type"] = GetTypeName();
        out["isActive"] = isActive_;
        out["isLocked"] = isLocked_;
        out["transform"]["translate"] = { transform_.translate.x, transform_.translate.y, transform_.translate.z };
        out["transform"]["rotate"] = { transform_.rotate.x, transform_.rotate.y, transform_.rotate.z };
        out["transform"]["scale"] = { transform_.scale.x, transform_.scale.y, transform_.scale.z };
    }

    /// <summary>
    /// JSONオブジェクトからオブジェクトの基本情報を復元（デシリアライズ）します。
    /// </summary>
    /// <param name="in">入力元JSON</param>
    virtual void Deserialize(const nlohmann::json& in) {
        if (in.contains("name")) name_ = in["name"];
        if (in.contains("isActive")) isActive_ = in["isActive"];
        if (in.contains("isLocked")) isLocked_ = in["isLocked"];
        if (in.contains("transform")) {
            const auto& t = in["transform"];
            if (t.contains("translate") && t["translate"].size() >= 3) {
                transform_.translate = { t["translate"][0], t["translate"][1], t["translate"][2] };
            }
            if (t.contains("rotate") && t["rotate"].size() >= 3) {
                transform_.rotate = { t["rotate"][0], t["rotate"][1], t["rotate"][2] };
            }
            if (t.contains("scale") && t["scale"].size() >= 3) {
                transform_.scale = { t["scale"][0], t["scale"][1], t["scale"][2] };
            }
            SetTransform(transform_);
        }
    }

    /// <summary>
    /// アタッチされている全コンポーネントを描画します。
    /// </summary>
    /// <param name="draw">描画コマンド発行用のDrawインスタンス</param>
    virtual void Draw(class Draw& draw) {
        for (auto& component : components_) {
            component->Draw(draw);
        }
    }

    /// <summary>
    /// ImGuiを使用したインスペクターUIを描画します。
    /// </summary>
    /// <param name="drawTransform">Transformの編集UIを表示するかどうか</param>
    virtual void ImGui(bool drawTransform = true) {
#ifdef _USE_IMGUI
        if (drawTransform && ImGui::CollapsingHeader(LanguageManager::Tr("Transform"), ImGuiTreeNodeFlags_DefaultOpen)) {
            float t[3] = { transform_.translate.x, transform_.translate.y, transform_.translate.z };
            if (ImGui::DragFloat3(LanguageManager::Tr("Position"), t, 0.1f)) {
                transform_.translate = { t[0], t[1], t[2] };
                SetTransform(transform_);
            }
            float r[3] = { transform_.rotate.x, transform_.rotate.y, transform_.rotate.z };
            if (ImGui::DragFloat3(LanguageManager::Tr("Rotation"), r, 0.1f)) {
                transform_.rotate = { r[0], r[1], r[2] };
                SetTransform(transform_);
            }
            float s[3] = { transform_.scale.x, transform_.scale.y, transform_.scale.z };
            if (ImGui::DragFloat3(LanguageManager::Tr("Scale"), s, 0.1f)) {
                transform_.scale = { s[0], s[1], s[2] };
                SetTransform(transform_);
            }
        }
        bool componentsOpen = true;
        if (drawTransform) {
            componentsOpen = ImGui::CollapsingHeader(LanguageManager::Tr("Components"), ImGuiTreeNodeFlags_DefaultOpen);
        }

        if (componentsOpen) {
            for (auto& component : components_) {
                component->ImGui();
            }

            ImGuiInnerComponents();

            if (drawTransform) {
                if (ImGui::Button(LanguageManager::Tr("Add Component"))) {
                    ImGui::OpenPopup("AddComponentPopup");
                }
                if (ImGui::BeginPopup("AddComponentPopup")) {
                    if (!HasMaterial()) {
                        if (ImGui::Selectable("MaterialComponent")) {
                            AddComponent<MaterialComponent>();
                        }
                    } else {
                        ImGui::TextDisabled("MaterialComponent (Added)");
                    }
                    if (!GetComponent<ColliderComponent>()) {
                        if (ImGui::Selectable("BoxColliderComponent")) {
                            AddComponent<BoxColliderComponent>();
                        }
                        if (ImGui::Selectable("SphereColliderComponent")) {
                            AddComponent<SphereColliderComponent>();
                        }
                    } else {
                        ImGui::TextDisabled("ColliderComponent (Added)");
                    }
                    ImGui::EndPopup();
                }
            }
        }
#endif
    }

    /// <summary>
    /// 派生クラス固有の内部コンポーネント用ImGuiを描画します。
    /// </summary>
    virtual void ImGuiInnerComponents() {}

    /// <summary>
    /// オブジェクト名を取得します。
    /// </summary>
    const std::string& GetName() const { return name_; }

    /// <summary>
    /// オブジェクト名を設定します。
    /// </summary>
    void SetName(const std::string& name) { name_ = name; }

    /// <summary>
    /// ローカルTransformを取得します。
    /// </summary>
    const Transform& GetTransform() const { return transform_; }

    /// <summary>
    /// ローカルTransformを設定し、Dirtyフラグを立てます。
    /// </summary>
    /// <param name="transform">設定するTransform</param>
    virtual void SetTransform(const Transform& transform) {
        transform_ = transform;
        MarkDirty();
    }

    /// <summary>
    /// 平行移動座標を設定します。
    /// </summary>
    void SetPosition(const Vector3& pos) {
        transform_.translate = pos;
        MarkDirty();
    }

    /// <summary>
    /// 回転角（ラジアン）を設定します。
    /// </summary>
    void SetRotation(const Vector3& rot) {
        transform_.rotate = rot;
        MarkDirty();
    }

    /// <summary>
    /// スケールを設定します。
    /// </summary>
    void SetScale(const Vector3& scale) {
        transform_.scale = scale;
        MarkDirty();
    }

    /// <summary>
    /// ワールド行列の再計算フラグ（Dirty Flag）を立て、子オブジェクトにも再帰的に伝播します。
    /// </summary>
    virtual void MarkDirty() {
        isWorldMatrixDirty_ = true;
        for (auto* child : children_) {
            if (child) {
                child->MarkDirty();
            }
        }
    }

    /// <summary>
    /// ワールド行列がDirty（再計算待ち）かどうかを取得します。
    /// </summary>
    bool IsDirty() const { return isWorldMatrixDirty_; }

    /// <summary>
    /// キャッシュされたワールド行列を取得します（Dirtyな場合は自動で再計算）。
    /// </summary>
    /// <returns>ワールド行列への参照</returns>
    virtual const Matrix4x4& GetWorldMatrix() const {
        if (isWorldMatrixDirty_) {
            UpdateWorldMatrix();
        }
        return worldMatrix_;
    }

    /// <summary>
    /// ローカル姿勢と親のワールド行列を合成してワールド行列を再計算します。
    /// </summary>
    virtual void UpdateWorldMatrix() const {
        Matrix4x4 localMat = MakeAffineMatrix(transform_.translate, transform_.scale, transform_.rotate);
        if (parent_) {
            worldMatrix_ = MultiplyMatrix4x4(localMat, parent_->GetWorldMatrix());
        } else {
            worldMatrix_ = localMat;
        }
        isWorldMatrixDirty_ = false;
    }

    /// <summary>
    /// 親オブジェクトを設定します（シーングラフの親子関係）。
    /// </summary>
    /// <param name="parent">親となるGameObjectへのポインタ（親を解除する場合はnullptr）</param>
    void SetParent(GameObject* parent) {
        if (parent_ == parent) return;
        if (parent_) {
            auto& pc = parent_->children_;
            pc.erase(std::remove(pc.begin(), pc.end(), this), pc.end());
        }
        parent_ = parent;
        if (parent_) {
            parent_->children_.push_back(this);
        }
        MarkDirty();
    }

    /// <summary>
    /// 親オブジェクトを取得します。
    /// </summary>
    GameObject* GetParent() const { return parent_; }

    /// <summary>
    /// 子オブジェクトのリストを取得します。
    /// </summary>
    const std::vector<GameObject*>& GetChildren() const { return children_; }

    /// <summary>
    /// マテリアルコンポーネントがアタッチされているか判定します。
    /// </summary>
    virtual bool HasMaterial() const {
        return GetComponent<MaterialComponent>() != nullptr;
    }

    /// <summary>
    /// 半透明描画対象（アルファブレンドあり等）かどうかを判定します。
    /// </summary>
    virtual bool IsTransparent() const {
        if (auto mat = GetComponent<MaterialComponent>()) {
            BlendMode blend = mat->GetBlend();
            if (blend != BlendMode::kBlendModeNone && blend != BlendMode::kBlendModeNormal) {
                return true;
            }
            if (mat->GetColor().w < 0.999f) {
                return true;
            }
        }
        return false;
    }

    /// <summary>
    /// オブジェクトのアクティブ状態を取得します。
    /// </summary>
    bool GetIsActive() const { return isActive_; }

    /// <summary>
    /// オブジェクトのアクティブ状態を設定します。
    /// </summary>
    void SetIsActive(bool active) { isActive_ = active; }

    /// <summary>
    /// エディタでのロック状態を取得します。
    /// </summary>
    bool GetIsLocked() const { return isLocked_; }

    /// <summary>
    /// エディタでのロック状態を設定します。
    /// </summary>
    void SetIsLocked(bool locked) { isLocked_ = locked; }
};
