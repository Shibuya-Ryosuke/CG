#pragma once
#include <memory>
#include <cstdint>

namespace RyoEngine {
	class Camera;
}

class Player;
class Mob;

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

private:
	int32_t kMobSpawnTimer_ = 300;
private:
	// プレイヤー
	std::unique_ptr<Player> player_ = nullptr;
	// モブ
	std::vector<std::unique_ptr<Mob>> mobs_;
	// スポーン時間
	int32_t mobSpawnTimer_ = kMobSpawnTimer_;

};