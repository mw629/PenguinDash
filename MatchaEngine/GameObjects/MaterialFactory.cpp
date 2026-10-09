#include "MaterialFactory.h"
#include "Graphics/GraphicsDevice.h"
#include "Math/Calculation.h"
#ifdef _USE_IMGUI
#include <imgui.h>
#endif


MaterialFactory::~MaterialFactory(){
	if (materialResource_) {
		materialResource_->Unmap(0, nullptr);
		materialResource_.Reset();
	}
}

void MaterialFactory::ImGui() {
#ifdef _USE_IMGUI
	ImGui::ColorEdit4("Color", &materialData_->color.x);
	bool enableLighting = materialData_->enableLighting != 0;
	if (ImGui::Checkbox("Enable Lighting", &enableLighting)) {
		materialData_->enableLighting = enableLighting ? 1 : 0;
	}
	ImGui::SliderFloat("Shininess", &materialData_->shininess, 0.0f, 100.0f);
	ImGui::SliderFloat("Environment Coefficient", &materialData_->environmentCoefficient, 0.0f, 1.0f);
#endif // _USE_IMGUI
}

void MaterialFactory::CreateMaterial(bool Lighting, float environmentCoefficient)
{
	//マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
	materialResource_ = GraphicsDevice::CreateBufferResource(sizeof(Material));
	//書き込むためのアドレス取得
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	//今回は書き込んでみる
	materialData_->color = kDefaultColor;
	materialData_->enableLighting = Lighting ? 1 : 0;
	materialData_->uvTransform = IdentityMatrix();
	materialData_->shininess = kDefaultShininess;
	materialData_->environmentCoefficient = environmentCoefficient;
}
