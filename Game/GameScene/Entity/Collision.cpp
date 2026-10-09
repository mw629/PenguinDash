#include "Collision.h"

bool Collision::CheckAABB(const AABB& a, const AABB& b)
{
	// 各軸で重なりがあるかチェック
	if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
	if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
	if (a.max.z < b.min.z || a.min.z > b.max.z) return false;

	return true;
}

AABB Collision::MakeAABB(const Transform& transform, float width, float height, float depth)
{
	AABB aabb;
	float halfW = width * kHalfExtentFactor;
	float halfH = height * kHalfExtentFactor;
	float halfD = depth * kHalfExtentFactor;
	aabb.min = {
		transform.translate.x - halfW,
		transform.translate.y - halfH,
		transform.translate.z - halfD
	};
	aabb.max = {
		transform.translate.x + halfW,
		transform.translate.y + halfH,
		transform.translate.z + halfD
	};
	return aabb;
}

bool Collision::CheckSphere(const BoundingSphere& a, const BoundingSphere& b)
{
	float dx = a.center.x - b.center.x;
	float dy = a.center.y - b.center.y;
	float dz = a.center.z - b.center.z;
	float distSquared = dx * dx + dy * dy + dz * dz;
	float radiusSum = a.radius + b.radius;
	return distSquared <= (radiusSum * radiusSum);
}

bool Collision::CheckAABBSphere(const AABB& aabb, const BoundingSphere& sphere)
{
	// AABB上の球の中心に最も近い点を見つける
	float closestX = (std::max)(aabb.min.x, (std::min)(sphere.center.x, aabb.max.x));
	float closestY = (std::max)(aabb.min.y, (std::min)(sphere.center.y, aabb.max.y));
	float closestZ = (std::max)(aabb.min.z, (std::min)(sphere.center.z, aabb.max.z));

	// その点と球の中心との距離を計算
	float dx = closestX - sphere.center.x;
	float dy = closestY - sphere.center.y;
	float dz = closestZ - sphere.center.z;

	float distSquared = dx * dx + dy * dy + dz * dz;
	return distSquared <= (sphere.radius * sphere.radius);
}
