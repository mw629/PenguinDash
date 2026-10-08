#include "ObjectBase.h"
#include "Calculation.h"

#ifdef _USE_IMGUI
#include <imgui.h>
#endif // _USE_IMGUI


// 静的メンバーの定義
float ObjectBase::kClientWidth_ = 0.0f;
float ObjectBase::kClientHeight_ = 0.0f;
int ObjectBase::s_wvpIndex = 0;

ObjectBase::~ObjectBase()
{

}


void ObjectBase::SetObjectResource(Vector2 ClientSize)
{
	kClientWidth_ = ClientSize.x;
	kClientHeight_ = ClientSize.y;
}

void ObjectBase::CreateWVP()
{
	for (int i = 0; i < 2; i++) {
		wvpDataResource_[i] = GraphicsDevice::CreateBufferResource(sizeof(TransformationMatrix) * maxInstanceCount_);

		wvpDataResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_[i]));
		for (int j = 0; j < maxInstanceCount_; j++) {
			wvpData_[i][j].WVP = IdentityMatrix();
			wvpData_[i][j].World = IdentityMatrix();
			wvpData_[i][j].WorldInverseTranspose = IdentityMatrix();
			wvpData_[i][j].numBones = 0;
		}
	}
}

void ObjectBase::CreateIndexResource()
{
	indexResource_ = GraphicsDevice::CreateBufferResource(sizeof(uint32_t) * 6); ;


	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

	indexSize_ = sizeof(uint32_t) * 6;

	indexData_[0] = 0;
	indexData_[1] = 1;
	indexData_[2] = 2;
	indexData_[3] = 1;
	indexData_[4] = 3;
	indexData_[5] = 2;

}


void ObjectBase::SettingWvp(Matrix4x4 viewMatrix)
{
	Matrix4x4 projectionMatri = MakePerspectiveFovMatrix(0.45f, float(kClientWidth_) / float(kClientHeight_), 0.1f, 10000.0f);

	if (isInstancing_ && !instancingTransforms_.empty()) {
		int count = min(maxInstanceCount_, static_cast<int>(instancingTransforms_.size()));
		for (int i = 0; i < count; ++i) {
			Matrix4x4 worldMatrix = MakeAffineMatrix(instancingTransforms_[i].translate, instancingTransforms_[i].scale, instancingTransforms_[i].rotate);
			Matrix4x4 worldViewProjectionMatrix = MultiplyMatrix4x4(worldMatrix, MultiplyMatrix4x4(viewMatrix, projectionMatri));
			Matrix4x4 worldInverseTranspose = TransposeMatrix4x4(Inverse(worldMatrix));

			wvpData_[s_wvpIndex][i].WVP = worldViewProjectionMatrix;
			wvpData_[s_wvpIndex][i].World = worldMatrix;
			wvpData_[s_wvpIndex][i].WorldInverseTranspose = worldInverseTranspose;
			wvpData_[s_wvpIndex][i].numBones = 0;
		}
	} else {
		Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.translate, transform_.scale, transform_.rotate);
		Matrix4x4 worldViewProjectionMatrix = MultiplyMatrix4x4(worldMatrix, MultiplyMatrix4x4(viewMatrix, projectionMatri));
		Matrix4x4 worldInverseTranspose = TransposeMatrix4x4(Inverse(worldMatrix)); // 法線変換用の行列を計算

		wvpData_[s_wvpIndex][0].WVP = worldViewProjectionMatrix;
		wvpData_[s_wvpIndex][0].World = worldMatrix;
		wvpData_[s_wvpIndex][0].WorldInverseTranspose = worldInverseTranspose;
		wvpData_[s_wvpIndex][0].numBones = 0;
	}
};

void ObjectBase::CreateObject()
{
	CreateVertexData();
	CreateWVP();
	CreateIndexResource();
}

void ObjectBase::CreateVertexData()
{
	// 基底クラスのデフォルト実装（派生クラスでオーバーライドされる）
}

Mesh ObjectBase::GetMesh()
{
	Mesh mesh;
	mesh.vertexBufferView = vertexBufferView_;
	mesh.vertexSize = vertexSize_;
	mesh.indexBufferView_ = indexBufferView_;
	return mesh;
}

D3D12_VERTEX_BUFFER_VIEW* ObjectBase::GetVertexBufferView()
{
	return &vertexBufferView_;
}

AABB ObjectBase::GetWorldAABB() const
{
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.translate, transform_.scale, transform_.rotate);
	return TransformAABB(localAABB_, worldMatrix);
}

BoundingSphere ObjectBase::GetWorldBoundingSphere() const
{
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.translate, transform_.scale, transform_.rotate);
	return TransformBoundingSphere(localSphere_, worldMatrix);
}

void ObjectBase::ImGuiInnerComponents()
{
#ifdef _USE_IMGUI
	if (ImGui::CollapsingHeader(LanguageManager::Tr("Rendering & Culling"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox(LanguageManager::Tr("Frustum Culling"), &isFrustumCullingEnabled_);

		if (!GetComponent<MaterialComponent>()) {
			static const CullMode cullModes[] = { kCullModeNone, kCullModeFront, kCullModeBack };
			static const char* cullNames[] = { "None (Both Sides)", "Front", "Back (Standard)" };
			int current_cull = 0;
			for (int i = 0; i < IM_ARRAYSIZE(cullModes); ++i) {
				if (cullMode_ == cullModes[i]) { current_cull = i; break; }
			}
			if (ImGui::Combo(LanguageManager::Tr("Cull Mode"), &current_cull, cullNames, IM_ARRAYSIZE(cullNames))) {
				cullMode_ = cullModes[current_cull];
			}
		}

		if (ImGui::TreeNode(LanguageManager::Tr("Bounding Box (AABB)"))) {
			AABB worldAABB = GetWorldAABB();
			ImGui::Text("Local Min: (%.2f, %.2f, %.2f)", localAABB_.min.x, localAABB_.min.y, localAABB_.min.z);
			ImGui::Text("Local Max: (%.2f, %.2f, %.2f)", localAABB_.max.x, localAABB_.max.y, localAABB_.max.z);
			ImGui::Text("World Min: (%.2f, %.2f, %.2f)", worldAABB.min.x, worldAABB.min.y, worldAABB.min.z);
			ImGui::Text("World Max: (%.2f, %.2f, %.2f)", worldAABB.max.x, worldAABB.max.y, worldAABB.max.z);
			ImGui::TreePop();
		}
	}
#endif
}

