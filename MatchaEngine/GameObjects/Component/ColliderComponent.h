#pragma once
#include "Component.h"
#include <memory>
#include "Core/VariableTypes.h"

class Cube;
class Sphere;
class BoxColliderComponent;
class SphereColliderComponent;

enum class ColliderShape {
	Box,
	Sphere
};

class ColliderComponent : public Component
{
protected:
	static constexpr float kDefaultExtent = 0.5f;
	Vector3 offset_ = { 0.0f, 0.0f, 0.0f };
	static bool s_isDrawDebug_;

public:
	ColliderComponent();
	virtual ~ColliderComponent() override = default;

	static bool IsDrawDebug() { return s_isDrawDebug_; }
	static void SetDrawDebug(bool draw) { s_isDrawDebug_ = draw; }
	static void ToggleDrawDebug() { s_isDrawDebug_ = !s_isDrawDebug_; }

	const Vector3& GetOffset() const { return offset_; }
	void SetOffset(const Vector3& offset) { offset_ = offset; }

	virtual ColliderShape GetShape() const { return ColliderShape::Box; }
	virtual void SetShape(ColliderShape /*shape*/) {}
	virtual AABB GetWorldAABB() const;
	virtual BoundingSphere GetWorldSphere() const;

	virtual void SetSize(const Vector3& /*size*/) {}
	virtual void SetRadius(float /*radius*/) {}
	virtual const Vector3& GetSize() const { static Vector3 s{1,1,1}; return s; }
	virtual float GetRadius() const { return kDefaultExtent; }

	// Double Dispatch for collision check
	virtual bool IntersectsWith(const ColliderComponent* other) const;
	virtual bool IntersectsBox(const BoxColliderComponent* other) const;
	virtual bool IntersectsSphere(const SphereColliderComponent* other) const;
};

class BoxColliderComponent : public ColliderComponent
{
private:
	static constexpr float kHalfExtentFactor = 0.5f;
	Vector3 size_ = { 1.0f, 1.0f, 1.0f };
#ifdef _DEBUG
	std::shared_ptr<Cube> debugCube_;
#endif

public:
	BoxColliderComponent();
	~BoxColliderComponent() override = default;

	void Initialize() override;
	void Update(Matrix4x4 view, float speedMultiplier = 1.0f) override;
	void Draw(class Draw& draw) override;
#ifdef _DEBUG
	void ImGui() override;
#endif

	ColliderShape GetShape() const override { return ColliderShape::Box; }
	AABB GetWorldAABB() const override;
	BoundingSphere GetWorldSphere() const override;

	const Vector3& GetSize() const override { return size_; }
	void SetSize(const Vector3& size) override { size_ = size; }

	bool IntersectsWith(const ColliderComponent* other) const override;
	bool IntersectsBox(const BoxColliderComponent* other) const override;
	bool IntersectsSphere(const SphereColliderComponent* other) const override;
};

class SphereColliderComponent : public ColliderComponent
{
private:
	static constexpr float kDiameterMultiplier = 2.0f;
	float radius_ = kDefaultExtent;
#ifdef _DEBUG
	std::shared_ptr<Sphere> debugSphere_;
#endif

public:
	SphereColliderComponent();
	~SphereColliderComponent() override = default;

	void Initialize() override;
	void Update(Matrix4x4 view, float speedMultiplier = 1.0f) override;
	void Draw(class Draw& draw) override;
#ifdef _DEBUG
	void ImGui() override;
#endif

	ColliderShape GetShape() const override { return ColliderShape::Sphere; }
	AABB GetWorldAABB() const override;
	BoundingSphere GetWorldSphere() const override;

	float GetRadius() const override { return radius_; }
	void SetRadius(float radius) override { radius_ = radius; }

	bool IntersectsWith(const ColliderComponent* other) const override;
	bool IntersectsBox(const BoxColliderComponent* other) const override;
	bool IntersectsSphere(const SphereColliderComponent* other) const override;
};
