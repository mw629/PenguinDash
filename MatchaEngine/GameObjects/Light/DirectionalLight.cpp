#include "DirectionalLight.h"
#include "LightManager.h"
#include "Graphics/Render/Draw.h"
#include "Math/Calculation.h"
#include "../../../Editer/EditorManager.h"

DirectionalLight::DirectionalLight()
{
	name_ = "Directional Light";
}

std::shared_ptr<GameObject> DirectionalLight::Clone() const
{
	auto copy = std::make_shared<DirectionalLight>();
	copy->SetName(name_);
	copy->SetTransform(transform_);
	copy->SetIsActive(isActive_);
	copy->SetIsLocked(isLocked_);
	copy->SetColor(color_);
	copy->SetIntensity(intensity_);
	return copy;
}

void DirectionalLight::ApplyToLightManager(LightManager& manager)
{
	Matrix4x4 rot = Rotation(transform_.rotate);
	Vector3 dir = Normalize(TransformMatrix(kDefaultDirection, rot));
	manager.AddDirectionalLight(color_, dir, intensity_);
}

void DirectionalLight::ImGui(bool drawTransform)
{
#ifdef _USE_IMGUI
	GameObject::ImGui(drawTransform);

	if (ImGui::CollapsingHeader(LanguageManager::Tr("Light Settings"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorEdit4(LanguageManager::Tr("Color"), &color_.x);
		ImGui::DragFloat(LanguageManager::Tr("Intensity"), &intensity_, kIntensityDragSpeed, kIntensityMin, kIntensityMax);
	}
#endif
}

void DirectionalLight::Draw(class Draw& draw)
{
#ifdef _USE_IMGUI
	if (!EditorManager::IsPlaying() && draw.GetLineRenderer() && GameObject::GetSelectedObject() == this) {
		Vector3 start = transform_.translate;
		Matrix4x4 rot = Rotation(transform_.rotate);
		Vector3 dir = Normalize(TransformMatrix(kDefaultDirection, rot));
		Vector3 end = AddVector3(start, ScalarMultiply(dir, kGizmoLineLength));
		
		// Main direction line (yellow)
		draw.GetLineRenderer()->AddLine(start, end, color_);
		
		// Draw a small arrow head
		Vector3 right = Normalize(Cross(dir, kUpReference));
		if (Length(right) < kGizmoParallelThreshold) {
			right = Normalize(Cross(dir, kRightReference));
		}
		Vector3 up = Normalize(Cross(right, dir));
		
		Vector3 backDir = ScalarMultiply(dir, -1.0f);
		Vector3 arrowSide1 = AddVector3(end, ScalarMultiply(Normalize(AddVector3(backDir, ScalarMultiply(right, kGizmoArrowWidth))), kGizmoArrowLength));
		Vector3 arrowSide2 = AddVector3(end, ScalarMultiply(Normalize(AddVector3(backDir, ScalarMultiply(right, -kGizmoArrowWidth))), kGizmoArrowLength));
		Vector3 arrowSide3 = AddVector3(end, ScalarMultiply(Normalize(AddVector3(backDir, ScalarMultiply(up, kGizmoArrowWidth))), kGizmoArrowLength));
		Vector3 arrowSide4 = AddVector3(end, ScalarMultiply(Normalize(AddVector3(backDir, ScalarMultiply(up, -kGizmoArrowWidth))), kGizmoArrowLength));
		
		draw.GetLineRenderer()->AddLine(end, arrowSide1, color_);
		draw.GetLineRenderer()->AddLine(end, arrowSide2, color_);
		draw.GetLineRenderer()->AddLine(end, arrowSide3, color_);
		draw.GetLineRenderer()->AddLine(end, arrowSide4, color_);
	}
#endif
}
