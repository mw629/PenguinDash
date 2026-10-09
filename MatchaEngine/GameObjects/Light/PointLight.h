#pragma once
#include "BaseLight.h"
#include <numbers>

class PointLight : public BaseLight
{
private:
	static constexpr float kDefaultRadius = 10.0f;
	static constexpr float kDefaultDecay = 2.0f;
	static constexpr int kGizmoCircleSegments = 16;

	static constexpr float kIntensityDragSpeed = 0.01f;
	static constexpr float kIntensityMin = 0.0f;
	static constexpr float kIntensityMax = 10.0f;

	static constexpr float kRadiusDragSpeed = 0.1f;
	static constexpr float kRadiusMin = 0.0f;
	static constexpr float kRadiusMax = 100.0f;

	static constexpr float kDecayDragSpeed = 0.1f;
	static constexpr float kDecayMin = 0.0f;
	static constexpr float kDecayMax = 10.0f;

	float radius_ = kDefaultRadius;
	float decay_ = kDefaultDecay;

public:
	PointLight();
	std::string GetTypeName() const override { return "PointLight"; }
	std::shared_ptr<GameObject> Clone() const override;

	void ImGui(bool drawTransform = true) override;
	void Draw(class Draw& draw) override;
	void ApplyToLightManager(class LightManager& manager) override;

	void Serialize(nlohmann::json& out) const override;
	void Deserialize(const nlohmann::json& in) override;

	float GetRadius() const { return radius_; }
	void SetRadius(float radius) { radius_ = radius; }

	float GetDecay() const { return decay_; }
	void SetDecay(float decay) { decay_ = decay; }
};
