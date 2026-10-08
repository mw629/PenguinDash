#pragma once  
#include <wrl.h>  
#include <d3d12.h>  
#include "../Core/VariableTypes.h"

class MaterialFactory
{
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_{};
	Material* materialData_{};

public:
	static constexpr Vector4 kDefaultColor = {1.0f, 1.0f, 1.0f, 1.0f};
	static constexpr float kDefaultShininess = 30.0f;
	static constexpr float kDefaultEnvironmentCoefficient = 0.0f;

	~MaterialFactory();

	void ImGui();

	void CreateMaterial(bool Lighting = false, float environmentCoefficient = kDefaultEnvironmentCoefficient);
	[[deprecated("Use CreateMaterial instead")]]
	void CreateMartial(bool Lighting = false, float environmentCoefficient = kDefaultEnvironmentCoefficient) {
		CreateMaterial(Lighting, environmentCoefficient);
	}

	void SetColor(Vector4 color) { materialData_->color = color; }
	void SetMaterialLighting(bool isActive) { materialData_->enableLighting = isActive ? 1 : 0; }
	void SetUVTransform(const Matrix4x4& uvTransform) { materialData_->uvTransform = uvTransform; }

	ID3D12Resource* GetMaterialResource() { return materialResource_.Get(); }
	Material* GetMaterialData() { return materialData_; }
};
