#pragma once
#include "TimeEnum.h"
class TimeManager {
public:
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
	void Initialize();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// ゲーム全体で流れる時間の速さを取得
	/// </summary>
	/// <returns>timeSpeed_</returns>
	float GetTimeSpeed() const {
		return GetInstance().timeSpeed_;
	}

	void SetTimeState(TimeState state){}
private:
	TimeManager() = default;
	~TimeManager() = default;
	TimeManager(const TimeManager&) = delete;
	TimeManager& operator=(const TimeManager&) = delete;

	// 初期倍率
	float kInitTimeScale = 1.0f;

private:
	// 時間の速さ
	float timeSpeed_ = 0.0f;
	// 時間に掛ける倍率（0 ~ 1）
	float timeScale_ = 0.0f;

	// 状態
	TimeState state_ = TimeState::None;
	TimeState request_ = TimeState::None;
};