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




/// オブジェクトの読み込み///

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


//ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename)
//{
//	std::unique_ptr<ModelManager> objManager = std::make_unique<ModelManager>();
//
//	if (objManager.get()->DuplicateConfirmation(directoryPath, filename)) {
//		return objManager.get()->DuplicateReturn(directoryPath, filename);
//	}
//
//	ModelData modelData;
//	std::vector<Vector4> positions;//位置
//	std::vector<Vector3> normals;//法線
//	std::vector<Vector2> texcoords;//テクスチャ座標
//	std::string line;//1行分の文字列を入れる変数
//	VertexData Triangle[3]{};
//
//	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
//	assert(file.is_open());
//
//	while (std::getline(file, line))
//	{
//		std::string identifier;
//		std::istringstream s(line);
//		s >> identifier;
//
//		if (identifier == "v") {
//			Vector4 position{};
//			s >> position.x >> position.y >> position.z;
//			position.w = 1.0f;
//			position.x *= -1.0f;
//			positions.push_back(position);
//		}
//		else if (identifier == "vt") {
//			Vector2 texcoord{};
//			s >> texcoord.x >> texcoord.y;
//			texcoord.y = 1.0f - texcoord.y;
//			texcoords.push_back(texcoord);
//		}
//		else if (identifier == "vn") {
//			Vector3 normal{};
//			s >> normal.x >> normal.y >> normal.z;
//			normal.x *= -1.0f;
//			normals.push_back(normal);
//		}
//		else if (identifier == "f") {
//			//面は三角形限定。そのほか未対応
//			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
//				std::string vertexDefinition;
//				s >> vertexDefinition;
//				//頂点の要素へのIndexは「位置/UV/法線」で格納されてるので、分解してIndexを取得する
//				std::istringstream v(vertexDefinition);
//				uint32_t elementIndices[3]{};
//				for (int32_t element = 0; element < 3; ++element) {
//					std::string index;
//					std::getline(v, index, '/');//区切りでインデックスを読んでいく
//					elementIndices[element] = std::stoi(index);
//				}
//
//				Vector4 position = positions[elementIndices[0] - 1];
//				Vector2 texcoord = texcoords[elementIndices[1] - 1];
//				Vector3 normal = normals[elementIndices[2] - 1];
//				Triangle[faceVertex] = { position ,texcoord ,normal };
//
//			}
//			modelData.vertices.push_back(Triangle[2]);
//			modelData.vertices.push_back(Triangle[1]);
//			modelData.vertices.push_back(Triangle[0]);
//		}
//		else if (identifier == "mtllib") {
//			//makterialTemplateLibraryファイルの名前を取得する
//			std::string materiaFilename;
//			s >> materiaFilename;
//			//基本的にobjファイルと同一改装にmtlは存在させるので、ディレクトリ名とファイル名を渡す 
//			modelData.material = LoadMaterialTemplateFile(directoryPath, materiaFilename);
//		}
//	}
//
//	std::unique_ptr<Texture> texture = std::make_unique<Texture>();
//
//	modelData.textureIndex = texture->CreateTexture(modelData.material.textureFilePath);
//
//	objManager.get()->SetModelList(modelData, directoryPath, filename);
//
//	return modelData;
//
//}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename)//NodeAnimationで使う
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
		bool isFlat = (sizeX < 0.05f || sizeY < 0.05f || sizeZ < 0.05f);

		if (sizeX < 0.05f) { minPos.x -= 0.1f; maxPos.x += 0.1f; }
		if (sizeY < 0.05f) { minPos.y -= 0.1f; maxPos.y += 0.1f; }
		if (sizeZ < 0.05f) { minPos.z -= 0.1f; maxPos.z += 0.1f; }

		modelData.localAABB.min = minPos;
		modelData.localAABB.max = maxPos;
		modelData.localSphere.center = {
			(minPos.x + maxPos.x) * 0.5f,
			(minPos.y + maxPos.y) * 0.5f,
			(minPos.z + maxPos.z) * 0.5f
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
		modelData.localAABB = { {-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f} };
		modelData.localSphere = { {0.0f, 0.0f, 0.0f}, 0.866f };
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
		bool isFlat = (sizeX < 0.05f || sizeY < 0.05f || sizeZ < 0.05f);

		if (sizeX < 0.05f) { minPos.x -= 0.1f; maxPos.x += 0.1f; }
		if (sizeY < 0.05f) { minPos.y -= 0.1f; maxPos.y += 0.1f; }
		if (sizeZ < 0.05f) { minPos.z -= 0.1f; maxPos.z += 0.1f; }

		modelData.localAABB.min = minPos;
		modelData.localAABB.max = maxPos;
		modelData.localSphere.center = {
			(minPos.x + maxPos.x) * 0.5f,
			(minPos.y + maxPos.y) * 0.5f,
			(minPos.z + maxPos.z) * 0.5f
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
		modelData.localAABB = { {-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f} };
		modelData.localSphere = { {0.0f, 0.0f, 0.0f}, 0.866f };
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
	//Node result;
	//aiMatrix4x4 aiLocalMatrix = node->mTransformation;//nodeのLocalMatrixを取得
	//aiLocalMatrix.Transpose();//列ベクトル形式を行ベクトル形式に転置
	//for (int i = 0; i < 4; i++) {
	//	for (int j = 0; j < 4; j++) {
	//		result.localMatrix.m[i][j] = aiLocalMatrix[i][j];//ほかの要素も同様に
	//	}
	//}
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

///テクスチャの読み込み///


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
