#include "PointLight.h"
#include "Graphics/Render/Draw.h"
#include "Math/Calculation.h"
#include "../../../Editer/EditorManager.h"
#include <cmath>

PointLight::PointLight()
{
	name_ = "Point Light";
}

void PointLight::ImGui(bool drawTransform)
{
#ifdef _USE_IMGUI
	GameObject::ImGui(drawTransform);

	if (ImGui::CollapsingHeader(LanguageManager::Tr("Light Settings"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorEdit4(LanguageManager::Tr("Color"), &color_.x);
		ImGui::DragFloat(LanguageManager::Tr("Intensity"), &intensity_, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat(LanguageManager::Tr("Radius"), &radius_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat(LanguageManager::Tr("Decay"), &decay_, 0.1f, 0.0f, 10.0f);
	}
#endif
}

void PointLight::Draw(class Draw& draw)
{
#ifdef _USE_IMGUI
	if (!EditorManager::IsPlaying() && draw.GetLineRenderer() && GameObject::selectedObject_ == this) {
		Vector3 center = transform_.translate;
		Vector4 col = color_;
		float r = radius_;
		
		// Draw 3 orthogonal crosses representing the sphere radius
		draw.GetLineRenderer()->AddLine({ center.x - r, center.y, center.z }, { center.x + r, center.y, center.z }, col);
		draw.GetLineRenderer()->AddLine({ center.x, center.y - r, center.z }, { center.x, center.y + r, center.z }, col);
		draw.GetLineRenderer()->AddLine({ center.x, center.y, center.z - r }, { center.x, center.y, center.z + r }, col);

		// Circle approximations in 3 planes
		const int segments = 16;
		for (int i = 0; i < segments; ++i) {
			float theta1 = (float)i * 2.0f * 3.14159265f / (float)segments;
			float theta2 = (float)(i + 1) * 2.0f * 3.14159265f / (float)segments;
			
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