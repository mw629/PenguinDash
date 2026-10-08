#pragma once
#pragma once
#include <VariableTypes.h>
#include "ObjectBase.h"
#include <Model.h>
#include "Line.h"

class TransformAnimation :public ObjectBase
{
private:

	ModelData modelData_{};

	Animation animation_;
	std::string currentAnimationName_ = "";
	float animationTime_ = 0.0f;

	Skeleton skeleton_;

	Matrix4x4 localMatrix_;

public:

	~TransformAnimation()override;
	TransformAnimation();

	void Initialize(ModelData modelData, const std::string& directoryPath, const std::string& filename);

	void SettingWvp(Matrix4x4 viewMatrix) override;

	Skeleton CreateSkeleton(const Node& rootNode);
	int32_t CreateJoint(const Node& node,
		const std::optional<int32_t>& parent, std::vector<Joint>& joints);

	void ApplyAnimation();

	void Update(Matrix4x4 viewMatrix);

	void SetAnimationTime(float time) { animationTime_ = time; }
	float GetAnimationTime() const { return animationTime_; }
	float GetDuration() const { 
		if (animation_.animationClips.find(currentAnimationName_) != animation_.animationClips.end()) {
			return animation_.animationClips.at(currentAnimationName_).duration;
		}
		return 0.0f;
	}
	void SetAnimation(const std::string& name) { currentAnimationName_ = name; animationTime_ = 0.0f; }

	ModelData GetModelData() { return modelData_; }

	Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframe, float time);
	Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframe, float time);

	SkinCluster CreateSkinCluster(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriprtorSize);

	Mesh GetMesh()override { return modelData_.mesh; }
};

