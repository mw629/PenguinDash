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
    static GameObject* GetSelectedObject() { return selectedObject_; }
    static void SetSelectedObject(GameObject* obj) { selectedObject_ = obj; }

public:
    GameObject() {}
    virtual ~GameObject() = default;

    template <class T, class... Args>
    std::shared_ptr<T> AddComponent(Args&&... args) {
        std::shared_ptr<T> component = std::make_shared<T>(std::forward<Args>(args)...);
        component->SetGameObject(this);
        components_.push_back(component);
        component->Initialize();
        return component;
    }

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

    template <class T>
    void RemoveComponent() {
        components_.erase(std::remove_if(components_.begin(), components_.end(),
            [](const std::shared_ptr<Component>& component) {
                return std::dynamic_pointer_cast<T>(component) != nullptr;
            }), components_.end());
    }

    virtual void Initialize() {
        // コンポーネントはAddComponentで初期化されるためここでは呼ばないケースもあるが、
        // GameObject全体の初期化時にコンポーネントも初期化する。
        for (auto& component : components_) {
            component->Initialize();
        }
    }
    virtual void Update(Matrix4x4 view, float speedMultiplier = 1.0f) {
        for (auto& component : components_) {
            component->Update(view, speedMultiplier);
        }
    }
    virtual void OnCollision(GameObject* /*other*/) {}

    virtual std::string GetTypeName() const { return "GameObject"; }

    virtual std::shared_ptr<GameObject> Clone() const {
        auto copy = std::make_shared<GameObject>();
        copy->SetName(name_);
        copy->SetTransform(transform_);
        copy->SetIsActive(isActive_);
        copy->SetIsLocked(isLocked_);
        return copy;
    }

    virtual void Serialize(nlohmann::json& out) const {
        out["name"] = name_;
        out["type"] = GetTypeName();
        out["isActive"] = isActive_;
        out["isLocked"] = isLocked_;
        out["transform"]["translate"] = { transform_.translate.x, transform_.translate.y, transform_.translate.z };
        out["transform"]["rotate"] = { transform_.rotate.x, transform_.rotate.y, transform_.rotate.z };
        out["transform"]["scale"] = { transform_.scale.x, transform_.scale.y, transform_.scale.z };
    }

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

    virtual void Draw(class Draw& draw) {
        for (auto& component : components_) {
            component->Draw(draw);
        }
    }
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

    virtual void ImGuiInnerComponents() {}

    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }

    const Transform& GetTransform() const { return transform_; }
    virtual void SetTransform(const Transform& transform) {
        transform_ = transform;
        MarkDirty();
    }

    void SetPosition(const Vector3& pos) {
        transform_.translate = pos;
        MarkDirty();
    }
    void SetRotation(const Vector3& rot) {
        transform_.rotate = rot;
        MarkDirty();
    }
    void SetScale(const Vector3& scale) {
        transform_.scale = scale;
        MarkDirty();
    }

    /// <summary>
    /// Dirty Flag: ワールド行列の再計算フラグを立てる。
    /// 親オブジェクトのTransformが変更された場合、すべての子オブジェクトにも再帰的にフラグを伝播します。
    /// </summary>
    virtual void MarkDirty() {
        isWorldMatrixDirty_ = true;
        for (auto* child : children_) {
            if (child) {
                child->MarkDirty();
            }
        }
    }

    bool IsDirty() const { return isWorldMatrixDirty_; }

    /// <summary>
    /// Dirty Flag: キャッシュされたワールド行列を取得。
    /// isWorldMatrixDirty_ が true の場合のみ再計算を行い、静止時の無駄な行列再計算をスキップします。
    /// </summary>
    virtual const Matrix4x4& GetWorldMatrix() const {
        if (isWorldMatrixDirty_) {
            UpdateWorldMatrix();
        }
        return worldMatrix_;
    }

    virtual void UpdateWorldMatrix() const {
        Matrix4x4 localMat = MakeAffineMatrix(transform_.translate, transform_.scale, transform_.rotate);
        if (parent_) {
            worldMatrix_ = MultiplyMatrix4x4(localMat, parent_->GetWorldMatrix());
        } else {
            worldMatrix_ = localMat;
        }
        isWorldMatrixDirty_ = false;
    }

    // シーングラフ（親子関係）
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
    GameObject* GetParent() const { return parent_; }
    const std::vector<GameObject*>& GetChildren() const { return children_; }

    virtual bool HasMaterial() const {
        return GetComponent<MaterialComponent>() != nullptr;
    }

    /// <summary>
    /// 半透明オブジェクトとして描画順を後回し（奥から手前へソート）にするかどうか
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

    bool GetIsActive() const { return isActive_; }
    void SetIsActive(bool active) { isActive_ = active; }

    bool GetIsLocked() const { return isLocked_; }
    void SetIsLocked(bool locked) { isLocked_ = locked; }
};
