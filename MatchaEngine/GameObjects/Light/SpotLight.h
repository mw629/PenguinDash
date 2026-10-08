#pragma once
#include "BaseLight.h"
#include <cmath>
#include <numbers>

class SpotLight : public BaseLight
{
private:
	float distance_ = 4.0f;
	float decay_ = 2.0f;
	float cosAngle_ = std::cos(std::numbers::pi_v<float> / 3.0f);
	float cosFalloffStart_ = std::cos(std::numbers::pi_v<float> / 3.0f) + 0.1f;

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