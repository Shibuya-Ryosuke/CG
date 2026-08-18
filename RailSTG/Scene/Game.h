#pragma once
#include <memory>
#include <cstdint>

namespace RyoEngine {
	class Camera;
}

class Player;
class BaseEnemy;
class Mob;
class HomingMob;
class Mine;
class Reticle;

class Game {
public:
	Game();
	~Game();
	Game(const Game&) = delete;
	Game& operator=(const Game&) = delete;

	/// <summary>
	/// 初期化
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

	void MobSpawn();
	void HomingMobSpawn();
	void MineSpawn();

	void CheckAllCollision();


	/// <summary>
	/// enemyIdから現在生存している敵を検索
	/// 見つからなければnullptr
	/// </summary>
	BaseEnemy* FindEnemyById(int32_t enemyId) const;

private:
	float kMobSpawnTimer_ = 3.0f;
	float kHomingMobSpawnTimer_ = 10.0f;

private:
	// プレイヤー
	std::unique_ptr<Player> player_ = nullptr;
	// レティクル
	std::unique_ptr<Reticle> reticle_ = nullptr;
	// モブ
	std::vector<Mob*> mobs_;
	// 追尾弾出す敵
	std::vector<HomingMob*> homingMobs_;
	// 機雷
	std::vector<Mine*> mines_;
	// 敵全体
	std::vector<std::unique_ptr<BaseEnemy>> enemies_;
	// スポーン時間
	float mobSpawnTimer_ = kMobSpawnTimer_;
	float homingMobSpawnTimer_ = kHomingMobSpawnTimer_;

};