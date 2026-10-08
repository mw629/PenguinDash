#pragma once
#include "../Object/GameObject.h"

class DirectionalLight : public GameObject
{
private:
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	float intensity_ = 1.0f;

public:
	DirectionalLight();
	void ImGui(bool drawTransform = true) override;
	void Draw(class Draw& draw) override;

	const Vector4& GetColor() const { return color_; }
	void SetColor(const Vector4& color) { color_ = color; }

	float GetIntensity() const { return intensity_; }
	void SetIntensity(float intensity) { intensity_ = intensity; }
};
