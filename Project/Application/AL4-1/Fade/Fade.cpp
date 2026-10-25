#include "Fade.h"
#include "../GameResources/GameTex.h"
#include <algorithm>

using namespace RyoEngine;

namespace AL4_1 {

	// 0～1 をなめらかに補間する(端でゆっくり)
	float Fade::SmoothStep01(float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	// 初期化(画面を覆うスプライトを作る)
	void Fade::Initialize() {
		sprite_ = std::make_unique<Sprite>();
		sprite_->Initialize(GameImage::GetImage(GameImage::Image::FadeBlack),
			{ screenSize_.x * 0.5f, screenSize_.y * 0.5f }, Anchor::Center);

		state_ = State::None;
		alpha_ = 0.0f;
		t_ = 0.0f;
		ApplyAlpha();
	}

	// 更新
	void Fade::Update() {
		if (!IsFading()) return;

		t_ += GetDeltaTime() / (std::max)(duration_, 0.001f);
		const float e = SmoothStep01(t_);

		if (state_ == State::FadeOut) {
			alpha_ = e;
			if (t_ >= 1.0f) {
				alpha_ = 1.0f;
				state_ = State::Black;  // StartFadeIn が呼ばれるまで真っ黒のまま
			}
		} else {
			alpha_ = 1.0f - e;
			if (t_ >= 1.0f) {
				alpha_ = 0.0f;
				state_ = State::None;
			}
		}
		ApplyAlpha();
	}

	// 描画
	void Fade::Draw() {
		// 透明なときは描かない
		if (!sprite_ || alpha_ <= 0.0f) return;
		sprite_->Draw();
	}

	// 終了処理
	void Fade::Finalize() {
		if (sprite_) {
			sprite_->Finalize();
			sprite_.reset();
		}
	}

	// フェードアウト開始(透明 → 真っ黒)
	void Fade::StartFadeOut(float duration) {
		state_ = State::FadeOut;
		duration_ = duration;
		// 途中から始めても飛ばないよう、今の不透明度の位置から進める
		t_ = alpha_;
		if (duration_ <= 0.0f) {
			t_ = 1.0f;
			alpha_ = 1.0f;
			state_ = State::Black;
		}
		ApplyAlpha();
	}

	// フェードイン開始(真っ黒 → 透明)
	void Fade::StartFadeIn(float duration) {
		state_ = State::FadeIn;
		duration_ = duration;
		// 途中から始めても飛ばないよう、今の不透明度の位置から進める
		t_ = 1.0f - alpha_;
		if (duration_ <= 0.0f) {
			t_ = 1.0f;
			alpha_ = 0.0f;
			state_ = State::None;
		}
		ApplyAlpha();
	}

	// 不透明度をスプライトへ反映(画像はRGBA=1,1,1,1で真っ黒なので、アルファだけ変える)
	void Fade::ApplyAlpha() {
		if (!sprite_) return;
		sprite_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	}

}
