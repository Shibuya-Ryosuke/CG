#pragma once
#include <cstdint>
#include "../../../Original/RyoEngine.h"

enum class FadeState {
	None,
	FadingIn,
	FadingOut,
};

class SceneFade {
public:
	/// <summary>
	/// フェードイン開始(黒→透明)
	/// </summary>
	/// <param name="durationFrames">フェードにかけるフレーム数(60fps基準。例: 1秒なら60)</param>
	void StartFadeIn(int32_t durationFrames);

	/// <summary>
	/// フェードアウト開始(透明→黒)
	/// </summary>
	void StartFadeOut(int32_t durationFrames);

	void Update();
	void Draw();

	bool IsIdle() const { return state_ == FadeState::None; }
	bool IsFadeOutJustFinished() const { return justFinishedFadeOut_; }
	float GetAlpha() const { return alpha_; }

private:
	FadeState state_ = FadeState::None;
	int32_t frameCount_ = 0;
	int32_t durationFrames_ = 90;
	float alpha_ = 1.0f;
	bool justFinishedFadeOut_ = false;

	RyoEngine::Sprite fade_;
};