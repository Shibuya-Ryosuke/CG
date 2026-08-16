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
		GetInstance().timeScale_ = 0.2f;
		if (GetInstance().justEvasionTime > 0) {
			GetInstance().justEvasionTime--;
		} else {
			GetInstance().justEvasionTime = GetInstance().kJustEvasionTime;
			GetInstance().request_ = TimeState::Default;
		}
		break;

	case TimeState::Targeting:
		GetInstance().timeScale_ = 0.025f;
		break;
	}

	// ゲームで使うdeltaタイム
	GetInstance().deltaTime_ = RyoEngine::GetDeltaTime() * GetInstance().timeScale_;
}
