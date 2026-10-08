#include "Model.h"
#include "GraphicsDevice.h"
#include "Calculation.h"
#include "Load.h"
#include "Texture.h"
#include "ModelManager.h"
#include <fstream>
#include <sstream>


Model::~Model()
{
	// 基底クラス(Object3DBase)のデストラクタが自動的に呼ばれる
	// materialはunique_ptrなので自動的に解放される
}


void Model::Initialize(ModelData modelData)
{

	modelNumber_ = modelData.modelNumber;
	textureSrvHandleGPU_ = texture->TextureData(modelData.textureIndex);

	localAABB_ = modelData.localAABB;
	localSphere_ = modelData.localSphere;
	SetCullMode(modelData.cullMode);

	//アニメーション
	rootNode_ = modelData.rootNode;

	AddComponent<MaterialComponent>();
	auto matComp = GetComponent<MaterialComponent>();
	if (matComp && !modelData.material.textureFilePath.empty()) {
		matComp->SetTexturePath(modelData.material.textureFilePath);
	}

	subMeshMaterials_.clear();
	for (const auto& subMesh : modelData.subMeshes) {
		ModelSubMeshMaterial mat;
		if (subMesh.textureIndex != -1) {
			mat.textureSrvHandleGPU = texture->TextureData(subMesh.textureIndex);
		} else {
			mat.textureSrvHandleGPU = textureSrvHandleGPU_;
		}
		mat.materialFactory = std::make_unique<MaterialFactory>();
		mat.materialFactory->CreateMartial(false, 0.0f);
		subMeshMaterials_.push_back(std::move(mat));
	}

	CreateObject();
}


void Model::SettingWvp(Matrix4x4 viewMatrix) {
	SettingWvp(viewMatrix, nullptr);
}

void Model::SettingWvp(Matrix4x4 viewMatrix, const Matrix4x4* customProjection) {
	Matrix4x4 projectionMatri = customProjection
		? *customProjection
		: MakePerspectiveFovMatrix(0.45f, float(kClientWidth_) / float(kClientHeight_), 0.1f, 10000.0f);

	Matrix4x4 worldMatrixObj;
	if (isBillboard_) {
		Matrix4x4 invView = Inverse(viewMatrix);
		Vector3 right = Normalize(Vector3{ invView.m[0][0], invView.m[0][1], invView.m[0][2] });
		Vector3 up    = Normalize(Vector3{ invView.m[1][0], invView.m[1][1], invView.m[1][2] });
		Vector3 fwd   = Normalize(Vector3{ invView.m[2][0], invView.m[2][1], invView.m[2][2] });

		worldMatrixObj = IdentityMatrix();
		worldMatrixObj.m[0][0] = right.x * transform_.scale.x;
		worldMatrixObj.m[0][1] = right.y * transform_.scale.x;
		worldMatrixObj.m[0][2] = right.z * transform_.scale.x;

		worldMatrixObj.m[1][0] = up.x * transform_.scale.y;
		worldMatrixObj.m[1][1] = up.y * transform_.scale.y;
		worldMatrixObj.m[1][2] = up.z * transform_.scale.y;

		worldMatrixObj.m[2][0] = fwd.x * transform_.scale.z;
		worldMatrixObj.m[2][1] = fwd.y * transform_.scale.z;
		worldMatrixObj.m[2][2] = fwd.z * transform_.scale.z;

		worldMatrixObj.m[3][0] = transform_.translate.x;
		worldMatrixObj.m[3][1] = transform_.translate.y;
		worldMatrixObj.m[3][2] = transform_.translate.z;
		worldMatrixObj.m[3][3] = 1.0f;
	} else {
		worldMatrixObj = MakeAffineMatrix(transform_.translate, transform_.scale, transform_.rotate);
	}

	Matrix4x4 worldViewProjectionMatrixObj = MultiplyMatrix4x4(worldMatrixObj, MultiplyMatrix4x4(viewMatrix, projectionMatri));
	Matrix4x4 worldInverseTranspose = TransposeMatrix4x4(Inverse(worldMatrixObj));

	GetWvpData()->WVP = MultiplyMatrix4x4(rootNode_.localMatrix, worldViewProjectionMatrixObj);
	GetWvpData()->World = MultiplyMatrix4x4(rootNode_.localMatrix, worldMatrixObj);
	GetWvpData()->WorldInverseTranspose = worldInverseTranspose;
}



void Model::CreateObject()
{
	CreateWVP();
}

Mesh Model::GetMesh()
{
	ModelData data = ModelManager::GetModelData(modelNumber_);
	return data.mesh;
}

AABB Model::GetWorldAABB() const
{
	if (isBillboard_) {
		float maxScale = (std::max)({ transform_.scale.x, transform_.scale.y, transform_.scale.z });
		AABB aabb;
		aabb.min = { transform_.translate.x - maxScale, transform_.translate.y - maxScale, transform_.translate.z - maxScale };
		aabb.max = { transform_.translate.x + maxScale, transform_.translate.y + maxScale, transform_.translate.z + maxScale };
		return aabb;
	}
	Matrix4x4 worldMatrixObj = MakeAffineMatrix(transform_.translate, transform_.scale, transform_.rotate);
	Matrix4x4 worldMatrix = MultiplyMatrix4x4(rootNode_.localMatrix, worldMatrixObj);
	return TransformAABB(localAABB_, worldMatrix);
}

BoundingSphere Model::GetWorldBoundingSphere() const
{
	if (isBillboard_) {
		float maxScale = (std::max)({ transform_.scale.x, transform_.scale.y, transform_.scale.z });
		BoundingSphere sphere;
		sphere.center = transform_.translate;
		sphere.radius = maxScale * 1.5f;
		return sphere;
	}
	Matrix4x4 worldMatrixObj = MakeAffineMatrix(transform_.translate, transform_.scale, transform_.rotate);
	Matrix4x4 worldMatrix = MultiplyMatrix4x4(rootNode_.localMatrix, worldMatrixObj);
	return TransformBoundingSphere(localSphere_, worldMatrix);
}


