#include "TransformAnimation.h"
#include "Calculation.h"
#include <Load.h>

namespace {
	ID3D12Device* device;
}

TransformAnimation::~TransformAnimation()
{
}

TransformAnimation::TransformAnimation()
{
}

void TransformAnimation::Initialize(ModelData modelData, const std::string& directoryPath, const std::string& filename)
{
	modelData_ = modelData;
	animation_ = LoadAnimationFile(directoryPath, filename);
	if (!animation_.animationClips.empty()) {
		currentAnimationName_ = animation_.animationClips.begin()->first;
	}
	textureSrvHandleGPU_ = texture->TextureData(modelData_.textureIndex);

	skeleton_ = CreateSkeleton(modelData_.rootNode);

	AddComponent<MaterialComponent>();
	CreateObject();
}


void TransformAnimation::SettingWvp(Matrix4x4 viewMatrix) {
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth_) / float(kClientHeight_), 0.1f, 10000.0f);
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.translate, transform_.scale, transform_.rotate);
	Matrix4x4 worldViewProjectionMatrix = MultiplyMatrix4x4(worldMatrix, MultiplyMatrix4x4(viewMatrix, projectionMatrix));
	Matrix4x4 worldInverseTranspose = TransposeMatrix4x4(Inverse(worldViewProjectionMatrix));


	GetWvpData()->WVP = localMatrix_ * worldMatrix * worldViewProjectionMatrix;
	GetWvpData()->World = localMatrix_ * worldMatrix;
	GetWvpData()->WorldInverseTranspose = worldInverseTranspose;
}

Skeleton TransformAnimation::CreateSkeleton(const Node& rootNode)
{
	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

	//名前とindexのマッピングを行いアクセスしやすくする
	for (const Joint& joint : skeleton.joints) {
		skeleton.jointMap.emplace(joint.name, joint.index);
	}
	return skeleton;

}

int32_t TransformAnimation::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints)
{
	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = IdentityMatrix();
	joint.transform = node.transform;
	joint.index = int32_t(joints.size());
	joint.parent = parent;
	joints.push_back(joint);
	for (const Node& child : node.children) {
		//子Jointを作成しindexを登録
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}
	return joint.index;

}





void TransformAnimation::Update(Matrix4x4 viewMatrix)
{
	float currentDuration = GetDuration();
	if (currentDuration > 0.0f) {
		animationTime_ += 1.0f / 60.0f;//時間を進める
		animationTime_ = std::fmod(animationTime_, currentDuration);//リピート再生

		AnimationClip& clip = animation_.animationClips.at(currentAnimationName_);
		AnimationNode& rootNodeAnimation = clip.AnimationNodes[modelData_.rootNode.name];

		Vector3 translate = CalculateValue(rootNodeAnimation.translate, animationTime_);
		Quaternion rotate = CalculateValue(rootNodeAnimation.rotate, animationTime_);

		Vector3 scale = CalculateValue(rootNodeAnimation.scale, animationTime_);
		localMatrix_ = MakeAffineMatrix(translate, scale, rotate);
	}

	SettingWvp(viewMatrix);
}


Vector3 TransformAnimation::CalculateValue(const std::vector<KeyframeVector3>& keyframe, float time)
{
	assert(!keyframe.empty());
	if (keyframe.size() == 1 || time <= keyframe[0].time) {//キーは一つか、時刻がキーフレーム前なら最初の値にする
		return keyframe[0].value;
	}

	for (size_t index = 0; index < keyframe.size() - 1; ++index) {
		size_t nextIndex = index + 1;
		//IndexとnextIndexの二つがkeyframeを取得して範囲内に時刻があるかの判定
		if (keyframe[index].time <= time && time <= keyframe[nextIndex].time) {
			//範囲内を保管する
			float t = (time - keyframe[index].time) / (keyframe[nextIndex].time - keyframe[index].time);
			return Lerp(keyframe[index].value, keyframe[nextIndex].value, t);
		}
	}
	// 範囲外の場合は最後の値を返す
	return keyframe.back().value;
}

Quaternion TransformAnimation::CalculateValue(const std::vector<KeyframeQuaternion>& keyframe, float time)
{
	assert(!keyframe.empty());
	if (keyframe.size() == 1 || time <= keyframe[0].time) {//キーは一つか、時刻がキーフレーム前なら最初の値にする
		return keyframe[0].value;
	}

	for (size_t index = 0; index < keyframe.size() - 1; ++index) {
		size_t nextIndex = index + 1;
		//IndexとnextIndexの二つがkeyframeを取得して範囲内に時刻があるかの判定
		if (keyframe[index].time <= time && time <= keyframe[nextIndex].time) {
			//範囲内を保管する
			float t = (time - keyframe[index].time) / (keyframe[nextIndex].time - keyframe[index].time);
			return Lerp(keyframe[index].value, keyframe[nextIndex].value, t);
		}
	}
	// 範囲外の場合は最後の値を返す
	return keyframe.back().value;
}

SkinCluster TransformAnimation::CreateSkinCluster(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize)
{
	SkinCluster skinCluster;

	skinCluster.paletteResource = GraphicsDevice::CreateBufferResource(sizeof(WellForGPU) * skeleton_.joints.size());
	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = { mappedPalette,skeleton_.joints.size() };
	skinCluster.paletteSrvHandle.first = GetCPUDescriptorHandle(descriptorHeap, descriptorSize);
	skinCluster.paletteSrvHandle.second = GetGPUDescriptorHandle(descriptorHeap, descriptorSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = UINT(skeleton_.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
	device->CreateShaderResourceView(skinCluster.paletteResource.Get(), &paletteSrvDesc, skinCluster.paletteSrvHandle.first);

	skinCluster.influenceResource = GraphicsDevice::CreateBufferResource(sizeof(VertexInfluence) * modelData_.mesh.vertexSize);


	return skinCluster;
}
