#pragma once
#include "../../../Original/RyoEngine.h"
#include <cstdint>
#include <array>

class Result {
public:
	Result() = default;
	~Result() = default;
	Result(const Result&) = delete;
	Result& operator=(const Result&) = delete;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	void SetIsClear(bool isClear) { isClear_ = isClear; }

	void SetInfo(int32_t spawnEnemies, int32_t destroyEnemies, int32_t justEvasionNumber, float remainingHP, float clearTime) {
		totalSpawnEnemies_ = spawnEnemies;
		totalDestroyEnemies_ = destroyEnemies;
		playerRemainingHP_ = remainingHP;
		playerJustEvasionNumber_ = justEvasionNumber;
		time_ = clearTime;
	}

private:
	RyoEngine::Sprite clear_;
	RyoEngine::Sprite failed_;

	RyoEngine::Sprite spawnEnemies_;
	RyoEngine::Sprite destroyEnemies_;
	RyoEngine::Sprite justEvasionNumber_;
	std::array<RyoEngine::Sprite, 4> remainingHp_;
	std::array<RyoEngine::Sprite, 4> clearTime_;

	std::array<uint32_t, 10> numbers_;

	int32_t totalSpawnEnemies_ = 0;
	int32_t totalDestroyEnemies_ = 0;
	float playerRemainingHP_ = 0.0f;
	int32_t playerJustEvasionNumber_ = 0;
	float time_ = 0.0f;

	bool isClear_ = true;
};