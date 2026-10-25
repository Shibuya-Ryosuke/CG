#include "GameUI.h"
#include "../GameResources/GameTex.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	GameUI::GameUI() {
		// 初期配置（1280x720想定。ParamEditorで調整する）
		playerHp_.name = "PlayerHp";
		playerHp_.position = { 40.0f, 60.0f };

		bossHp_.name = "BossHp";
		bossHp_.position = { 740.0f, 60.0f };

		// クリアの文字はズームしたプレイヤーの右側に出す
		clearText_.name = "TextClear";
		clearText_.position = { 900.0f, 230.0f };
		clearText_.scale = { 0.6f, 0.6f };
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

		ParamEditor::BeginGroup("Gauge");
		ParamEditor::RegisterValue("position", &gauge_.position, 1.0f, -2000.0f, 4000.0f);
		ParamEditor::RegisterValue("scale", &gauge_.scale, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("fillOffset", &gauge_.fillOffset, 0.5f, -1000.0f, 1000.0f);
		ParamEditor::RegisterValue("fillScale", &gauge_.fillScale, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("fillSpeed", &gauge_.fillSpeed, 0.01f, 0.01f, 20.0f);
		ParamEditor::RegisterColor("maxDimColor", &gauge_.maxDimColor);
		ParamEditor::RegisterValue("maxBlinkSpeed", &gauge_.maxBlinkSpeed, 0.1f, 0.0f, 20.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("FailedDim");
		ParamEditor::RegisterValue("position", &failedDim_.position, 1.0f, -2000.0f, 4000.0f);
		ParamEditor::RegisterValue("scale", &failedDim_.scale, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("maxAlpha", &failedDim_.maxAlpha, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("fadeTime", &failedDim_.fadeTime, 0.01f, 0.0f, 5.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("ClearTime");
		ParamEditor::RegisterValue("position", &time_.position, 1.0f, -2000.0f, 4000.0f);
		ParamEditor::RegisterValue("scale", &time_.scale, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("numberOffset", &time_.numberOffset, 0.5f, -2000.0f, 2000.0f);
		ParamEditor::RegisterValue("numberScale", &time_.numberScale, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("charAdvance", &time_.charAdvance, 0.5f, 0.0f, 300.0f);
		ParamEditor::RegisterValue("dotAdvance", &time_.dotAdvance, 0.5f, 0.0f, 300.0f);
		ParamEditor::RegisterValue("delay", &time_.delay, 0.01f, 0.0f, 5.0f);
		ParamEditor::RegisterValue("fadeTime", &time_.fadeTime, 0.01f, 0.0f, 5.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("Menu");
		ParamEditor::RegisterValue("clearPosition", &menu_.clearPosition, 1.0f, -2000.0f, 4000.0f);
		ParamEditor::RegisterValue("failedPosition", &menu_.failedPosition, 1.0f, -2000.0f, 4000.0f);
		ParamEditor::RegisterValue("itemSpacing", &menu_.itemSpacing, 0.5f, 0.0f, 500.0f);
		ParamEditor::RegisterValue("scale", &menu_.scale, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("selectedScale", &menu_.selectedScale, 0.01f, 0.1f, 3.0f);
		ParamEditor::RegisterValue("pulseSpeed", &menu_.pulseSpeed, 0.1f, 0.0f, 20.0f);
		ParamEditor::RegisterValue("pulseAmount", &menu_.pulseAmount, 0.005f, 0.0f, 0.5f);
		ParamEditor::RegisterColor("selectedColor", &menu_.selectedColor);
		ParamEditor::RegisterColor("normalColor", &menu_.normalColor);
		ParamEditor::RegisterValue("clearDelay", &menu_.clearDelay, 0.01f, 0.0f, 5.0f);
		ParamEditor::RegisterValue("fadeTime", &menu_.fadeTime, 0.01f, 0.0f, 5.0f);
		ParamEditor::EndGroup();

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
		ParamEditor::RegisterValue("delay", &text.delay, 0.01f, 0.0f, 5.0f);
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

		// ゲージも左端を基準にする
		gauge_.frame = create(GameImage::Image::GaugeFrame, Anchor::Left);
		gauge_.fill = create(GameImage::Image::GaugeFill, Anchor::Left);

		// 文字は中央を基準にする
		clearText_.sprite = create(GameImage::Image::TextClear, Anchor::Center);
		failedText_.sprite = create(GameImage::Image::TextFailed, Anchor::Center);
		// ゲームオーバー時の黒（フェードと同じ黒い画像）
		failedDim_.sprite = create(GameImage::Image::FadeBlack, Anchor::Center);

		// クリアタイム（数字は全部同じ大きさの画像なので、毎フレーム画像を差し替えて使う）
		time_.label = create(GameImage::Image::TextClearTime, Anchor::Left);
		for (auto& c : time_.chars) {
			c = create(GameImage::Image::Num0, Anchor::Left);
		}

		// メニュー（左端を基準にそろえる）
		menu_.items[static_cast<size_t>(MenuItem::Retry)] = create(GameImage::Image::MenuRetry, Anchor::Left);
		menu_.items[static_cast<size_t>(MenuItem::Title)] = create(GameImage::Image::MenuTitle, Anchor::Left);

		Reset();
	}

	void GameUI::Update(float playerHpRatio, float bossHpRatio, float gaugeRatio) {
		UpdateHpBar(playerHp_, playerHpRatio);
		UpdateHpBar(bossHp_, bossHpRatio);
		UpdateGauge(gaugeRatio);
		UpdateText(clearText_);
		UpdateText(failedText_);
		UpdateDim();
		UpdateTime();
		UpdateMenu();
	}

	float GameUI::FadeAlpha(float timer, float delay, float fadeTime) {
		if (timer < delay) return 0.0f;
		if (fadeTime <= 0.0f) return 1.0f;
		return clamp((timer - delay) / fadeTime, 0.0f, 1.0f);
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

	void GameUI::UpdateGauge(float ratio) {
		if (!gauge_.frame) return;

		const float dt = GetDeltaTime();
		gauge_.ratio = clamp(ratio, 0.0f, 1.0f);
		// 表示は少し遅れて追いつく（フルパリィで0になるときも、すっと減って見える）
		const float step = gauge_.fillSpeed * dt;
		if (gauge_.displayRatio < gauge_.ratio) {
			gauge_.displayRatio = min(gauge_.ratio, gauge_.displayRatio + step);
		} else {
			gauge_.displayRatio = max(gauge_.ratio, gauge_.displayRatio - step);
		}

		// 満タンのときは点滅して「今撃てる」と分かるようにする
		Vector4 color = { 1.0f,1.0f,1.0f,1.0f };
		if (gauge_.ratio >= 1.0f) {
			gauge_.timer += dt;
			const float wave = (cos(gauge_.timer * gauge_.maxBlinkSpeed * 2.0f * numbers::pi_v<float>) + 1.0f) * 0.5f;
			color = {
				gauge_.maxDimColor.x + (1.0f - gauge_.maxDimColor.x) * wave,
				gauge_.maxDimColor.y + (1.0f - gauge_.maxDimColor.y) * wave,
				gauge_.maxDimColor.z + (1.0f - gauge_.maxDimColor.z) * wave,
				gauge_.maxDimColor.w + (1.0f - gauge_.maxDimColor.w) * wave,
			};
		} else {
			gauge_.timer = 0.0f;
		}

		const Vector2& s = gauge_.scale;
		gauge_.frame->SetTranslate(gauge_.position);
		gauge_.frame->SetScale(s);
		gauge_.frame->SetColor(color);
		gauge_.fill->SetTranslate({ gauge_.position.x + gauge_.fillOffset.x * s.x, gauge_.position.y + gauge_.fillOffset.y * s.y });
		gauge_.fill->SetScale({ s.x * gauge_.fillScale.x * gauge_.displayRatio, s.y * gauge_.fillScale.y });
		gauge_.fill->SetColor(color);
		gauge_.frame->TransferMatrix();
		gauge_.fill->TransferMatrix();
	}

	void GameUI::UpdateText(Text& text) {
		if (!text.sprite) return;

		float alpha = 0.0f;
		if (text.isVisible) {
			text.timer += GetDeltaTime();
			alpha = FadeAlpha(text.timer, text.delay, text.fadeTime);
		}
		text.sprite->SetTranslate(text.position);
		text.sprite->SetScale(text.scale);
		text.sprite->SetColor({ 1.0f,1.0f,1.0f,alpha });
		text.sprite->TransferMatrix();
	}

	void GameUI::UpdateDim() {
		if (!failedDim_.sprite) return;

		float alpha = 0.0f;
		if (failedDim_.isVisible) {
			failedDim_.timer += GetDeltaTime();
			alpha = FadeAlpha(failedDim_.timer, 0.0f, failedDim_.fadeTime) * failedDim_.maxAlpha;
		}
		failedDim_.sprite->SetTranslate(failedDim_.position);
		failedDim_.sprite->SetScale(failedDim_.scale);
		failedDim_.sprite->SetColor({ 1.0f,1.0f,1.0f,alpha });
		failedDim_.sprite->TransferMatrix();
	}

	void GameUI::UpdateTime() {
		if (!time_.label) return;

		float alpha = 0.0f;
		if (time_.isVisible) {
			time_.timer += GetDeltaTime();
			alpha = FadeAlpha(time_.timer, time_.delay, time_.fadeTime);
		}
		const Vector2& s = time_.scale;
		time_.label->SetTranslate(time_.position);
		time_.label->SetScale(s);
		time_.label->SetColor({ 1.0f,1.0f,1.0f,alpha });
		time_.label->TransferMatrix();

		// 数字を1文字ずつ並べる（小数点だけ幅を詰める）
		const Vector2 numScale = { s.x * time_.numberScale.x, s.y * time_.numberScale.y };
		float x = time_.position.x + time_.numberOffset.x * s.x;
		const float y = time_.position.y + time_.numberOffset.y * s.y;
		for (size_t i = 0; i < time_.chars.size(); ++i) {
			Sprite* sprite = time_.chars[i].get();
			if (i >= time_.text.size()) {
				sprite->SetColor({ 1.0f,1.0f,1.0f,0.0f });
				sprite->TransferMatrix();
				continue;
			}
			const char c = time_.text[i];
			GameImage::Image image = GameImage::Image::NumS;
			float advance = time_.charAdvance;
			float drawX = x;
			if (c >= '0' && c <= '9') {
				image = static_cast<GameImage::Image>(static_cast<uint32_t>(GameImage::Image::Num0) + static_cast<uint32_t>(c - '0'));
			} else if (c == '.') {
				image = GameImage::Image::NumDot;
				advance = time_.dotAdvance;
				// 画像は数字と同じ幅で点が真ん中にあるので、詰めた分だけ左へずらす
				drawX -= (time_.charAdvance - time_.dotAdvance) * 0.5f * numScale.x;
			}
			sprite->SetTex(GameImage::GetImage(image));
			sprite->SetTranslate({ drawX, y });
			sprite->SetScale(numScale);
			sprite->SetColor({ 1.0f,1.0f,1.0f,alpha });
			sprite->TransferMatrix();
			x += advance * numScale.x;
		}
	}

	void GameUI::UpdateMenu() {
		if (!menu_.items[0]) return;

		float alpha = 0.0f;
		if (menu_.isVisible) {
			menu_.timer += GetDeltaTime();
			alpha = FadeAlpha(menu_.timer, menu_.delay, menu_.fadeTime);
		}
		const Vector2 base = menu_.isClearLayout ? menu_.clearPosition : menu_.failedPosition;
		// 選んでいる項目は少し大きく・明るく、ゆっくり脈打たせる
		const float pulse = 1.0f + menu_.pulseAmount * sin(menu_.timer * menu_.pulseSpeed * 2.0f * numbers::pi_v<float>);
		for (size_t i = 0; i < menu_.items.size(); ++i) {
			Sprite* sprite = menu_.items[i].get();
			const bool isSelected = static_cast<int32_t>(i) == menu_.selection;
			const float k = isSelected ? menu_.selectedScale * pulse : 1.0f;
			Vector4 color = isSelected ? menu_.selectedColor : menu_.normalColor;
			color.w *= alpha;
			sprite->SetTranslate({ base.x, base.y + menu_.itemSpacing * static_cast<float>(i) });
			sprite->SetScale({ menu_.scale.x * k, menu_.scale.y * k });
			sprite->SetColor(color);
			sprite->TransferMatrix();
		}
	}

	void GameUI::Draw() {
		if (isHpVisible_) {
			DrawHpBar(playerHp_);
			DrawHpBar(bossHp_);
			// ゲージ（空のときは中身を描かない）
			if (gauge_.frame) {
				gauge_.frame->Draw();
				if (gauge_.displayRatio > 0.0f) {
					gauge_.fill->Draw();
				}
			}
		}
		// ゲームオーバー時の黒（HPなどのUIの上、文字の下）
		if (failedDim_.isVisible && failedDim_.sprite) {
			failedDim_.sprite->Draw();
		}
		if (clearText_.isVisible && clearText_.sprite) {
			clearText_.sprite->Draw();
		}
		if (failedText_.isVisible && failedText_.sprite) {
			failedText_.sprite->Draw();
		}
		if (time_.isVisible && time_.label) {
			time_.label->Draw();
			for (size_t i = 0; i < time_.text.size() && i < time_.chars.size(); ++i) {
				time_.chars[i]->Draw();
			}
		}
		if (menu_.isVisible) {
			for (auto& item : menu_.items) {
				if (item) item->Draw();
			}
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
		finalize(gauge_.frame);
		finalize(gauge_.fill);
		finalize(clearText_.sprite);
		finalize(failedText_.sprite);
		finalize(failedDim_.sprite);
		finalize(time_.label);
		for (auto& c : time_.chars) {
			finalize(c);
		}
		for (auto& item : menu_.items) {
			finalize(item);
		}
	}

	void GameUI::ShowClearResult(float clearTime) {
		clearText_.isVisible = true;
		clearText_.timer = 0.0f;

		// 小数点以下2桁 + s（999.99s まで）
		char buffer[16]{};
		snprintf(buffer, sizeof(buffer), "%.2fs", clamp(clearTime, 0.0f, 999.99f));
		time_.text = buffer;
		time_.isVisible = true;
		time_.timer = 0.0f;
	}

	void GameUI::ShowFailedText() {
		failedText_.isVisible = true;
		failedText_.timer = 0.0f;
		// 文字と一緒に黒を敷く
		failedDim_.isVisible = true;
		failedDim_.timer = 0.0f;
	}

	void GameUI::ShowMenu(bool isClearLayout) {
		menu_.isVisible = true;
		menu_.isClearLayout = isClearLayout;
		// クリア時はクリアタイムの後に出す。ゲームオーバー時は演出が終わってから呼ばれるのですぐ出す
		menu_.delay = isClearLayout ? menu_.clearDelay : 0.0f;
		menu_.timer = 0.0f;
		menu_.selection = 0;
	}

	void GameUI::Reset() {
		isHpVisible_ = false;
		ResetHpBar(playerHp_);
		ResetHpBar(bossHp_);
		gauge_.ratio = 0.0f;
		gauge_.displayRatio = 0.0f;
		gauge_.timer = 0.0f;
		clearText_.isVisible = false;
		clearText_.timer = 0.0f;
		failedText_.isVisible = false;
		failedText_.timer = 0.0f;
		failedDim_.isVisible = false;
		failedDim_.timer = 0.0f;
		time_.isVisible = false;
		time_.timer = 0.0f;
		time_.text.clear();
		menu_.isVisible = false;
		menu_.timer = 0.0f;
		menu_.selection = 0;
	}

	void GameUI::ResetHpBar(HpBar& bar) {
		bar.ratio = 1.0f;
		bar.ghostRatio = 1.0f;
		bar.ghostTimer = 0.0f;
	}
}
