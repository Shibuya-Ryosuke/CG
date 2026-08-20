#include "SceneFade.h"
#include "../../../Original/RyoEngine.h"

using namespace RyoEngine;

void SceneFade::StartFadeIn(int32_t durationFrames) {
	state_ = FadeState::FadingIn;
	frameCount_ = 0;
	durationFrames_ = (durationFrames > 0) ? durationFrames : 1;
	alpha_ = 1.0f;
}

void SceneFade::StartFadeOut(int32_t durationFrames) {
	state_ = FadeState::FadingOut;
	frameCount_ = 0;
	durationFrames_ = (durationFrames > 0) ? durationFrames : 1;
	alpha_ = 0.0f;
}

void SceneFade::Update() {
	justFinishedFadeOut_ = false;

	if (state_ == FadeState::None) {
		return;
	}

	frameCount_++; // deltaTimeを使わず、TimeStateに影響されないフレームカウントで進める
	float t = static_cast<float>(frameCount_) / static_cast<float>(durationFrames_);
	if (t > 1.0f) t = 1.0f;

	if (state_ == FadeState::FadingIn) {
		alpha_ = EaseInQuad(t, 1.0f, 0.0f);
	} else {
		alpha_ = EaseOutQuad(t, 0.0f, 1.0f);
	}

	if (t >= 1.0f) {
		bool wasFadingOut = (state_ == FadeState::FadingOut);
		state_ = FadeState::None;
		justFinishedFadeOut_ = wasFadingOut;
	}
}

void SceneFade::Draw() const {
	if (alpha_ <= 0.0001f) {
		return;
	}
	PrimitiveRenderer::DrawRect2D(
		{ 640.0f, 360.0f },
		{ 1280.0f, 720.0f },
		0.0f,
		{ 0.0f, 0.0f, 0.0f, alpha_ },
		PrimitiveDrawMode::Fill
	);
}