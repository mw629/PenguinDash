#pragma once
#include "../Object/GameObject.h"

class PointLight : public GameObject
{
private:
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	float intensity_ = 1.0f;
	float radius_ = 10.0f;
	float decay_ = 2.0f;

public:
	PointLight();
	void ImGui(bool drawTransform = true) override;
	void Draw(class Draw& draw) override;

	const Vector4& GetColor() const { return color_; }
	void SetColor(const Vector4& color) { color_ = color; }

	float GetIntensity() const { return intensity_; }
	void SetIntensity(float intensity) { intensity_ = intensity; }

	float GetRadius() const { return radius_; }
	void SetRadius(float radius) { radius_ = radius; }

	float GetDecay() const { return decay_; }
	void SetDecay(float decay) { decay_ = decay; }
};
