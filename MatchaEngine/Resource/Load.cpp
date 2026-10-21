#include "Load.h"
#include "Graphics/GraphicsDevice.h"
#include "Math/Calculation.h"
#include "Core/LogHandler.h"
#include <fstream>
#include <sstream>
#include <d3dx12.h>
#include "Texture.h"
#include "ModelManager.h"
#include <cfloat>
#include <algorithm>

namespace {
constexpr float kFlatDimensionThreshold = 0.05f;
constexpr float kFlatPadding = 0.1f;
constexpr float kHalfCenterMultiplier = 0.5f;
constexpr float kFallbackBoundingExtent = 0.5f;
constexpr float kFallbackBoundingSphereRadius = 0.866025f; // sqrt(3) / 2
}

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData;//構築するMaterialData
	std::string line;//ファイルから読んだ一行を格納する
	std::string fullPath = directoryPath + "/" + filename;
	std::ifstream file(fullPath);//ファイルを開く
	if (!file.is_open()) {
		LOG_ERROR("Failed to open material file: " + fullPath);
		assert(file.is_open());
		return materialData;
	}

	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		//identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			//連結してファイルパス
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}


ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename)
{
	std::unique_ptr<ModelManager> objManager = std::make_unique<ModelManager>();

	if (objManager.get()->DuplicateConfirmation(directoryPath, filename)) {
		return objManager.get()->DuplicateReturn(directoryPath, filename);
	}

	ModelData modelData;

	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(),
		aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_Triangulate);
	if (!scene || !scene->HasMeshes()) {
		std::string assimpErr = importer.GetErrorString();
		std::string errorMessage = std::format("Failed to load model file: {}\nAssimp Error: {}", filePath, assimpErr.empty() ? "(none)" : assimpErr);
		LOG_ERROR(errorMessage);
		MessageBoxA(nullptr, errorMessage.c_str(), "Model Load Error", MB_OK | MB_ICONERROR);
		assert(false && "Model Load Error");
		return modelData;
	}

	std::unique_ptr<Texture> texture = std::make_unique<Texture>();

	Vector3 minPos = { FLT_MAX, FLT_MAX, FLT_MAX };
	Vector3 maxPos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	bool hasVertices = false;

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		
		std::vector<VertexData> vertices;
		std::vector<int32_t>indices;
		SubMesh subMesh;

		vertices.resize(mesh->mNumVertices);

		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			aiVector3D& position = mesh->mVertices[vertexIndex];
			aiVector3D normal = {0.0f, 0.0f, 0.0f};
			if (mesh->HasNormals()) {
				normal = mesh->mNormals[vertexIndex];
			}
			aiVector3D texcord = {0.0f, 0.0f, 0.0f};
			if (mesh->HasTextureCoords(0)) {
				texcord = mesh->mTextureCoords[0][vertexIndex];
			}

			vertices[vertexIndex].position = { -position.x,position.y,position.z,1.0f };
			vertices[vertexIndex].normal = { -normal.x,normal.y,normal.z };
			vertices[vertexIndex].texcoord = { texcord.x,texcord.y };

			minPos.x = (std::min)(minPos.x, vertices[vertexIndex].position.x);
			minPos.y = (std::min)(minPos.y, vertices[vertexIndex].position.y);
			minPos.z = (std::min)(minPos.z, vertices[vertexIndex].position.z);

			maxPos.x = (std::max)(maxPos.x, vertices[vertexIndex].position.x);
			maxPos.y = (std::max)(maxPos.y, vertices[vertexIndex].position.y);
			maxPos.z = (std::max)(maxPos.z, vertices[vertexIndex].position.z);
			hasVertices = true;
		}
		
		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = subMesh.skinClusterData[jointName];
			
			// Global skin cluster data for backward compatibility / animation root
			JointWeightData& globalJointWeightData = modelData.skinClusterData[jointName];

			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D translate;
			aiVector3D scale;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
			Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
				Vector3{ -translate.x, translate.y, translate.z }, // translate
				Vector3{ scale.x, scale.y, scale.z },              // scale
				Quaternion{ rotate.x, -rotate.y, -rotate.z, rotate.w }); // rotate
			jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatrix);
			globalJointWeightData.inverseBindPoseMatrix = jointWeightData.inverseBindPoseMatrix;

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
				jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight,bone->mWeights[weightIndex].mVertexId });
				// (Optional: can populate global if needed, but submesh data is preferred)
				globalJointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight,bone->mWeights[weightIndex].mVertexId });
			}
		}
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& fence = mesh->mFaces[faceIndex];
			assert(fence.mNumIndices == 3);

			for (uint32_t element = 0; element < fence.mNumIndices; ++element) {
				uint32_t vertexIndex = fence.mIndices[element];
				indices.push_back(vertexIndex);
			}

		}
		// materialを解析する
		subMesh.textureIndex = -1;
		if (mesh->mMaterialIndex < scene->mNumMaterials) {
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
				aiString textureFilePath;
				material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
				subMesh.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
				subMesh.textureIndex = texture->CreateTexture(subMesh.material.textureFilePath);
			}
		}

		subMesh.mesh = objManager.get()->CreateMesh(vertices, indices);
		modelData.subMeshes.push_back(subMesh);
	}

	if (hasVertices) {
		float sizeX = maxPos.x - minPos.x;
		float sizeY = maxPos.y - minPos.y;
		float sizeZ = maxPos.z - minPos.z;
		bool isFlat = (sizeX < kFlatDimensionThreshold || sizeY < kFlatDimensionThreshold || sizeZ < kFlatDimensionThreshold);

		if (sizeX < kFlatDimensionThreshold) { minPos.x -= kFlatPadding; maxPos.x += kFlatPadding; }
		if (sizeY < kFlatDimensionThreshold) { minPos.y -= kFlatPadding; maxPos.y += kFlatPadding; }
		if (sizeZ < kFlatDimensionThreshold) { minPos.z -= kFlatPadding; maxPos.z += kFlatPadding; }

		modelData.localAABB.min = minPos;
		modelData.localAABB.max = maxPos;
		modelData.localSphere.center = {
			(minPos.x + maxPos.x) * kHalfCenterMultiplier,
			(minPos.y + maxPos.y) * kHalfCenterMultiplier,
			(minPos.z + maxPos.z) * kHalfCenterMultiplier
		};
		Vector3 d = { maxPos.x - modelData.localSphere.center.x, maxPos.y - modelData.localSphere.center.y, maxPos.z - modelData.localSphere.center.z };
		modelData.localSphere.radius = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);

		int twoSided = 0;
		if (scene->mNumMaterials > 0 && scene->mMaterials[0]->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS && twoSided != 0) {
			modelData.cullMode = kCullModeNone;
		} else if (isFlat || filename.find("plane") != std::string::npos || filename.find("Plane") != std::string::npos) {
			modelData.cullMode = kCullModeNone;
		} else {
			modelData.cullMode = kCullModeBack;
		}
	} else {
		modelData.localAABB = { {-kFallbackBoundingExtent, -kFallbackBoundingExtent, -kFallbackBoundingExtent}, {kFallbackBoundingExtent, kFallbackBoundingExtent, kFallbackBoundingExtent} };
		modelData.localSphere = { {0.0f, 0.0f, 0.0f}, kFallbackBoundingSphereRadius };
		modelData.cullMode = kCullModeBack;
	}

	modelData.rootNode = ReadNode(scene->mRootNode);

	if (!modelData.subMeshes.empty()) {
		modelData.mesh = modelData.subMeshes[0].mesh;
		modelData.material = modelData.subMeshes[0].material;
		modelData.textureIndex = modelData.subMeshes[0].textureIndex;
	}

	objManager.get()->SetModelList(modelData, directoryPath, filename);

	return modelData;

}

