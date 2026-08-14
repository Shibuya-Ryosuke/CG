#pragma once
#include "TimeEnum.h"
#include <cstdint>

class TimeManager {
public:
	TimeManager() = default;
	~TimeManager() = default;
	TimeManager(const TimeManager&) = delete;
	TimeManager& operator=(const TimeManager&) = delete;

	/// <summary>
	/// インスタンスの取得
	/// </summary>
	/// <returns>instance</returns>
	static TimeManager& GetInstance() {
		static TimeManager instance;
		return instance;
	}

	/// <summary>
	/// 初期化
	/// </summary>
	static void Initialize();

	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	/// <summary>
	/// 更新
	/// </summary>
	static void Update();

	/// <summary>
	/// ゲーム全体で流れる時間の速さを取得
	/// </summary>
	/// <returns>timeScale_</returns>
	static float GetTimeScale() {
		return GetInstance().timeScale_;
	}

	static float GetDeltaTime() {
		return GetInstance().deltaTime_;
	}
	static void SetTimeState(TimeState state) { GetInstance().request_ = state; }
private:

	// 初期倍率
	float kInitTimeScale = 1.0f;
	
	// ジャスト回避スロー時間
	int32_t kJustEvasionTime = 30;
	
private:
	// 時間に掛ける倍率（0 ~ 1）
	float timeScale_ = kInitTimeScale;
	// deltaタイム
	float deltaTime_ = 0.0f;

	// ジャスト回避スロー時間
	int32_t justEvasionTime = kJustEvasionTime;

	// 状態
	TimeState state_ = TimeState::Default;
	TimeState request_ = TimeState::None;
};