#pragma once
#include "Component.h"
#include <memory>

class Cube;
class Sphere;

enum class ColliderShape {
	Box,
	Sphere
};

class ColliderComponent : public Component
{
private:
	ColliderShape shape_ = ColliderShape::Box;

	// 共通
	Vector3 offset_ = { 0.0f, 0.0f, 0.0f };

	// Box用
	Vector3 size_ = { 1.0f, 1.0f, 1.0f }; // 幅, 高さ, 奥行き

	// Sphere用
	float radius_ = 0.5f;

#ifdef _DEBUG
	std::shared_ptr<Cube> debugCube_;
	std::shared_ptr<Sphere> debugSphere_;
#endif

public:
	ColliderComponent();
	~ColliderComponent() override;

	void Initialize() override;
	void Update(Matrix4x4 view, float speedMultiplier = 1.0f) override;
	void Draw(class Draw& draw) override;
#ifdef _DEBUG
	void ImGui() override;
#endif

	static bool s_isDrawDebug_;

	ColliderShape GetShape() const { return shape_; }
	
	// ワールド座標系でのAABBを取得
	AABB GetWorldAABB() const;

	// ワールド座標系でのBoundingSphereを取得
	BoundingSphere GetWorldSphere() const;

	void SetShape(ColliderShape shape) { shape_ = shape; }
	void SetOffset(const Vector3& offset) { offset_ = offset; }
	void SetSize(const Vector3& size) { size_ = size; }
	void SetRadius(float radius) { radius_ = radius; }
};
