#pragma once
#include "../Object/GameObject.h"
#include <nlohmann/json.hpp>

class LightManager;

class BaseLight : public GameObject
{
protected:
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	float intensity_ = 1.0f;

public:
	BaseLight() = default;
	virtual ~BaseLight() = default;

	const Vector4& GetColor() const { return color_; }
	void SetColor(const Vector4& color) { color_ = color; }

	float GetIntensity() const { return intensity_; }
	void SetIntensity(float intensity) { intensity_ = intensity; }

	virtual void ApplyToLightManager(LightManager& manager) = 0;

	void Serialize(nlohmann::json& out) const override {
		GameObject::Serialize(out);
		out["lightColor"] = { color_.x, color_.y, color_.z, color_.w };
		out["intensity"] = intensity_;
	}

	void Deserialize(const nlohmann::json& in) override {
		GameObject::Deserialize(in);
		if (in.contains("lightColor") && in["lightColor"].size() >= 4) {
			color_ = { in["lightColor"][0], in["lightColor"][1], in["lightColor"][2], in["lightColor"][3] };
		}
		if (in.contains("intensity")) {
			intensity_ = in["intensity"];
		}
	}
};
