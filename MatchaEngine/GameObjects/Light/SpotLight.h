#pragma once
#include "BaseLight.h"
#include <cmath>
#include <numbers>

class SpotLight : public BaseLight
{
private:
	static constexpr float kDefaultDistance = 4.0f;
	static constexpr float kDefaultDecay = 2.0f;
	static constexpr float kDefaultAngleDivisor = 3.0f;
	static constexpr float kDefaultFalloffStartOffset = 0.1f;
	static constexpr Vector3 kDefaultDirection = { 0.0f, -1.0f, 0.0f };
	static constexpr Vector3 kUpReference = { 0.0f, 1.0f, 0.0f };
	static constexpr Vector3 kRightReference = { 1.0f, 0.0f, 0.0f };
	static constexpr float kGizmoParallelThreshold = 0.01f;
	static constexpr int kGizmoCircleSegments = 16;

	static constexpr float kIntensityDragSpeed = 0.01f;
	static constexpr float kIntensityMin = 0.0f;
	static constexpr float kIntensityMax = 10.0f;

	static constexpr float kDistanceDragSpeed = 0.1f;
	static constexpr float kDistanceMin = 0.0f;
	static constexpr float kDistanceMax = 100.0f;

	static constexpr float kDecayDragSpeed = 0.1f;
	static constexpr float kDecayMin = 0.0f;
	static constexpr float kDecayMax = 10.0f;

	static constexpr float kAngleDragSpeed = 0.01f;
	static constexpr float kAngleMin = 0.0f;
	static constexpr float kAngleMax = 1.0f;

	static constexpr float kFalloffDragSpeed = 0.01f;
	static constexpr float kFalloffMin = 0.0f;
	static constexpr float kFalloffMax = 1.0f;

	float distance_ = kDefaultDistance;
	float decay_ = kDefaultDecay;
	float cosAngle_ = std::cos(std::numbers::pi_v<float> / kDefaultAngleDivisor);
	float cosFalloffStart_ = std::cos(std::numbers::pi_v<float> / kDefaultAngleDivisor) + kDefaultFalloffStartOffset;

public:
	SpotLight();
	std::string GetTypeName() const override { return "SpotLight"; }
	std::shared_ptr<GameObject> Clone() const override;

	void ImGui(bool drawTransform = true) override;
	void Draw(class Draw& draw) override;
	void ApplyToLightManager(class LightManager& manager) override;

	void Serialize(nlohmann::json& out) const override;
	void Deserialize(const nlohmann::json& in) override;

	float GetDistance() const { return distance_; }
	void SetDistance(float distance) { distance_ = distance; }

	float GetDecay() const { return decay_; }
	void SetDecay(float decay) { decay_ = decay; }

	float GetCosAngle() const { return cosAngle_; }
	void SetCosAngle(float cosAngle) { cosAngle_ = cosAngle; }

	float GetCosFalloffStart() const { return cosFalloffStart_; }
	void SetCosFalloffStart(float falloff) { cosFalloffStart_ = falloff; }
};