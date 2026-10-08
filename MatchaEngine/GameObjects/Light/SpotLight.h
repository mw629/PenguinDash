#pragma once
#include "../Object/GameObject.h"
#include <cmath>
#include <numbers>

class SpotLight : public GameObject
{
public:
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	float intensity_ = 7.0f;
	float distance_ = 4.0f;
	float decay_ = 2.0f;
	float cosAngle_ = std::cos(std::numbers::pi_v<float> / 3.0f);
	float cosFalloffStart_ = std::cos(std::numbers::pi_v<float> / 3.0f) + 0.1f;

	SpotLight();
	void ImGui(bool drawTransform = true) override;
	void Draw(class Draw& draw) override;
};