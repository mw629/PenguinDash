#pragma once
#include "../Object/GameObject.h"
#include <cmath>
#include <numbers>

class SpotLight : public GameObject
{
private:
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	float intensity_ = 7.0f;
	float distance_ = 4.0f;
	float decay_ = 2.0f;
	float cosAngle_ = std::cos(std::numbers::pi_v<float> / 3.0f);
	float cosFalloffStart_ = std::cos(std::numbers::pi_v<float> / 3.0f) + 0.1f;

public:
	SpotLight();
	void ImGui(bool drawTransform = true) override;
	void Draw(class Draw& draw) override;

	const Vector4& GetColor() const { return color_; }
	void SetColor(const Vector4& color) { color_ = color; }

	float GetIntensity() const { return intensity_; }
	void SetIntensity(float intensity) { intensity_ = intensity; }

	float GetDistance() const { return distance_; }
	void SetDistance(float distance) { distance_ = distance; }

	float GetDecay() const { return decay_; }
	void SetDecay(float decay) { decay_ = decay; }

	float GetCosAngle() const { return cosAngle_; }
	void SetCosAngle(float cosAngle) { cosAngle_ = cosAngle; }

	float GetCosFalloffStart() const { return cosFalloffStart_; }
	void SetCosFalloffStart(float falloff) { cosFalloffStart_ = falloff; }
};