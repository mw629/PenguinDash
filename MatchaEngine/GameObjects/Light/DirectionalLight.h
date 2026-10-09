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

private:
	static constexpr Vector3 kDefaultDirection = { 0.0f, -1.0f, 0.0f };
	static constexpr float kGizmoLineLength = 3.0f;
	static constexpr Vector3 kUpReference = { 0.0f, 1.0f, 0.0f };
	static constexpr Vector3 kRightReference = { 1.0f, 0.0f, 0.0f };
	static constexpr float kGizmoParallelThreshold = 0.01f;
	static constexpr float kGizmoArrowWidth = 0.3f;
	static constexpr float kGizmoArrowLength = 0.5f;

	static constexpr float kIntensityDragSpeed = 0.01f;
	static constexpr float kIntensityMin = 0.0f;
	static constexpr float kIntensityMax = 10.0f;
};
