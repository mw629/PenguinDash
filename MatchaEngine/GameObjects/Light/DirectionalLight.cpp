#include "DirectionalLight.h"
#include "Graphics/Render/Draw.h"
#include "Math/Calculation.h"
#include "../../../Editer/EditorManager.h"

DirectionalLight::DirectionalLight()
{
	name_ = "Directional Light";
}

void DirectionalLight::ImGui(bool drawTransform)
{
#ifdef _USE_IMGUI
	GameObject::ImGui(drawTransform);

	if (ImGui::CollapsingHeader(LanguageManager::Tr("Light Settings"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorEdit4(LanguageManager::Tr("Color"), &color_.x);
		ImGui::DragFloat(LanguageManager::Tr("Intensity"), &intensity_, 0.01f, 0.0f, 10.0f);
	}
#endif
}

void DirectionalLight::Draw(class Draw& draw)
{
#ifdef _USE_IMGUI
	if (!EditorManager::IsPlaying() && draw.GetLineRenderer() && GameObject::selectedObject_ == this) {
		Vector3 start = transform_.translate;
		Matrix4x4 rot = Rotation(transform_.rotate);
		Vector3 baseDir = { 0.0f, -1.0f, 0.0f };
		Vector3 dir = Normalize(TransformMatrix(baseDir, rot));
		Vector3 end = AddVector3(start, ScalarMultiply(dir, 3.0f));
		
		// Main direction line (yellow)
		draw.GetLineRenderer()->AddLine(start, end, color_);
		
		// Draw a small arrow head
		Vector3 right = Normalize(Cross(dir, { 0.0f, 1.0f, 0.0f }));
		if (Length(right) < 0.01f) {
			right = Normalize(Cross(dir, { 1.0f, 0.0f, 0.0f }));
		}
		Vector3 up = Normalize(Cross(right, dir));
		
		Vector3 arrowSide1 = AddVector3(end, ScalarMultiply(Normalize(AddVector3(ScalarMultiply(dir, -1.0f), ScalarMultiply(right, 0.3f))), 0.5f));
		Vector3 arrowSide2 = AddVector3(end, ScalarMultiply(Normalize(AddVector3(ScalarMultiply(dir, -1.0f), ScalarMultiply(right, -0.3f))), 0.5f));
		Vector3 arrowSide3 = AddVector3(end, ScalarMultiply(Normalize(AddVector3(ScalarMultiply(dir, -1.0f), ScalarMultiply(up, 0.3f))), 0.5f));
		Vector3 arrowSide4 = AddVector3(end, ScalarMultiply(Normalize(AddVector3(ScalarMultiply(dir, -1.0f), ScalarMultiply(up, -0.3f))), 0.5f));
		
		draw.GetLineRenderer()->AddLine(end, arrowSide1, color_);
		draw.GetLineRenderer()->AddLine(end, arrowSide2, color_);
		draw.GetLineRenderer()->AddLine(end, arrowSide3, color_);
		draw.GetLineRenderer()->AddLine(end, arrowSide4, color_);
	}
#endif
}
