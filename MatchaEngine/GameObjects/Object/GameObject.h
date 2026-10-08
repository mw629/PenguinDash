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

#ifdef _USE_IMGUI
#include <imgui.h>
#endif

class GameObject
{
public:
    std::string name_ = "GameObject";
    static inline GameObject* selectedObject_ = nullptr;
protected:
    Transform transform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    bool isActive_ = true;
    bool isLocked_ = false;
    std::vector<std::shared_ptr<Component>> components_;

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

    virtual std::shared_ptr<GameObject> Clone() {
        return nullptr; // 基底クラスではnullptr。必要に応じて派生クラスで実装
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
                        if (ImGui::Selectable("ColliderComponent")) {
                            AddComponent<ColliderComponent>();
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
    virtual void SetTransform(const Transform& transform) { transform_ = transform; }

    virtual bool HasMaterial() const {
        return GetComponent<MaterialComponent>() != nullptr;
    }

    bool GetIsActive() const { return isActive_; }
    void SetIsActive(bool active) { isActive_ = active; }

    bool GetIsLocked() const { return isLocked_; }
    void SetIsLocked(bool locked) { isLocked_ = locked; }
};
