#include "ColliderComponent.h"
#include "../Object/GameObject.h"
#include "../../Graphics/Render/Draw.h"
#include "../Object/3d/Cube.h"
#include "../Object/3d/Sphere.h"
#include "../../Game/GameScene/Entity/Collision.h"
#include <imgui.h>

bool ColliderComponent::s_isDrawDebug_ = false;

// ==========================================
// Base ColliderComponent
// ==========================================
ColliderComponent::ColliderComponent()
{
}

AABB ColliderComponent::GetWorldAABB() const
{
	AABB aabb;
	Vector3 centerPos = offset_;
	if (gameObject_) {
		const Transform& t = gameObject_->GetTransform();
		centerPos.x += t.translate.x;
		centerPos.y += t.translate.y;
		centerPos.z += t.translate.z;
	}
	aabb.min = { centerPos.x - 0.5f, centerPos.y - 0.5f, centerPos.z - 0.5f };
	aabb.max = { centerPos.x + 0.5f, centerPos.y + 0.5f, centerPos.z + 0.5f };
	return aabb;
}

BoundingSphere ColliderComponent::GetWorldSphere() const
{
	BoundingSphere sphere;
	sphere.radius = 0.5f;
	sphere.center = offset_;
	if (gameObject_) {
		const Transform& t = gameObject_->GetTransform();
		sphere.center.x += t.translate.x;
		sphere.center.y += t.translate.y;
		sphere.center.z += t.translate.z;
	}
	return sphere;
}

bool ColliderComponent::IntersectsWith(const ColliderComponent* other) const
{
	if (!other) return false;
	return other->IntersectsBox(reinterpret_cast<const BoxColliderComponent*>(this));
}

bool ColliderComponent::IntersectsBox(const BoxColliderComponent* other) const
{
	if (!other) return false;
	return Collision::CheckAABB(GetWorldAABB(), other->GetWorldAABB());
}

bool ColliderComponent::IntersectsSphere(const SphereColliderComponent* other) const
{
	if (!other) return false;
	return Collision::CheckAABBSphere(GetWorldAABB(), other->GetWorldSphere());
}

// ==========================================
// BoxColliderComponent
// ==========================================
BoxColliderComponent::BoxColliderComponent()
{
}

void BoxColliderComponent::Initialize()
{
	ColliderComponent::Initialize();

#ifdef _DEBUG
	Texture tex;
	int whiteTex = tex.CreateTexture("Resources/Texture/white64x64.png");

	debugCube_ = std::make_shared<Cube>();
	debugCube_->Initialize(whiteTex);
	if (auto mat = debugCube_->GetComponent<MaterialComponent>()) {
		mat->SetShader("WireFrameShader");
		mat->GetMaterialFactory()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
	}
#endif
}

void BoxColliderComponent::Update(Matrix4x4 view, float speedMultiplier)
{
	(void)speedMultiplier;
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
#endif
}

void BoxColliderComponent::Draw(class Draw& draw)
{
#ifdef _DEBUG
	if (!s_isDrawDebug_) return;
	if (debugCube_) {
		draw.DrawObj(debugCube_.get());
	}
#endif
}

#ifdef _DEBUG
void BoxColliderComponent::ImGui()
{
	if (ImGui::TreeNode("BoxColliderComponent")) {
		ImGui::DragFloat3("Offset", &offset_.x, 0.01f);
		ImGui::DragFloat3("Size (W,H,D)", &size_.x, 0.01f);
		ImGui::TreePop();
	}
}
#endif

AABB BoxColliderComponent::GetWorldAABB() const
{
	AABB aabb;
	Vector3 centerPos = offset_;
	if (gameObject_) {
		const Transform& t = gameObject_->GetTransform();
		centerPos.x += t.translate.x;
		centerPos.y += t.translate.y;
		centerPos.z += t.translate.z;
	}

	aabb.min.x = centerPos.x - (size_.x / 2.0f);
	aabb.max.x = centerPos.x + (size_.x / 2.0f);
	aabb.min.y = centerPos.y - (size_.y / 2.0f);
	aabb.max.y = centerPos.y + (size_.y / 2.0f);
	aabb.min.z = centerPos.z - (size_.z / 2.0f);
	aabb.max.z = centerPos.z + (size_.z / 2.0f);
	return aabb;
}

