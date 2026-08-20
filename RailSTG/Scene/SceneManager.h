#pragma once
#include "../../Original/RyoEngine.h"
#include <memory>
#include <vector>

#include "SceneEnum.h"
#include "Game/PhaseRoute.h"
#include "../RailCamera/RailCameraController.h"

class Game;

class SceneManager {
public:
	SceneManager();
	~SceneManager();
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Scene sceneState);

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

private:
	/// <summary>
	/// ゲームカメラとデバッグカメラの切り替え（テキスト表示も込み）
	/// </summary>
	void UpdateCamera();

private:
	// Phase(First~Third)をphaseRoutes_の添字に変換する
	static size_t PhaseToRouteIndex(Phase phase) {
		return static_cast<size_t>(phase) - static_cast<size_t>(Phase::First);
	}

	// フェーズ変化を検知するための比較用メソッド
	void CheckPhaseChange();

private:
	// カメラ(レールに沿って自動移動するゲーム用カメラ)
	std::unique_ptr<RailCameraController> camera_ = nullptr;
	std::unique_ptr<RyoEngine::DebugCamera> debugCamera_ = nullptr;
	RyoEngine::Camera* activeCamera_ = nullptr;

	// シーン（enum）
	Scene scene_ = Scene::None;
	// 各フェーズの経路と制限時間をまとめたもの
	std::vector<PhaseRoute> phaseRoutes_;
	// 前フレームまでに把握していたフェーズ(初期値はFirst。Initializeで既にFirstの経路をセット済みのため)
	Phase lastPhase_ = Phase::First;
	float kChangingDuration_ = 5.0f; // Changingの長さ(仮。演出時間に合わせて調整)

	// シーン別保持
	std::unique_ptr<Game> game_ = nullptr;

	// 天球
	std::unique_ptr<RyoEngine::Model> skydome_ = nullptr;
	// 地面
	std::unique_ptr<RyoEngine::Model> ground_ = nullptr;
};
