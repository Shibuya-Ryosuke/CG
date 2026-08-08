#pragma once
#include "../../../Original/RyoEngine.h"

#include "../BaseBullet.h"

class PlayerBullet : public BaseBullet {
public:
	PlayerBullet();
	~PlayerBullet() override = default;
	// コピーコンストラクタと代入演算子の明示的削除
	PlayerBullet(const PlayerBullet&) = delete;
	PlayerBullet& operator=(const PlayerBullet&) = delete;

	void Initialize() override;
	void Finalize() override;

	void Update(const RyoEngine::Camera& camera) override;
	void Draw() override;

private:

};