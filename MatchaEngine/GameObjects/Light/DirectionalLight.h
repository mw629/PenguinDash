#pragma once
#include "../Object/GameObject.h"

class DirectionalLight : public GameObject
{
public:
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	float intensity_ = 1.0f;

	DirectionalLight();
	void ImGui(bool drawTransform = true) override;
	void Draw(class Draw& draw) override;
};
