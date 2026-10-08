#pragma once
#include "BaseLight.h"

class DirectionalLight : public BaseLight
{
public:
	DirectionalLight();
	std::string GetTypeName() const override { return "DirectionalLight"; }
	std::shared_ptr<GameObject> Clone() const override;

	void ImGui(bool drawTransform = true) override;
	void Draw(class Draw& draw) override;
	void ApplyToLightManager(class LightManager& manager) override;
};
