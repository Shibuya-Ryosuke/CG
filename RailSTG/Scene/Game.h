#pragma once
#include <memory>
#include <cstdint>

namespace RyoEngine {
	class Camera;
}

class Player;
class BaseEnemy;
class Mob;
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

	void CheckAllCollision();

private:
	float kMobSpawnTimer_ = 3.0f;
private:
	// プレイヤー
	std::unique_ptr<Player> player_ = nullptr;
	// レティクル
	std::unique_ptr<Reticle> reticle_ = nullptr;
	// モブ
	std::vector<Mob*> mobs_;
	// 敵全体
	std::vector<std::unique_ptr<BaseEnemy>> enemies_;
	// スポーン時間
	float mobSpawnTimer_ = kMobSpawnTimer_;

};