BoundingSphere BoxColliderComponent::GetWorldSphere() const
{
	BoundingSphere sphere;
	sphere.radius = (std::max)({ size_.x, size_.y, size_.z }) * 0.5f;
	sphere.center = offset_;
	if (gameObject_) {
		const Transform& t = gameObject_->GetTransform();
		sphere.center.x += t.translate.x;
		sphere.center.y += t.translate.y;
		sphere.center.z += t.translate.z;
	}
	return sphere;
}

bool BoxColliderComponent::IntersectsWith(const ColliderComponent* other) const
{
	if (!other) return false;
	return other->IntersectsBox(this);
}

bool BoxColliderComponent::IntersectsBox(const BoxColliderComponent* other) const
{
	if (!other) return false;
	return Collision::CheckAABB(GetWorldAABB(), other->GetWorldAABB());
}

bool BoxColliderComponent::IntersectsSphere(const SphereColliderComponent* other) const
{
	if (!other) return false;
	return Collision::CheckAABBSphere(GetWorldAABB(), other->GetWorldSphere());
}

// ==========================================
// SphereColliderComponent
// ==========================================
SphereColliderComponent::SphereColliderComponent()
{
}

void SphereColliderComponent::Initialize()
{
	ColliderComponent::Initialize();

#ifdef _DEBUG
	Texture tex;
	int whiteTex = tex.CreateTexture("Resources/Texture/white64x64.png");

	debugSphere_ = std::make_shared<Sphere>();
	debugSphere_->Initialize(whiteTex);
	if (auto mat = debugSphere_->GetComponent<MaterialComponent>()) {
		mat->SetShader("WireFrameShader");
		mat->GetMaterialFactory()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
	}
#endif
}

void SphereColliderComponent::Update(Matrix4x4 view, float speedMultiplier)
{
	(void)speedMultiplier;
#ifdef _DEBUG
	if (debugSphere_) {
		Transform t = {};
		if (gameObject_) t = gameObject_->GetTransform();
		
		t.translate.x += offset_.x;
		t.translate.y += offset_.y;
		t.translate.z += offset_.z;

		t.scale = { radius_ * 2.0f, radius_ * 2.0f, radius_ * 2.0f };
		
		debugSphere_->SetTransform(t);
		debugSphere_->SettingWvp(view);
	}
#endif
}

void SphereColliderComponent::Draw(class Draw& draw)
{
#ifdef _DEBUG
	if (!s_isDrawDebug_) return;
	if (debugSphere_) {
		draw.DrawObj(debugSphere_.get());
	}
#endif
}

#ifdef _DEBUG
void SphereColliderComponent::ImGui()
{
	if (ImGui::TreeNode("SphereColliderComponent")) {
		ImGui::DragFloat3("Offset", &offset_.x, 0.01f);
		ImGui::DragFloat("Radius", &radius_, 0.01f);
		ImGui::TreePop();
	}
}
#endif

AABB SphereColliderComponent::GetWorldAABB() const
{
	AABB aabb;
	Vector3 centerPos = offset_;
	if (gameObject_) {
		const Transform& t = gameObject_->GetTransform();
		centerPos.x += t.translate.x;
		centerPos.y += t.translate.y;
		centerPos.z += t.translate.z;
	}

	aabb.min.x = centerPos.x - radius_;
	aabb.max.x = centerPos.x + radius_;
	aabb.min.y = centerPos.y - radius_;
	aabb.max.y = centerPos.y + radius_;
	aabb.min.z = centerPos.z - radius_;
	aabb.max.z = centerPos.z + radius_;
	return aabb;
}

BoundingSphere SphereColliderComponent::GetWorldSphere() const
{
	BoundingSphere sphere;
	sphere.radius = radius_;
	sphere.center = offset_;
	if (gameObject_) {
		const Transform& t = gameObject_->GetTransform();
		sphere.center.x += t.translate.x;
		sphere.center.y += t.translate.y;
		sphere.center.z += t.translate.z;
	}
	return sphere;
}

bool SphereColliderComponent::IntersectsWith(const ColliderComponent* other) const
{
	if (!other) return false;
	return other->IntersectsSphere(this);
}

bool SphereColliderComponent::IntersectsBox(const BoxColliderComponent* other) const
{
	if (!other) return false;
	return Collision::CheckAABBSphere(other->GetWorldAABB(), GetWorldSphere());
}

bool SphereColliderComponent::IntersectsSphere(const SphereColliderComponent* other) const
{
	if (!other) return false;
	return Collision::CheckSphere(GetWorldSphere(), other->GetWorldSphere());
}
