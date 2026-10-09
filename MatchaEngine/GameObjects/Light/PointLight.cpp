#include "PointLight.h"
#include "LightManager.h"
#include "Graphics/Render/Draw.h"
#include "Math/Calculation.h"
#include "../../../Editer/EditorManager.h"
#include <cmath>

PointLight::PointLight()
{
	name_ = "Point Light";
}

std::shared_ptr<GameObject> PointLight::Clone() const
{
	auto copy = std::make_shared<PointLight>();
	copy->SetName(name_);
	copy->SetTransform(transform_);
	copy->SetIsActive(isActive_);
	copy->SetIsLocked(isLocked_);
	copy->SetColor(color_);
	copy->SetIntensity(intensity_);
	copy->SetRadius(radius_);
	copy->SetDecay(decay_);
	return copy;
}

void PointLight::ApplyToLightManager(LightManager& manager)
{
	manager.AddPointLight(color_, transform_.translate, intensity_, radius_, decay_);
}

void PointLight::Serialize(nlohmann::json& out) const
{
	BaseLight::Serialize(out);
	out["radius"] = radius_;
	out["decay"] = decay_;
}

void PointLight::Deserialize(const nlohmann::json& in)
{
	BaseLight::Deserialize(in);
	if (in.contains("radius")) radius_ = in["radius"];
	if (in.contains("decay")) decay_ = in["decay"];
}

void PointLight::ImGui(bool drawTransform)
{
#ifdef _USE_IMGUI
	GameObject::ImGui(drawTransform);

	if (ImGui::CollapsingHeader(LanguageManager::Tr("Light Settings"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorEdit4(LanguageManager::Tr("Color"), &color_.x);
		ImGui::DragFloat(LanguageManager::Tr("Intensity"), &intensity_, kIntensityDragSpeed, kIntensityMin, kIntensityMax);
		ImGui::DragFloat(LanguageManager::Tr("Radius"), &radius_, kRadiusDragSpeed, kRadiusMin, kRadiusMax);
		ImGui::DragFloat(LanguageManager::Tr("Decay"), &decay_, kDecayDragSpeed, kDecayMin, kDecayMax);
	}
#endif
}

void PointLight::Draw(class Draw& draw)
{
#ifdef _USE_IMGUI
	if (!EditorManager::IsPlaying() && draw.GetLineRenderer() && GameObject::GetSelectedObject() == this) {
		Vector3 center = transform_.translate;
		Vector4 col = color_;
		float r = radius_;
		
		// Draw 3 orthogonal crosses representing the sphere radius
		draw.GetLineRenderer()->AddLine({ center.x - r, center.y, center.z }, { center.x + r, center.y, center.z }, col);
		draw.GetLineRenderer()->AddLine({ center.x, center.y - r, center.z }, { center.x, center.y + r, center.z }, col);
		draw.GetLineRenderer()->AddLine({ center.x, center.y, center.z - r }, { center.x, center.y, center.z + r }, col);

		// Circle approximations in 3 planes
		const int segments = kGizmoCircleSegments;
		for (int i = 0; i < segments; ++i) {
			float theta1 = (float)i * 2.0f * std::numbers::pi_v<float> / (float)segments;
			float theta2 = (float)(i + 1) * 2.0f * std::numbers::pi_v<float> / (float)segments;
			
			// XY plane
			draw.GetLineRenderer()->AddLine(
				{ center.x + r * cosf(theta1), center.y + r * sinf(theta1), center.z },
				{ center.x + r * cosf(theta2), center.y + r * sinf(theta2), center.z },
				col
			);
			// XZ plane
			draw.GetLineRenderer()->AddLine(
				{ center.x + r * cosf(theta1), center.y, center.z + r * sinf(theta1) },
				{ center.x + r * cosf(theta2), center.y, center.z + r * sinf(theta2) },
				col
			);
			// YZ plane
			draw.GetLineRenderer()->AddLine(
				{ center.x, center.y + r * cosf(theta1), center.z + r * sinf(theta1) },
				{ center.x, center.y + r * cosf(theta2), center.z + r * sinf(theta2) },
				col
			);
		}
	}
#endif
}