ModelData AssimpLoadObjFile(const std::string& directoryPath, const std::string& filename)
{
	std::unique_ptr<ModelManager> objManager = std::make_unique<ModelManager>();

	if (objManager.get()->DuplicateConfirmation(directoryPath, filename)) {
		return objManager.get()->DuplicateReturn(directoryPath, filename);
	}

	ModelData modelData;

	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(),
		aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_Triangulate);
	if (!scene || !scene->HasMeshes()) {
		std::string assimpErr = importer.GetErrorString();
		std::string errorMessage = std::format("Failed to load model file: {}\nAssimp Error: {}", filePath, assimpErr.empty() ? "(none)" : assimpErr);
		LOG_ERROR(errorMessage);
		MessageBoxA(nullptr, errorMessage.c_str(), "Model Load Error", MB_OK | MB_ICONERROR);
		assert(false && "Model Load Error");
		return modelData;
	}

	std::unique_ptr<Texture> texture = std::make_unique<Texture>();

	Vector3 minPos = { FLT_MAX, FLT_MAX, FLT_MAX };
	Vector3 maxPos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	bool hasVertices = false;

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		std::vector<VertexData> vertices;
		std::vector<int32_t>indices;
		SubMesh subMesh;
		
		vertices.resize(mesh->mNumVertices);

		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			aiVector3D& position = mesh->mVertices[vertexIndex];
			aiVector3D normal = {0.0f, 0.0f, 0.0f};
			if (mesh->HasNormals()) {
				normal = mesh->mNormals[vertexIndex];
			}
			aiVector3D texcord = {0.0f, 0.0f, 0.0f};
			if (mesh->HasTextureCoords(0)) {
				texcord = mesh->mTextureCoords[0][vertexIndex];
			}

			vertices[vertexIndex].position = { -position.x,position.y,position.z,1.0f };
			vertices[vertexIndex].normal = { -normal.x,normal.y,normal.z };
			vertices[vertexIndex].texcoord = { texcord.x,texcord.y };

			minPos.x = (std::min)(minPos.x, vertices[vertexIndex].position.x);
			minPos.y = (std::min)(minPos.y, vertices[vertexIndex].position.y);
			minPos.z = (std::min)(minPos.z, vertices[vertexIndex].position.z);

			maxPos.x = (std::max)(maxPos.x, vertices[vertexIndex].position.x);
			maxPos.y = (std::max)(maxPos.y, vertices[vertexIndex].position.y);
			maxPos.z = (std::max)(maxPos.z, vertices[vertexIndex].position.z);
			hasVertices = true;
		}
		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = subMesh.skinClusterData[jointName];
			
			// Global skin cluster data for backward compatibility / animation root
			JointWeightData& globalJointWeightData = modelData.skinClusterData[jointName];

			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D translate;
			aiVector3D scale;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
			Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
				Vector3{ -translate.x, translate.y, translate.z }, // translate
				Vector3{ scale.x, scale.y, scale.z },              // scale
				Quaternion{ rotate.x, -rotate.y, -rotate.z, rotate.w }); // rotate
			jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatrix);
			globalJointWeightData.inverseBindPoseMatrix = jointWeightData.inverseBindPoseMatrix;

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
				jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight,bone->mWeights[weightIndex].mVertexId });
				globalJointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight,bone->mWeights[weightIndex].mVertexId });
			}
		}
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& fence = mesh->mFaces[faceIndex];
			assert(fence.mNumIndices == 3);

			for (uint32_t element = 0; element < fence.mNumIndices; ++element) {
				uint32_t vertexIndex = fence.mIndices[element];
				indices.push_back(vertexIndex);
			}

		}
		// materialを解析する
		subMesh.textureIndex = -1;
		if (mesh->mMaterialIndex < scene->mNumMaterials) {
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
				aiString textureFilePath;
				material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
				subMesh.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
				subMesh.textureIndex = texture->CreateTexture(subMesh.material.textureFilePath);
			}
		}

		subMesh.mesh = objManager.get()->CreateMesh(vertices, indices);
		modelData.subMeshes.push_back(subMesh);
	}

	if (hasVertices) {
		float sizeX = maxPos.x - minPos.x;
		float sizeY = maxPos.y - minPos.y;
		float sizeZ = maxPos.z - minPos.z;
		bool isFlat = (sizeX < kFlatDimensionThreshold || sizeY < kFlatDimensionThreshold || sizeZ < kFlatDimensionThreshold);

		if (sizeX < kFlatDimensionThreshold) { minPos.x -= kFlatPadding; maxPos.x += kFlatPadding; }
		if (sizeY < kFlatDimensionThreshold) { minPos.y -= kFlatPadding; maxPos.y += kFlatPadding; }
		if (sizeZ < kFlatDimensionThreshold) { minPos.z -= kFlatPadding; maxPos.z += kFlatPadding; }

		modelData.localAABB.min = minPos;
		modelData.localAABB.max = maxPos;
		modelData.localSphere.center = {
			(minPos.x + maxPos.x) * kHalfCenterMultiplier,
			(minPos.y + maxPos.y) * kHalfCenterMultiplier,
			(minPos.z + maxPos.z) * kHalfCenterMultiplier
		};
		Vector3 d = { maxPos.x - modelData.localSphere.center.x, maxPos.y - modelData.localSphere.center.y, maxPos.z - modelData.localSphere.center.z };
		modelData.localSphere.radius = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);

		int twoSided = 0;
		if (scene->mNumMaterials > 0 && scene->mMaterials[0]->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS && twoSided != 0) {
			modelData.cullMode = kCullModeNone;
		} else if (isFlat || filename.find("plane") != std::string::npos || filename.find("Plane") != std::string::npos) {
			modelData.cullMode = kCullModeNone;
		} else {
			modelData.cullMode = kCullModeBack;
		}
	} else {
		modelData.localAABB = { {-kFallbackBoundingExtent, -kFallbackBoundingExtent, -kFallbackBoundingExtent}, {kFallbackBoundingExtent, kFallbackBoundingExtent, kFallbackBoundingExtent} };
		modelData.localSphere = { {0.0f, 0.0f, 0.0f}, kFallbackBoundingSphereRadius };
		modelData.cullMode = kCullModeBack;
	}

	modelData.rootNode = ReadNode(scene->mRootNode);

	if (!modelData.subMeshes.empty()) {
		modelData.mesh = modelData.subMeshes[0].mesh;
		modelData.material = modelData.subMeshes[0].material;
		modelData.textureIndex = modelData.subMeshes[0].textureIndex;
	}

	objManager.get()->SetModelList(modelData, directoryPath, filename);


	return modelData;

}

