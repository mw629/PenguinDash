#include "ColliderComponent.h"
#include "../Object/GameObject.h"
#include "../../Graphics/Render/Draw.h"
#include "../Object/3d/Cube.h"
#include "../Object/3d/Sphere.h"
#include <imgui.h>

bool ColliderComponent::s_isDrawDebug_ = false;

ColliderComponent::ColliderComponent()
{
}

ColliderComponent::~ColliderComponent()
{
}

void ColliderComponent::Initialize()
{
	Component::Initialize();

#ifdef _DEBUG
	Texture tex;
	int whiteTex = tex.CreateTexture("Resources/Texture/white64x64.png");

	debugCube_ = std::make_shared<Cube>();
	debugCube_->Initialize(whiteTex);
	if (auto mat = debugCube_->GetComponent<MaterialComponent>()) {
		mat->SetShader("WireFrameShader");
		mat->GetMaterialFactory()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
	}

	debugSphere_ = std::make_shared<Sphere>();
	debugSphere_->Initialize(whiteTex);
	if (auto mat = debugSphere_->GetComponent<MaterialComponent>()) {
		mat->SetShader("WireFrameShader");
		mat->GetMaterialFactory()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
	}
#endif
}

void ColliderComponent::Update(Matrix4x4 view, float speedMultiplier)
{
#ifdef _DEBUG
	if (debugCube_) {
		Transform t = {};
		if (gameObject_) t = gameObject_->GetTransform();
		
		t.translate.x += offset_.x;
		t.translate.y += offset_.y;
		t.translate.z += offset_.z;
		
		t.scale = size_;
		
		debugCube_->SetTransform(t);
		debugCube_->SettingWvp(view);
	}
	if (debugSphere_) {
		Transform t = {};
		if (gameObject_) t = gameObject_->GetTransform();
		
		t.translate.x += offset_.x;
		t.translate.y += offset_.y;
		t.translate.z += offset_.z;

		// Sphere uses radius, so scale is 2 * radius to match diameter
		t.scale = { radius_ * 2.0f, radius_ * 2.0f, radius_ * 2.0f };
		
		debugSphere_->SetTransform(t);
		debugSphere_->SettingWvp(view);
	}
#endif
}

void ColliderComponent::Draw(class Draw& draw)
{
#ifdef _DEBUG
	if (!s_isDrawDebug_) return;

	if (shape_ == ColliderShape::Box && debugCube_) {
		draw.DrawObj(debugCube_.get());
	}
	else if (shape_ == ColliderShape::Sphere && debugSphere_) {
		draw.DrawObj(debugSphere_.get());
	}
#endif
}

#ifdef _DEBUG
void ColliderComponent::ImGui()
{
	if (ImGui::TreeNode("ColliderComponent")) {
		
		int currentShape = static_cast<int>(shape_);
		const char* shapeNames[] = { "Box", "Sphere" };
		if (ImGui::Combo("Shape", &currentShape, shapeNames, IM_ARRAYSIZE(shapeNames))) {
			shape_ = static_cast<ColliderShape>(currentShape);
		}

		ImGui::DragFloat3("Offset", &offset_.x, 0.01f);

		if (shape_ == ColliderShape::Box) {
			ImGui::DragFloat3("Size (W,H,D)", &size_.x, 0.01f);
		}
		else if (shape_ == ColliderShape::Sphere) {
			ImGui::DragFloat("Radius", &radius_, 0.01f);
		}

		ImGui::TreePop();
	}
}
#endif

AABB ColliderComponent::GetWorldAABB() const
{
	AABB aabb;
	if (gameObject_) {
		const Transform& t = gameObject_->GetTransform();
		Vector3 centerPos = {
			t.translate.x + offset_.x,
			t.translate.y + offset_.y,
			t.translate.z + offset_.z
		};

		aabb.min.x = centerPos.x - (size_.x / 2.0f);
		aabb.max.x = centerPos.x + (size_.x / 2.0f);

		// Boxコライダーの仕様として、Y軸は中心基準にするか足元基準にするか。
		// ここでは一般的な中心基準として計算する。
		aabb.min.y = centerPos.y - (size_.y / 2.0f);
		aabb.max.y = centerPos.y + (size_.y / 2.0f);

		aabb.min.z = centerPos.z - (size_.z / 2.0f);
		aabb.max.z = centerPos.z + (size_.z / 2.0f);
	}
	else {
		// fallback
		aabb.min = { -size_.x/2.0f, -size_.y/2.0f, -size_.z/2.0f };
		aabb.max = {  size_.x/2.0f,  size_.y/2.0f,  size_.z/2.0f };
	}
	return aabb;
}

BoundingSphere ColliderComponent::GetWorldSphere() const
{
	BoundingSphere sphere;
	sphere.radius = radius_;
	if (gameObject_) {
		const Transform& t = gameObject_->GetTransform();
		sphere.center = {
			t.translate.x + offset_.x,
			t.translate.y + offset_.y,
			t.translate.z + offset_.z
		};
	}
	else {
		sphere.center = offset_;
	}
	return sphere;
}
