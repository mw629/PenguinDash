#pragma once
#include <vector>
#include <memory>

class ColliderComponent;
class GameObjectManager;

class CollisionManager
{
public:
	CollisionManager() = default;
	~CollisionManager() = default;

	// 全コライダーの当たり判定をチェック
	void UpdateCollisions(GameObjectManager* manager);
};
