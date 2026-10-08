#pragma once
#include "BaseLight.h"

class PointLight : public BaseLight
{
private:
	float radius_ = 10.0f;
	float decay_ = 2.0f;

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
