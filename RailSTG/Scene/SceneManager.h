#pragma once
#include "../../Original/RyoEngine.h"
#include <memory>
#include <vector>
#include <cstdint>

#include "SceneFade/SceneFade.h"
#include "SceneEnum.h"
#include "Game/PhaseRoute.h"
#include "../RailCamera/RailCameraController.h"

class Title;
class Game;
class Result;

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

	void PressSpaceFade();

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

	void RouteInitialize();
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
	Phase lastPhase_ = Phase::Ready;
	float kChangingDuration_ = 4.0f; // Changingの長さ(仮。演出時間に合わせて調整)

	// シーン切り替えのフェード処理
	SceneFade fade_;
	Scene pendingScene_ = Scene::None; // フェードアウト完了後に切り替える先のシーン
	static constexpr int32_t kFadeInDurationFrames_ = 90;  // 1秒 @60fps
	static constexpr int32_t kFadeOutDurationFrames_ = 90; // 1秒 @60fps

	// シーン別保持
	std::unique_ptr<Title> title_ = nullptr;
	std::unique_ptr<Game> game_ = nullptr;
	std::unique_ptr<Result> result_ = nullptr;

	// 天球
	std::unique_ptr<RyoEngine::Model> skydome_ = nullptr;
	// 地面
	std::unique_ptr<RyoEngine::Model> ground_ = nullptr;

	// pressSpace
	RyoEngine::Sprite pressSpace_;
	int32_t pressSpaceTimer_ = 0;
};
