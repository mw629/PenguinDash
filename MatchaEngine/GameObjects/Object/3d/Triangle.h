#pragma once
#include "VariableTypes.h"
#include "MaterialFactory.h"
#include "PipelineState.h"
#include <memory>
#include "../Object/GameObject.h"
#include "../Component/MaterialComponent.h"

class Triangle : public GameObject
{
private:

	Vector4 vertex_[3];
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_{};
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	VertexData* vertexData_ = nullptr;


	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_{};
	TransformationMatrix* wvpData_{};


public:
	~Triangle();

	static void SetScreenSize(Vector2 screenSize);


	void Initialize( D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU);
	
	void CreateVertexData();

	void CreateWVP();

	void CreateTriangle();

	void SettingWvp(Matrix4x4 viewMatrix);


	void SetShape();
	void SetTransform(Transform transform) { transform_ = transform; }
	void SetVertex(Vector4 vertex[3]);
	void SetMaterialLighting(bool isActive) { 
		auto matComp = GetComponent<MaterialComponent>();
		if(matComp) matComp->GetMaterialFactory()->SetMaterialLighting(isActive);
	}

	D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() { return &vertexBufferView_; }
	ID3D12Resource* GetVertexResource()const { return wvpResource_.Get(); }
	MaterialFactory* GetMaterial()const { 
		auto matComp = GetComponent<MaterialComponent>();
		return matComp ? matComp->GetMaterialFactory() : nullptr;
	}
	[[deprecated("Use GetMaterial instead")]]
	MaterialFactory* GetMartial()const { return GetMaterial(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU()const { return textureSrvHandleGPU_; }

	void SetBlend(BlendMode blend) { blend_ = blend; }
	ShaderName GetShader() { return shader_; }
	BlendMode GetBlend() { return blend_; }

	void SetCullMode(CullMode cull) {
		cullMode_ = cull;
		if (auto mat = GetComponent<MaterialComponent>()) mat->SetCullMode(cull);
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

private:
	ShaderName shader_ = "ObjectShader";
	BlendMode blend_ = BlendMode::kBlendModeNone;
	CullMode cullMode_ = kCullModeBack;
	AABB localAABB_{ {-0.1f, -0.1f, -0.01f}, {0.1f, 0.1f, 0.01f} };
	BoundingSphere localSphere_{ {0.0f, 0.0f, 0.0f}, 0.15f };
	bool isFrustumCullingEnabled_ = true;
};

