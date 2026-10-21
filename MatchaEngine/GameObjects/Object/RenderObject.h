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

    /// <summary>
    /// 型名識別文字列を取得します。
    /// </summary>
    /// <returns>"RenderObject"</returns>
    std::string GetTypeName() const override { return "RenderObject"; }

    /// <summary>
    /// 読み込み元のモデルファイルパスを取得します。
    /// </summary>
    const std::string& GetModelFilePath() const { return modelFilePath_; }

    /// <summary>
    /// 読み込み元のモデルファイルパスを設定します。
    /// </summary>
    void SetModelFilePath(const std::string& path) { modelFilePath_ = path; }

    /// <summary>
    /// 描画対象となるObjectBaseインスタンスを設定します。
    /// </summary>
    /// <param name="obj">ObjectBaseのshared_ptr</param>
    void SetObjectBase(std::shared_ptr<ObjectBase> obj) {
        objectBase_ = obj;
        if (objectBase_) {
            name_ = objectBase_->GetName();
            SetTransform(objectBase_->GetTransform());
        }
    }

    /// <summary>
    /// 自身のクローン（複製）をディープコピーして生成します。
    /// </summary>
    /// <returns>新しいRenderObjectのshared_ptr</returns>
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

    /// <summary>
    /// JSONオブジェクトへレンダリング設定（モデルパス等）をシリアライズします。
    /// </summary>
    /// <param name="out">出力先JSON</param>
    void Serialize(nlohmann::json& out) const override {
        GameObject::Serialize(out);
        out["modelFilePath"] = modelFilePath_;
    }

    /// <summary>
    /// JSONオブジェクトからレンダリング設定をデシリアライズしてモデルを復元します。
    /// </summary>
    /// <param name="in">入力元JSON</param>
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

    /// <summary>
    /// オブジェクトおよび内包するモデルのTransformやWVP行列を更新します。
    /// </summary>
    /// <param name="view">ビュー行列</param>
    /// <param name="speedMultiplier">更新速度倍率</param>
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

    /// <summary>
    /// 内包する描画可能モデルの描画コマンドを発行します。
    /// </summary>
    /// <param name="draw">Drawインスタンス</param>
    void Draw(class Draw& draw) override {
        GameObject::Draw(draw);
        if (objectBase_) {
            objectBase_->Render(draw);
        }
    }

    /// <summary>
    /// Transformを設定し、内包モデルにも反映します。
    /// </summary>
    /// <param name="transform">設定する姿勢</param>
    void SetTransform(const Transform& transform) override {
        GameObject::SetTransform(transform);
        if (objectBase_) {
            objectBase_->SetTransform(transform_);
        }
    }

    /// <summary>
    /// 視錐台カリングの有効/無効を設定します。
    /// </summary>
    /// <param name="enable">trueならカリング有効</param>
    void SetFrustumCullingEnabled(bool enable) {
        if (objectBase_) {
            objectBase_->SetFrustumCullingEnabled(enable);
        }
    }

    /// <summary>
    /// 視錐台カリングが有効かどうかを取得します。
    /// </summary>
    bool IsFrustumCullingEnabled() const {
        return objectBase_ ? objectBase_->IsFrustumCullingEnabled() : false;
    }

    /// <summary>
    /// 内包モデルのImGuiデバッグUIを描画します。
    /// </summary>
    void ImGuiInnerComponents() override {
        if (objectBase_) {
            objectBase_->ImGui(false);
        }
    }

    /// <summary>
    /// マテリアルコンポーネントを保持しているかを判定します。
    /// </summary>
    bool HasMaterial() const override {
        return objectBase_ && objectBase_->GetComponent<MaterialComponent>() != nullptr || GameObject::HasMaterial();
    }

    /// <summary>
    /// 半透明描画対象（アルファブレンドあり等）かどうかを判定します。
    /// </summary>
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

    /// <summary>
    /// 内包モデルの描画色・カラーティントを設定します。
    /// </summary>
    /// <param name="color">乗算カラー (RGBA)</param>
    void SetColor(const Vector4& color) {
        if (objectBase_) {
            if (auto model = dynamic_cast<Model*>(objectBase_.get())) {
                model->SetColor(color);
            }
        }
    }

    /// <summary>
    /// 内包するObjectBaseインスタンスを取得します。
    /// </summary>
    std::shared_ptr<ObjectBase> GetObjectBase() const { return objectBase_; }
};
