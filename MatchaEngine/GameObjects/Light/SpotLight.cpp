#include "SpotLight.h"
#include "Graphics/Render/Draw.h"
#include "Math/Calculation.h"
#include "../../../Editer/EditorManager.h"
#include <cmath>

SpotLight::SpotLight()
{
	name_ = "Spot Light";
}

void SpotLight::ImGui(bool drawTransform)
{
#ifdef _USE_IMGUI
	GameObject::ImGui(drawTransform);

	if (ImGui::CollapsingHeader(LanguageManager::Tr("Light Settings"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorEdit4(LanguageManager::Tr("Color"), &color_.x);
		ImGui::DragFloat(LanguageManager::Tr("Intensity"), &intensity_, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat(LanguageManager::Tr("Distance"), &distance_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat(LanguageManager::Tr("Decay"), &decay_, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat(LanguageManager::Tr("Angle"), &cosAngle_, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat(LanguageManager::Tr("Falloff"), &cosFalloffStart_, 0.01f, 0.0f, 1.0f);
	}
#endif
}

void SpotLight::Draw(class Draw& draw)
{
#ifdef _USE_IMGUI
	if (!EditorManager::IsPlaying() && draw.GetLineRenderer() && GameObject::selectedObject_ == this) {
		Vector3 start = transform_.translate;
		Matrix4x4 rot = Rotation(transform_.rotate);
		Vector3 baseDir = { 0.0f, -1.0f, 0.0f };
		Vector3 dir = Normalize(TransformMatrix(baseDir, rot));
		Vector3 end = AddVector3(start, ScalarMultiply(dir, distance_));
		
		Vector4 col = color_;
		
		// Center direction line
		draw.GetLineRenderer()->AddLine(start, end, col);
		
		// Calculate outer ring radius
		float angle = acosf(cosAngle_);
		float r = distance_ * tanf(angle);
		
		Vector3 right = Normalize(Cross(dir, { 0.0f, 1.0f, 0.0f }));
		if (Length(right) < 0.01f) {
			right = Normalize(Cross(dir, { 1.0f, 0.0f, 0.0f }));
		}
		Vector3 up = Normalize(Cross(right, dir));
		
		// Boundary lines of the cone
		Vector3 coneEdge1 = AddVector3(end, ScalarMultiply(right, r));
		Vector3 coneEdge2 = AddVector3(end, ScalarMultiply(right, -r));
		Vector3 coneEdge3 = AddVector3(end, ScalarMultiply(up, r));
		Vector3 coneEdge4 = AddVector3(end, ScalarMultiply(up, -r));
		
		draw.GetLineRenderer()->AddLine(start, coneEdge1, col);
		draw.GetLineRenderer()->AddLine(start, coneEdge2, col);
		draw.GetLineRenderer()->AddLine(start, coneEdge3, col);
		draw.GetLineRenderer()->AddLine(start, coneEdge4, col);
		
		// Outer circle representation
		const int segments = 16;
		for (int i = 0; i < segments; ++i) {
			float theta1 = (float)i * 2.0f * 3.14159265f / (float)segments;
			float theta2 = (float)(i + 1) * 2.0f * 3.14159265f / (float)segments;
			
			Vector3 p1 = AddVector3(end, AddVector3(ScalarMultiply(right, r * cosf(theta1)), ScalarMultiply(up, r * sinf(theta1))));
			Vector3 p2 = AddVector3(end, AddVector3(ScalarMultiply(right, r * cosf(theta2)), ScalarMultiply(up, r * sinf(theta2))));
			
			draw.GetLineRenderer()->AddLine(p1, p2, col);
		}
	}
#endif
}