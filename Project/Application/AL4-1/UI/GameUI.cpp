#include "GameUI.h"
#include "../GameResources/GameTex.h"
#include <algorithm>

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	GameUI::GameUI() {
		// 初期配置（1280x720想定。ParamEditorで調整する）
		playerHp_.name = "PlayerHp";
		playerHp_.position = { 40.0f, 60.0f };

		bossHp_.name = "BossHp";
		bossHp_.position = { 740.0f, 60.0f };

		clearText_.name = "TextClear";
		failedText_.name = "TextFailed";
	}
	GameUI::~GameUI() {
		Finalize();
	}

	void GameUI::RegisterInParamEditor() {
		ParamEditor::BeginGroup("GameUI");
		RegisterHpBar(playerHp_);
		RegisterHpBar(bossHp_);
		RegisterText(clearText_);
		RegisterText(failedText_);
		ParamEditor::EndGroup();
	}

	void GameUI::RegisterHpBar(HpBar& bar) {
		ParamEditor::BeginGroup(bar.name.c_str());
		ParamEditor::RegisterValue("position", &bar.position, 1.0f, -2000.0f, 4000.0f);
		ParamEditor::RegisterValue("scale", &bar.scale, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("fillOffset", &bar.fillOffset, 0.5f, -1000.0f, 1000.0f);
		ParamEditor::RegisterValue("fillScale", &bar.fillScale, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("labelOffset", &bar.labelOffset, 0.5f, -1000.0f, 1000.0f);
		ParamEditor::RegisterValue("labelScale", &bar.labelScale, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterColor("ghostColor", &bar.ghostColor);
		ParamEditor::RegisterValue("ghostDelay", &bar.ghostDelay, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("ghostSpeed", &bar.ghostSpeed, 0.01f, 0.01f, 10.0f);
		ParamEditor::EndGroup();
	}

	void GameUI::RegisterText(Text& text) {
		ParamEditor::BeginGroup(text.name.c_str());
		ParamEditor::RegisterValue("position", &text.position, 1.0f, -2000.0f, 4000.0f);
		ParamEditor::RegisterValue("scale", &text.scale, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("fadeTime", &text.fadeTime, 0.01f, 0.0f, 5.0f);
		ParamEditor::EndGroup();
	}

	void GameUI::Initialize() {
		auto create = [](GameImage::Image image, Anchor anchor) {
			auto sprite = make_unique<Sprite>();
			sprite->Initialize(GameImage::GetImage(image), { 0.0f,0.0f }, anchor);
			return sprite;
		};

		// HPバーは左端を基準にする（fillは左端固定で縮む）
		playerHp_.frame = create(GameImage::Image::PlayerHpFrame, Anchor::Left);
		playerHp_.ghost = create(GameImage::Image::PlayerHpFill, Anchor::Left);
		playerHp_.fill = create(GameImage::Image::PlayerHpFill, Anchor::Left);
		playerHp_.label = create(GameImage::Image::PlayerHpLabel, Anchor::Left);

		bossHp_.frame = create(GameImage::Image::BossHpFrame, Anchor::Left);
		bossHp_.ghost = create(GameImage::Image::BossHpFill, Anchor::Left);
		bossHp_.fill = create(GameImage::Image::BossHpFill, Anchor::Left);
		bossHp_.label = create(GameImage::Image::BossHpLabel, Anchor::Left);

		// 文字は中央を基準にする
		clearText_.sprite = create(GameImage::Image::TextClear, Anchor::Center);
		failedText_.sprite = create(GameImage::Image::TextFailed, Anchor::Center);

		Reset();
	}

	void GameUI::Update(float playerHpRatio, float bossHpRatio) {
		UpdateHpBar(playerHp_, playerHpRatio);
		UpdateHpBar(bossHp_, bossHpRatio);
		UpdateText(clearText_);
		UpdateText(failedText_);
	}

	void GameUI::UpdateHpBar(HpBar& bar, float ratio) {
		if (!bar.frame) return;

		// スローの影響を受けないように実時間で進める
		const float dt = GetDeltaTime();
		ratio = clamp(ratio, 0.0f, 1.0f);

		// 減ったら待ち時間からやり直す（連続で当たっている間は止まったまま）
		if (ratio < bar.ratio) {
			bar.ghostTimer = bar.ghostDelay;
		}
		bar.ratio = ratio;

		if (bar.ghostRatio < bar.ratio) {
			// 回復したときはすぐに合わせる
			bar.ghostRatio = bar.ratio;
		} else if (bar.ghostTimer > 0.0f) {
			bar.ghostTimer -= dt;
		} else {
			bar.ghostRatio = max(bar.ratio, bar.ghostRatio - bar.ghostSpeed * dt);
		}

		// 位置と大きさ（fillとlabelは全体の大きさに合わせてずれも伸び縮みする）
		const Vector2& s = bar.scale;
		const Vector2 fillPos = { bar.position.x + bar.fillOffset.x * s.x, bar.position.y + bar.fillOffset.y * s.y };
		const Vector2 fillScale = { s.x * bar.fillScale.x, s.y * bar.fillScale.y };
		const Vector2 labelPos = { bar.position.x + bar.labelOffset.x * s.x, bar.position.y + bar.labelOffset.y * s.y };

		bar.frame->SetTranslate(bar.position);
		bar.frame->SetScale(s);

		bar.ghost->SetTranslate(fillPos);
		bar.ghost->SetScale({ fillScale.x * bar.ghostRatio, fillScale.y });
		bar.ghost->SetColor(bar.ghostColor);

		bar.fill->SetTranslate(fillPos);
		bar.fill->SetScale({ fillScale.x * bar.ratio, fillScale.y });

		bar.label->SetTranslate(labelPos);
		bar.label->SetScale({ s.x * bar.labelScale.x, s.y * bar.labelScale.y });

		bar.frame->TransferMatrix();
		bar.ghost->TransferMatrix();
		bar.fill->TransferMatrix();
		bar.label->TransferMatrix();
	}

	void GameUI::UpdateText(Text& text) {
		if (!text.sprite) return;

		float alpha = 0.0f;
		if (text.isVisible) {
			text.timer += GetDeltaTime();
			alpha = text.fadeTime <= 0.0f ? 1.0f : clamp(text.timer / text.fadeTime, 0.0f, 1.0f);
		}
		text.sprite->SetTranslate(text.position);
		text.sprite->SetScale(text.scale);
		text.sprite->SetColor({ 1.0f,1.0f,1.0f,alpha });
		text.sprite->TransferMatrix();
	}

	void GameUI::Draw() {
		if (isHpVisible_) {
			DrawHpBar(playerHp_);
			DrawHpBar(bossHp_);
		}
		if (clearText_.isVisible && clearText_.sprite) {
			clearText_.sprite->Draw();
		}
		if (failedText_.isVisible && failedText_.sprite) {
			failedText_.sprite->Draw();
		}
	}

	void GameUI::DrawHpBar(HpBar& bar) {
		if (!bar.frame) return;
		bar.frame->Draw();
		// 遅れゲージは減っている間だけ見えればいい
		if (bar.ghostRatio > bar.ratio) {
			bar.ghost->Draw();
		}
		bar.fill->Draw();
		bar.label->Draw();
	}

	void GameUI::Finalize() {
		auto finalize = [](unique_ptr<Sprite>& sprite) {
			if (sprite) {
				sprite->Finalize();
				sprite.reset();
			}
		};
		for (HpBar* bar : { &playerHp_, &bossHp_ }) {
			finalize(bar->frame);
			finalize(bar->ghost);
			finalize(bar->fill);
			finalize(bar->label);
		}
		finalize(clearText_.sprite);
		finalize(failedText_.sprite);
	}

	void GameUI::ShowClearText() {
		clearText_.isVisible = true;
		clearText_.timer = 0.0f;
	}

	void GameUI::ShowFailedText() {
		failedText_.isVisible = true;
		failedText_.timer = 0.0f;
	}

	void GameUI::Reset() {
		isHpVisible_ = false;
		ResetHpBar(playerHp_);
		ResetHpBar(bossHp_);
		clearText_.isVisible = false;
		clearText_.timer = 0.0f;
		failedText_.isVisible = false;
		failedText_.timer = 0.0f;
	}

	void GameUI::ResetHpBar(HpBar& bar) {
		bar.ratio = 1.0f;
		bar.ghostRatio = 1.0f;
		bar.ghostTimer = 0.0f;
	}
}
