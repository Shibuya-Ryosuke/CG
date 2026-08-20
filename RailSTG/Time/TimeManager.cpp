#include "../../Original/RyoEngine.h"

#include "TimeEnum.h"
#include "TimeManager.h"

void TimeManager::Initialize()
{}

void TimeManager::Finalize()
{}

void TimeManager::Update() {
	// リクエストがNone以外なら変更
	if (GetInstance().request_ != TimeState::None) {
		GetInstance().state_ = GetInstance().request_;
		GetInstance().request_ = TimeState::None;
	}

	switch (GetInstance().state_) {
	case TimeState::None:
		break;

	case TimeState::Default:
		GetInstance().timeScale_ = 1.0f;
		break;

	case TimeState::JustEvasion:
		// 余裕があれば
		// 本当は一気に0.2でタイマーが0に近づくごとに徐々に1.0に近づけるようなことをしたい
		GetInstance().timeScale_ = 0.2f;
		if (GetInstance().justEvasionDuration_ > 0) {
			GetInstance().justEvasionDuration_ -= GetInstance().deltaTime_;
		} else {
			GetInstance().request_ = TimeState::Default;
		}
		break;

	case TimeState::Targeting:
		GetInstance().timeScale_ = 0.025f;
		break;

	case TimeState::Ready:
		GetInstance().timeScale_ = 0.0f;
		break;
	}

	// ゲームで使うdeltaタイム
	GetInstance().deltaTime_ = RyoEngine::GetDeltaTime() * GetInstance().timeScale_;
}
