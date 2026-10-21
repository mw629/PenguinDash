#pragma once
#include"ObjectBase.h"


struct ModelSubMeshMaterial {
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU{};
	std::unique_ptr<MaterialFactory> materialFactory;
};

class Model	:public ObjectBase
{
private:
	static constexpr int kInvalidTextureIndex = -1;
	static constexpr float kBillboardBoundingSphereScale = 1.5f;

	int modelNumber_;

	//アニメーション
	Node rootNode_;

	std::vector<ModelSubMeshMaterial> subMeshMaterials_;
	bool isBillboard_ = false;

public:
	/// <summary>
	/// デストラクタ
	/// </summary>
	~Model() override;

	/// <summary>
	/// モデルデータの初期化
	/// </summary>
	/// <param name="modelData">読み込んだ3Dモデルデータ</param>
	void Initialize(ModelData modelData);

	/// <summary>
	/// WVP定数バッファの更新
	/// </summary>
	/// <param name="viewMatrix">ビュー行列</param>
	void SettingWvp(Matrix4x4 viewMatrix) override;

	/// <summary>
	/// カスタムプロジェクション行列を用いたWVP更新
	/// </summary>
	/// <param name="viewMatrix">ビュー行列</param>
	/// <param name="customProjection">プロジェクション行列</param>
	void SettingWvp(Matrix4x4 viewMatrix, const Matrix4x4* customProjection);
	
	/// <summary>
	/// モデルのメッシュ・マテリアルリソース生成
	/// </summary>
	void CreateObject() override;

	/// <summary>
	/// モデルの描画実行
	/// </summary>
	/// <param name="draw">描画マネージャ</param>
	void Render(class Draw& draw) override;

	void SetBillboard(bool isBillboard) { isBillboard_ = isBillboard; }
	bool GetBillboard() const { return isBillboard_; }

	void SetLighting(bool isActive) {
		ObjectBase::SetLighting(isActive);
		for (auto& mat : subMeshMaterials_) {
			if (mat.materialFactory) {
				mat.materialFactory->SetMaterialLighting(isActive);
			}
		}
	}

	void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU) {
		ObjectBase::SetTexture(textureSrvHandleGPU);
		for (auto& mat : subMeshMaterials_) {
			mat.textureSrvHandleGPU = textureSrvHandleGPU;
		}
	}

	void SetColor(Vector4 color) {
		if (auto matComp = GetComponent<MaterialComponent>()) {
			if (matComp->GetMaterialFactory()) {
				matComp->GetMaterialFactory()->SetColor(color);
			}
		}
		for (auto& mat : subMeshMaterials_) {
			if (mat.materialFactory) {
				mat.materialFactory->SetColor(color);
			}
		}
	}

	Mesh GetMesh() override;

	int GetModelNumber() { return modelNumber_; }

	const std::vector<ModelSubMeshMaterial>& GetSubMeshMaterials() const { return subMeshMaterials_; }

	AABB GetWorldAABB() const;
	BoundingSphere GetWorldBoundingSphere() const;
};