Node ReadNode(aiNode* node)
{
	Node result;

	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate); // assimpの行列からSRTを抽出する関数を利用
	result.transform.scale = { scale.x, scale.y, scale.z }; // Scaleはそのまま
	result.transform.rotate = { rotate.x, -rotate.y, -rotate.z, rotate.w }; // x軸を反転、さらに回転方向が逆なので軸を反転させる
	result.transform.translate = { -translate.x, translate.y, translate.z }; // x軸を反転
	result.localMatrix = MakeAffineMatrix(result.transform.translate, result.transform.scale, result.transform.rotate);

	result.name = node->mName.C_Str(); // Nodeの名前
	result.children.resize(node->mNumChildren); // 子供の数だけ確保
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		// 再帰的に読んで階層構造を作っていく
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}
	return result;
}

Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename)
{
	Animation animation;
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	if (!scene || scene->mNumAnimations == 0) {
		std::string assimpErr = importer.GetErrorString();
		std::string errorMessage = std::format("Failed to load animation file: {}\nAssimp Error: {}", filePath, assimpErr.empty() ? "(none)" : assimpErr);
		LOG_ERROR(errorMessage);
		MessageBoxA(nullptr, errorMessage.c_str(), "Animation Load Error", MB_OK | MB_ICONERROR);
		assert(false && "Animation Load Error");
		return animation;
	}
	for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
		aiAnimation* animationAssimp = scene->mAnimations[i];
		AnimationClip clip;
		clip.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);//時間単位を秒に変換

		//AnimationNodeを解析
		for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {

			aiNodeAnim* AnimationNodeAssimp = animationAssimp->mChannels[channelIndex];
			AnimationNode& AnimationNode = clip.AnimationNodes[AnimationNodeAssimp->mNodeName.C_Str()];

			//Translate
			for (uint32_t keyIndex = 0; keyIndex < AnimationNodeAssimp->mNumPositionKeys; ++keyIndex) {
				aiVectorKey& keyAssimp = AnimationNodeAssimp->mPositionKeys[keyIndex];
				KeyframeVector3 keyframe;
				keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);//ここも秒に変換
				keyframe.value = { -keyAssimp.mValue.x,keyAssimp.mValue.y,keyAssimp.mValue.z };//右手→左手
				AnimationNode.translate.push_back(keyframe);
			}
			//Rotate
			for (uint32_t keyIndex = 0; keyIndex < AnimationNodeAssimp->mNumRotationKeys; ++keyIndex) {
				aiQuatKey& keyAssimp = AnimationNodeAssimp->mRotationKeys[keyIndex];
				KeyframeQuaternion keyframe;
				keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);//ここも秒に変換
				keyframe.value = { keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w };
				AnimationNode.rotate.push_back(keyframe);
			}
			//Scale
			for (uint32_t keyIndex = 0; keyIndex < AnimationNodeAssimp->mNumScalingKeys; ++keyIndex) {
				aiVectorKey& keyAssimp = AnimationNodeAssimp->mScalingKeys[keyIndex];
				KeyframeVector3 keyframe;
				keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);//ここも秒に変換
				keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
				AnimationNode.scale.push_back(keyframe);
			}
		}
		std::string clipName = animationAssimp->mName.C_Str();
		if (clipName.empty()) {
			clipName = "Anim_" + std::to_string(i);
		}
		animation.animationClips[clipName] = clip;
	}
	return animation;
}

