#pragma once
#include <memory>
#include <cstdint>
#include <vector>
#include <array>
#include "PhaseRoute.h"

namespace RyoEngine {
	class Camera;
}

class Player;
class BaseEnemy;
class Mob;
class HomingMob;
class Mine;
class ReticleGunner;
class Reticle;

class Game {
public:
	Game();
	~Game();
	Game(const Game&) = delete;
	Game& operator=(const Game&) = delete;

	/// <summary>
	/// 初期化(何度でも呼び直して1プレイ分の状態をリセットできる)
	/// </summary>
	void Initialize();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update(const RyoEngine::Camera& camera);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	void MobSpawn(const RyoEngine::Vector3 followoffset, const RyoEngine::Vector3 velocity, const RyoEngine::Camera& camera);
	void HomingMobSpawn(const RyoEngine::Vector3 followoffset, const RyoEngine::Vector3 velocity, const RyoEngine::Camera& camera);
	void MineSpawn(float randXMin, float randXMax, float randYMin, float randYMax, float randZMin, float randZMax, int32_t maxMines, const RyoEngine::Camera& camera);
	void ReticleGunnerSpawn(const RyoEngine::Vector3 followoffset, const RyoEngine::Vector3 velocity, const RyoEngine::Camera& camera);

	void FirstPhaseMoveEnemy();
	void SecondPhaseMoveEnemy();
	void ThirdPhaseMoveEnemy();

	/// <summary>
	/// ファーストフェーズの敵スポーン
	/// </summary>
	void FirstPhaseSpawn(const RyoEngine::Camera& camera);
	/// <summary>
	/// セカンドフェーズ敵スポーン
	/// </summary>
	void SecondPhaseSpawn(const RyoEngine::Camera& camera);
	/// <summary>
	/// サードフェーズの敵スポーン
	/// </summary>
	void ThirdPhaseSpawn(const RyoEngine::Camera& camera);

	void CheckAllCollision();

	/// <summary>
	/// enemyIdから現在生存している敵を検索
	/// 見つからなければnullptr
	/// </summary>
	BaseEnemy* FindEnemyById(int32_t enemyId) const;

	Phase GetPhase() const { return state_.phase; }

	void SetPhaseTimeLimits(const std::vector<float>& timeLimits) { phaseTimeLimits_ = timeLimits; }

	void AdvanceToNextPhase();
	void UpdatePhase(const RyoEngine::Camera& camera);

	void SetChangingDuration(float duration) { changingDuration_ = duration; }

	bool IsPlayerDead() const;

	void UpdateSprite();
	void DrawSprite();

	bool IsBackToTitle() const { return state_.isBackToTitle; }

private:
	// 変化しない設定値(RuntimeStateのデフォルト初期化から参照するためstatic constexprにしている)
	static constexpr float kMobSpawnTimer_ = 3.0f;
	static constexpr float kHomingMobSpawnTimer_ = 10.0f;

	static constexpr float kFirstSpawnInterval_ = 0.6f;
	static constexpr float kSecondSpawnInterval_ = 2.0f;
	static constexpr float kThirdSpawnInterval_ = 2.8f;

	static constexpr int32_t kFirstSpawnEnemies_ = 6;
	static constexpr int32_t kSecondSpawnEnemies_ = 12;
	static constexpr int32_t kThirdSpawnEnemies_ = 24;

	static constexpr int32_t kReadyFrames_ = 210;

private:
	// 1プレイ分でリセットしたい実行時状態をまとめたもの。
	// Initialize()で state_ = RuntimeState{}; とするだけで全部デフォルトに戻せる。
	struct RuntimeState {
		RuntimeState() = default;
		~RuntimeState() = default;
		RuntimeState(const RuntimeState&) = delete;
		RuntimeState& operator=(const RuntimeState&) = delete;
		RuntimeState(RuntimeState&&) = default;
		RuntimeState& operator=(RuntimeState&&) = default;

		std::unique_ptr<Player> player = nullptr;

		std::vector<Mob*> mobs;
		std::vector<HomingMob*> homingMobs;
		std::vector<Mine*> mines;
		std::vector<ReticleGunner*> reticleGunners;
		std::vector<std::unique_ptr<BaseEnemy>> enemies;

		float mobSpawnTimer = kMobSpawnTimer_;
		float homingMobSpawnTimer = kHomingMobSpawnTimer_;

		float enemySpawnTimer = kFirstSpawnInterval_;

		Phase phase = Phase::Ready;
		Phase nextPhase = Phase::First;
		float changingElapsedTime = 0.0f;

		int32_t readyFrameCount = 0;

		float phaseElapsedTime = 0.0f;

		bool isFirstSpawning = true;
		bool isSecondSpawning = false;
		bool isThirdSpawning = false;

		int32_t spawnEnemies = 0;
		int32_t totalSpawnEnemies = 0;
		int32_t totalDestroyEnemies = 0;

		RyoEngine::Vector3 spawnSpace{};

		bool isPause = false;
		bool isBackToGame = true;
		bool isBackToTitle = false;
	};

	RuntimeState state_;

	RyoEngine::Sprite phaseInfo_;
	std::array< RyoEngine::Sprite, 3> phases_;
	RyoEngine::Sprite ready_;
	RyoEngine::Sprite start_;
	RyoEngine::Sprite nextPhase_;
	RyoEngine::Sprite pauseButton_;
	RyoEngine::Sprite pauseBack_;
	RyoEngine::Sprite pause_;
	RyoEngine::Sprite triangle_;
	RyoEngine::Sprite end_;


	// SceneManagerから一度だけセットされ、リプレイ時も保持したい値(RuntimeStateには含めない)
	std::vector<float> phaseTimeLimits_;
	float changingDuration_ = 0.0f;
};