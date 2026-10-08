#include "CollisionManager.h"
#include "../GameObjects/Component/ColliderComponent.h"
#include "../GameObjects/Object/GameObject.h"
#include "../../Game/GameScene/Entity/Collision.h"
#include "../System/GameObjectManager.h"
#include <algorithm>
#include <vector>

void CollisionManager::UpdateCollisions(GameObjectManager* manager)
{
	if (!manager) return;

	std::vector<ColliderComponent*> colliders;
	for (const auto& obj : manager->GetObjects()) {
		if (obj && obj->GetIsActive()) {
			auto col = obj->GetComponent<ColliderComponent>();
			if (col) {
				colliders.push_back(col.get());
			}
		}
	}

	// 登録されている全コライダーで総当り判定
	// i と j で重複チェックを避けるため、j は i + 1 から始める
	for (size_t i = 0; i < colliders.size(); ++i) {
		for (size_t j = i + 1; j < colliders.size(); ++j) {
			ColliderComponent* colA = colliders[i];
			ColliderComponent* colB = colliders[j];

			// 親オブジェクトが非アクティブ等の場合はスキップ
			if (!colA->GetGameObject() || !colA->GetGameObject()->GetIsActive()) continue;
			if (!colB->GetGameObject() || !colB->GetGameObject()->GetIsActive()) continue;

			bool hit = colA->IntersectsWith(colB);

			if (hit) {
				// 衝突発生時、それぞれの親オブジェクトに通知
				colA->GetGameObject()->OnCollision(colB->GetGameObject());
				colB->GetGameObject()->OnCollision(colA->GetGameObject());
			}
		}
	}
}
