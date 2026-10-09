#pragma once
#include "../Object/GameObject.h"
#include <nlohmann/json.hpp>

class LightManager;

class BaseLight : public GameObject
{
protected:
	static constexpr Vector4 kDefaultLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	static constexpr float kDefaultIntensity = 1.0f;
	static constexpr size_t kColorComponentCount = 4;

	Vector4 color_ = kDefaultLightColor;
	float intensity_ = kDefaultIntensity;

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
		if (in.contains("lightColor") && in["lightColor"].size() >= kColorComponentCount) {
			color_ = { in["lightColor"][0], in["lightColor"][1], in["lightColor"][2], in["lightColor"][3] };
		}
		if (in.contains("intensity")) {
			intensity_ = in["intensity"];
		}
	}
};
