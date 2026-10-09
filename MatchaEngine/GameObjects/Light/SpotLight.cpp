#include "SpotLight.h"
#include "LightManager.h"
#include "Graphics/Render/Draw.h"
#include "Math/Calculation.h"
#include "../../../Editer/EditorManager.h"
#include <cmath>

SpotLight::SpotLight()
{
	name_ = "Spot Light";
}

std::shared_ptr<GameObject> SpotLight::Clone() const
{
	auto copy = std::make_shared<SpotLight>();
	copy->SetName(name_);
	copy->SetTransform(transform_);
	copy->SetIsActive(isActive_);
	copy->SetIsLocked(isLocked_);
	copy->SetColor(color_);
	copy->SetIntensity(intensity_);
	copy->SetDistance(distance_);
	copy->SetDecay(decay_);
	copy->SetCosAngle(cosAngle_);
	copy->SetCosFalloffStart(cosFalloffStart_);
	return copy;
}

void SpotLight::ApplyToLightManager(LightManager& manager)
{
	Matrix4x4 rot = Rotation(transform_.rotate);
	Vector3 baseDir = kDefaultDirection;
	Vector3 dir = Normalize(TransformMatrix(baseDir, rot));
	manager.AddSpotLight(color_, transform_.translate, dir, intensity_, distance_, decay_, cosAngle_, cosFalloffStart_);
}

void SpotLight::Serialize(nlohmann::json& out) const
{
	BaseLight::Serialize(out);
	out["distance"] = distance_;
	out["decay"] = decay_;
	out["cosAngle"] = cosAngle_;
	out["cosFalloffStart"] = cosFalloffStart_;
}

void SpotLight::Deserialize(const nlohmann::json& in)
{
	BaseLight::Deserialize(in);
	if (in.contains("distance")) distance_ = in["distance"];
	if (in.contains("decay")) decay_ = in["decay"];
	if (in.contains("cosAngle")) cosAngle_ = in["cosAngle"];
	if (in.contains("cosFalloffStart")) cosFalloffStart_ = in["cosFalloffStart"];
}

void SpotLight::ImGui(bool drawTransform)
{
#ifdef _USE_IMGUI
	GameObject::ImGui(drawTransform);

	if (ImGui::CollapsingHeader(LanguageManager::Tr("Light Settings"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorEdit4(LanguageManager::Tr("Color"), &color_.x);
		ImGui::DragFloat(LanguageManager::Tr("Intensity"), &intensity_, kIntensityDragSpeed, kIntensityMin, kIntensityMax);
		ImGui::DragFloat(LanguageManager::Tr("Distance"), &distance_, kDistanceDragSpeed, kDistanceMin, kDistanceMax);
		ImGui::DragFloat(LanguageManager::Tr("Decay"), &decay_, kDecayDragSpeed, kDecayMin, kDecayMax);
		ImGui::DragFloat(LanguageManager::Tr("Angle"), &cosAngle_, kAngleDragSpeed, kAngleMin, kAngleMax);
		ImGui::DragFloat(LanguageManager::Tr("Falloff"), &cosFalloffStart_, kFalloffDragSpeed, kFalloffMin, kFalloffMax);
	}
#endif
}

void SpotLight::Draw(class Draw& draw)
{
#ifdef _USE_IMGUI
	if (!EditorManager::IsPlaying() && draw.GetLineRenderer() && GameObject::GetSelectedObject() == this) {
		Vector3 start = transform_.translate;
		Matrix4x4 rot = Rotation(transform_.rotate);
		Vector3 baseDir = kDefaultDirection;
		Vector3 dir = Normalize(TransformMatrix(baseDir, rot));
		Vector3 end = AddVector3(start, ScalarMultiply(dir, distance_));
		
		Vector4 col = color_;
		
		// Center direction line
		draw.GetLineRenderer()->AddLine(start, end, col);
		
		// Calculate outer ring radius
		float angle = acosf(cosAngle_);
		float r = distance_ * tanf(angle);
		
		Vector3 right = Normalize(Cross(dir, kUpReference));
		if (Length(right) < kGizmoParallelThreshold) {
			right = Normalize(Cross(dir, kRightReference));
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
		const int segments = kGizmoCircleSegments;
		for (int i = 0; i < segments; ++i) {
			float theta1 = (float)i * 2.0f * std::numbers::pi_v<float> / (float)segments;
			float theta2 = (float)(i + 1) * 2.0f * std::numbers::pi_v<float> / (float)segments;
			
			Vector3 p1 = AddVector3(end, AddVector3(ScalarMultiply(right, r * cosf(theta1)), ScalarMultiply(up, r * sinf(theta1))));
			Vector3 p2 = AddVector3(end, AddVector3(ScalarMultiply(right, r * cosf(theta2)), ScalarMultiply(up, r * sinf(theta2))));
			
			draw.GetLineRenderer()->AddLine(p1, p2, col);
		}
	}
#endif
}