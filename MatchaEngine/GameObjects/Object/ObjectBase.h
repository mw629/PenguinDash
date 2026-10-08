#pragma once
#include <wrl.h>
#include <d3dx12.h>
#include "VariableTypes.h"
#include "Texture.h"
#include "MaterialFactory.h"
#include "PipelineState.h"
#include "GameObject.h"
#include "../Component/MaterialComponent.h"

class ObjectBase : public GameObject
{
protected:
	// 派生クラスからアクセス可能な静的メンバー
	static float kClientWidth_;
	static float kClientHeight_;

	//objectResource

	//マテリアルデータ
	std::unique_ptr<Texture> texture = std::make_unique<Texture>();
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

	//頂点データ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	VertexData* vertexData_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpDataResource_[2];
	TransformationMatrix* wvpData_[2] = { nullptr, nullptr };
	int vertexSize_ = 0;

	static int s_wvpIndex;

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	uint32_t* indexData_ = nullptr;
	int indexSize_;

	bool isInstancing_ = false;
	std::vector<Transform> instancingTransforms_;
	int maxInstanceCount_ = 1000;

public:
	virtual ~ObjectBase();

	void SetMaxInstanceCount(int count) { maxInstanceCount_ = count; }

	static void SetObjectResource(Vector2 ClientSize);
	static void SetWvpIndex(int index) { s_wvpIndex = index; }

	virtual void CreateVertexData();
	virtual void CreateWVP();
	virtual void CreateIndexResource();

	virtual void CreateObject();

	virtual void SettingWvp(Matrix4x4 viewMatrix);


	void SetTransform(Transform transform) { transform_ = transform; }  // バグ修正: transform_ = transform_ → transform_ = transform
	void SetLighting(bool isActive) { 
		auto matComp = GetComponent<MaterialComponent>();
		if(matComp) matComp->GetMaterialFactory()->SetMaterialLighting(isActive);
	}
	void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU) { textureSrvHandleGPU_ = textureSrvHandleGPU; }

	void SetInstancing(bool isInstancing) { isInstancing_ = isInstancing; }
	void AddInstanceTransform(Transform transform) { instancingTransforms_.push_back(transform); }
	void ClearInstanceTransforms() { instancingTransforms_.clear(); }
	int GetInstanceCount() { return isInstancing_ ? static_cast<int>(instancingTransforms_.size()) : 1; }

	//getter
	Transform GetTransform() const { return transform_; }
	MaterialFactory* GetMaterial() { 
		auto matComp = GetComponent<MaterialComponent>();
		return matComp ? matComp->GetMaterialFactory() : nullptr;
	}
	[[deprecated("Use GetMaterial instead")]]
	MaterialFactory* GetMartial() { return GetMaterial(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU()const { 
		auto matComp = GetComponent<MaterialComponent>();
		if (matComp && matComp->GetTextureSrvHandleGPU().ptr != 0) {
			return matComp->GetTextureSrvHandleGPU();
		}
		return textureSrvHandleGPU_; 
	}

	virtual Mesh GetMesh();

	D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView();
	ID3D12Resource* GetWvpDataResource() { return wvpDataResource_[s_wvpIndex].Get(); }
	int GetVertexSize() { return vertexSize_; }
	TransformationMatrix* GetWvpData() { return wvpData_[s_wvpIndex]; }

	ID3D12Resource* GetIndexResource() { return indexResource_.Get(); }
	D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() { return &indexBufferView_; }

	virtual int GetIndexSize() { return indexSize_; }

	void SetShader(ShaderName shader) { 
		if (auto mat = GetComponent<MaterialComponent>()) mat->SetShader(shader); 
		if (shader == SkyBoxShader) {
			SetCullMode(kCullModeFront);
			SetFrustumCullingEnabled(false);
		}
	}
	void SetBlend(BlendMode blend) { 
		if (auto mat = GetComponent<MaterialComponent>()) mat->SetBlend(blend); 
	}

	void SetCullMode(CullMode cull) {
		cullMode_ = cull;
		if (auto mat = GetComponent<MaterialComponent>()) mat->SetCullMode(cull);
	}

	ShaderName GetShader() { 
		if (auto mat = GetComponent<MaterialComponent>()) return mat->GetShader();
		return "ObjectShader"; 
	}
	BlendMode GetBlend() { 
		if (auto mat = GetComponent<MaterialComponent>()) return mat->GetBlend();
		return BlendMode::kBlendModeNone; 
	}
	CullMode GetCullMode() const {
		if (auto mat = GetComponent<MaterialComponent>()) return mat->GetCullMode();
		return cullMode_;
	}

	void SetLocalAABB(const AABB& aabb) { localAABB_ = aabb; }
	const AABB& GetLocalAABB() const { return localAABB_; }

	void SetLocalBoundingSphere(const BoundingSphere& sphere) { localSphere_ = sphere; }
	const BoundingSphere& GetLocalBoundingSphere() const { return localSphere_; }

	AABB GetWorldAABB() const;
	BoundingSphere GetWorldBoundingSphere() const;

	void SetFrustumCullingEnabled(bool enable) { isFrustumCullingEnabled_ = enable; }
	bool IsFrustumCullingEnabled() const { return isFrustumCullingEnabled_; }

	void ImGuiInnerComponents() override;

protected:
	CullMode cullMode_ = kCullModeBack;
	AABB localAABB_{ {-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f} };
	BoundingSphere localSphere_{ {0.0f, 0.0f, 0.0f}, 0.866f };
	bool isFrustumCullingEnabled_ = true;
};