DirectX::ScratchImage LoadTexture(const std::string& filePath) {

	//テクスチャファイルを読み込んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr;
	if (filePathW.ends_with(L".dds")) {
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	}
	else {
		hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}
	if (FAILED(hr)) {
		std::string errorMessage = std::format("Failed to load texture file: {}\nHRESULT: {}", filePath, FormatHResult(hr));
		LOG_ERROR(errorMessage);
		MessageBoxA(nullptr, errorMessage.c_str(), "Texture Load Error", MB_OK | MB_ICONERROR);
		assert(SUCCEEDED(hr) && "Texture Load Error");
		return image;
	}

	//ミニマップの作成
	DirectX::ScratchImage mipImages{};
	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		mipImages = std::move(image);
	}
	else {
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 4, mipImages);
	}
	if (FAILED(hr)) {
		std::string errorMessage = std::format("Failed to generate mipmaps for texture: {}\nHRESULT: {}", filePath, FormatHResult(hr));
		LOG_ERROR(errorMessage);
		MessageBoxA(nullptr, errorMessage.c_str(), "Texture Mipmap Error", MB_OK | MB_ICONERROR);
		assert(SUCCEEDED(hr) && "Texture Mipmap Error");
		return mipImages;
	}

	//ミニマップ付きのデータを返す
	return mipImages;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata)
{

	//metadataを基にResorceの設定

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);//Textureの幅
	resourceDesc.Height = UINT(metadata.height);//Textureの縦
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);//mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);;//奥行き or 配列Textureの配列数
	resourceDesc.Format = metadata.format;//TextureのFormat
	resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。１固定。
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);//Textureの次元数。普段使っているのは二次元

	//利用するHeapの設定

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;//細かい設定を行う


	//Resourceを作成

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,//Heapの設定
		D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。特になし
		&resourceDesc,//Resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST,//データ転送される設定
		nullptr,//Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource));
	if (FAILED(hr)) {
		CheckHResult(hr, "CreateCommittedResource failed for texture", device);
		assert(SUCCEEDED(hr));
	}
	return resource;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	if (!texture || !device || !commandList) {
		return nullptr;
	}

	std::vector<D3D12_SUBRESOURCE_DATA> subresources;

	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = GraphicsDevice::CreateBufferResource(intermediateSize);

	UpdateSubresources(commandList, texture, intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());


	//Textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);

	return intermediateResource;

}


namespace {
	int CPUNum = 1;
	int GPUNum = 1;
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriprtorSize)
{

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriprtorSize * CPUNum);
	CPUNum++;
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriprtorSize)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriprtorSize * GPUNum);
	GPUNum++;
	return handleGPU;
}

