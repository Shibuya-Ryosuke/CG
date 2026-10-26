#include "BulletManager.h"

using namespace RyoEngine;

namespace AL4_1 {
	void BulletManager::Initialize(const std::string& modelPath, uint32_t maxCount) {
		model_.Initialize(modelPath, maxCount);

		bullets_.clear();
		bullets_.resize(maxCount);
		for (auto& bullet : bullets_) {
			bullet.Initialize(&model_);
		}
		model_.UpdateBuffer();
	}

	Bullet* BulletManager::Fire(const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity) {
		for (auto& bullet : bullets_) {
			if (!bullet.IsDead()) continue;
			bullet.Fire(position, velocity);
			return &bullet;
		}
		return nullptr;
	}

	void BulletManager::Update() {
		for (auto& bullet : bullets_) {
			if (bullet.IsDead()) continue;
			bullet.Update();

			// ボスの弾は壁や床で消える（反射弾はボスまで必ず届くように地形を無視する）
			if (!bullet.IsDead() && !bullet.IsReflected() && map_ && map_->IsSolidAt(bullet.GetPosition())) {
				bullet.OnCollision();
			}
		}
		model_.UpdateBuffer();
	}

	void BulletManager::Draw(RyoEngine::Camera& camera) {
		model_.Draw(camera);

#ifdef _DEBUG
		// デバッグ：反射弾の判定を水色で表示
		for (auto& bullet : bullets_) {
			if (bullet.IsDead() || !bullet.IsReflected()) continue;
			PrimitiveRenderer::DrawOBB(bullet.GetOBB(), { 0.3f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);
		}
#endif
	}

	void BulletManager::Clear() {
		for (auto& bullet : bullets_) {
			if (!bullet.IsDead()) bullet.OnCollision();
		}
		model_.UpdateBuffer();
	}
}
