#pragma once
#include "../BaseBullet.h"

class EnemyBullet : public BaseBullet {
public:
	EnemyBullet();
	~EnemyBullet() override = default;
	// コピーコンストラクタと代入演算子の明示的削除
	EnemyBullet(const EnemyBullet&) = delete;
	EnemyBullet& operator=(const EnemyBullet&) = delete;

	void Initialize() override;
	void Finalize() override;

	void Update(const RyoEngine::Camera& camera) override;
	void Draw() override;

private:

};