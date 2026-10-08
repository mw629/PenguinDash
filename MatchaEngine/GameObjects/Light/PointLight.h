#pragma once
#include "../Object/GameObject.h"

class PointLight : public GameObject
{
public:
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	float intensity_ = 1.0f;
	float radius_ = 10.0f;
	float decay_ = 2.0f;

	PointLight();
	void ImGui(bool drawTransform = true) override;
	void Draw(class Draw& draw) override;
};
