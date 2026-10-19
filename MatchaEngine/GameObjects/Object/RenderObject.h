#pragma once
#include "GameObject.h"
#include <memory>
#include "Graphics/Render/Draw.h"
#include "Model.h"
#include "Resource/AssetManager.h"

class RenderObject : public GameObject {
private:
    std::shared_ptr<ObjectBase> objectBase_;
    std::string modelFilePath_ = "";

public:
    RenderObject() {}
    RenderObject(std::shared_ptr<ObjectBase> obj) : objectBase_(obj) {
        if (objectBase_) {
            name_ = objectBase_->GetName();
            SetTransform(objectBase_->GetTransform());
        }
    }

    std::string GetTypeName() const override { return "RenderObject"; }

    const std::string& GetModelFilePath() const { return modelFilePath_; }
    void SetModelFilePath(const std::string& path) { modelFilePath_ = path; }

    void SetObjectBase(std::shared_ptr<ObjectBase> obj) {
        objectBase_ = obj;
        if (objectBase_) {
            name_ = objectBase_->GetName();
            SetTransform(objectBase_->GetTransform());
        }
    }

    std::shared_ptr<GameObject> Clone() const override {
        std::shared_ptr<RenderObject> copy = nullptr;
        if (!modelFilePath_.empty()) {
            size_t lastSlash = modelFilePath_.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                std::string dirPath = modelFilePath_.substr(0, lastSlash);
                std::string fileName = modelFilePath_.substr(lastSlash + 1);
                try {
                    ModelData modelData = AssetManager::LoadModel(dirPath, fileName);
                    auto model = std::make_shared<Model>();
                    model->Initialize(modelData);
                    model->SetName(fileName);
                    copy = std::make_shared<RenderObject>(model);
                    copy->SetModelFilePath(modelFilePath_);
                } catch (...) {}
            }
        }
        if (!copy) {
            copy = std::make_shared<RenderObject>();
            if (objectBase_) copy->SetObjectBase(objectBase_);
        }
        copy->SetName(name_);
        copy->SetTransform(transform_);
        copy->SetIsActive(isActive_);
        copy->SetIsLocked(isLocked_);
        return copy;
    }

    void Serialize(nlohmann::json& out) const override {
        GameObject::Serialize(out);
        out["modelFilePath"] = modelFilePath_;
    }

    void Deserialize(const nlohmann::json& in) override {
        GameObject::Deserialize(in);
        if (in.contains("modelFilePath")) {
            modelFilePath_ = in.value("modelFilePath", "");
            if (!modelFilePath_.empty() && !objectBase_) {
                size_t lastSlash = modelFilePath_.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    std::string dirPath = modelFilePath_.substr(0, lastSlash);
                    std::string fileName = modelFilePath_.substr(lastSlash + 1);
                    try {
                        ModelData modelData = AssetManager::LoadModel(dirPath, fileName);
                        auto model = std::make_shared<Model>();
                        model->Initialize(modelData);
                        model->SetName(fileName);
                        SetObjectBase(model);
                    } catch (...) {}
                }
            }
        }
    }

    void Update(Matrix4x4 view, float speedMultiplier = 1.0f) override {
        GameObject::Update(view, speedMultiplier);
        if (objectBase_) {
            if (objectBase_->GetName() == "SkyBox" || objectBase_->GetShader() == SkyBoxShader) {
                SetTransform(objectBase_->GetTransform());
            } else {
                objectBase_->SetTransform(transform_);
            }
            objectBase_->SettingWvp(view);
        }
    }

    void Draw(class Draw& draw) override {
        GameObject::Draw(draw);
        if (objectBase_) {
            objectBase_->Render(draw);
        }
    }

    void SetTransform(const Transform& transform) override {
        GameObject::SetTransform(transform);
        if (objectBase_) {
            objectBase_->SetTransform(transform_);
        }
    }

    void SetFrustumCullingEnabled(bool enable) {
        if (objectBase_) {
            objectBase_->SetFrustumCullingEnabled(enable);
        }
    }
    bool IsFrustumCullingEnabled() const {
        return objectBase_ ? objectBase_->IsFrustumCullingEnabled() : false;
    }

    void ImGuiInnerComponents() override {
        if (objectBase_) {
            objectBase_->ImGui(false);
        }
    }

    bool HasMaterial() const override {
        return objectBase_ && objectBase_->GetComponent<MaterialComponent>() != nullptr || GameObject::HasMaterial();
    }

    bool IsTransparent() const override {
        if (objectBase_) {
            BlendMode blend = objectBase_->GetBlend();
            if (blend != BlendMode::kBlendModeNone && blend != BlendMode::kBlendModeNormal) {
                return true;
            }
            if (auto mat = objectBase_->GetComponent<MaterialComponent>()) {
                if (mat->GetColor().w < 0.999f) return true;
            }
        }
        return GameObject::IsTransparent();
    }

    void SetColor(const Vector4& color) {
        if (objectBase_) {
            if (auto model = dynamic_cast<Model*>(objectBase_.get())) {
                model->SetColor(color);
            }
        }
    }

    std::shared_ptr<ObjectBase> GetObjectBase() const { return objectBase_; }
};
