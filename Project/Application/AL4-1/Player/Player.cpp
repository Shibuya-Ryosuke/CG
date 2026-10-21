#include "Player.h"
#include <algorithm>

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	Player::Player() = default;
	Player::~Player() = default;

	void Player::Initialize() {
		model_ = Model::Create("Resources/ApplicationResources/AL4-1/Player/player.obj");
		RegisterInParamEditor();
	}

	void Player::Update() {
		const float dt = GetScaleTime();

		// 移動
		float moveX = 0.0f;
		if (Input::PushKey(DIK_D)) {
			moveX += 1.0f;
		}
		if (Input::PushKey(DIK_A)) {
			moveX -= 1.0f;
		}
		// ジャンプ
		Jump();
		// hp残量に応じた演出のアップデート
		HpEffectUpdate();
		// 無敵時間のアップデート
		InvincibleUpdate();
		// 被ダメージ時のアップデート
		DamageTakenUpdate();
		// パリィ（受付・硬直・色）のアップデート
		ParryUpdate();
		// パリィ成功時のヒットストップのアップデート
		HitStopUpdate();

		// 速度の更新
		velocity_.x = moveX * moveSpeed_;
		velocity_.y -= gravity_ * dt;
		velocity_.y = max(velocity_.y, -maxFallSpeed_);

		Vector3 delta = { velocity_.x * dt, velocity_.y * dt, 0.0f };

		// --- マップとの判定付き移動 ---
		if (map_) {
			MapHit hit;
			transform_.translate = map_->MoveAndCollide(transform_.translate, delta, halfSize_, hit);

			if (hit.bottom) velocity_.y = 0.0f;                    // 着地
			if (hit.top && velocity_.y > 0.0f) velocity_.y = 0.0f; // 天井に頭をぶつけた
			onGround_ = hit.bottom;
		} else {
			// マップ未設定のときは従来どおり y=0 を地面にする
			transform_.translate.x += delta.x;
			transform_.translate.y += delta.y;
			if (transform_.translate.y < 0.0f) {
				transform_.translate.y = 0.0f;
				velocity_.y = 0.0f;
				onGround_ = true;
			} else {
				onGround_ = false;
			}
		}
		Vector3 translate = transform_.translate;
		obb_.center = { translate.x,translate.y + halfSize_.y,translate.z };
		// パリィ判定は体の中心に合わせ、体の判定より少し大きくする
		parryObb_.center = obb_.center;
		parryObb_.size = { halfSize_.x + parryRange_.x, halfSize_.y + parryRange_.y, halfSize_.z + parryRange_.z };
		// modelにセット
		model_->SetTransform(transform_);
	}

	void Player::Draw(RyoEngine::Camera& camera) {
		// 行列の確定
		model_->TransferMatrix(camera);

		// 描画
		model_->Draw();
		PrimitiveRenderer::DrawOBB(obb_, { 0.0f,1.0f,0.0f,1.0f },PrimitiveDrawMode::Wireframe);
		// デバッグ：パリィ受付中は判定を黄色で表示
		if (isParryActive_) {
			PrimitiveRenderer::DrawOBB(parryObb_, { 1.0f,1.0f,0.0f,1.0f }, PrimitiveDrawMode::Wireframe);
		}
	}

	void Player::OnDamage(float damage) {
		// 既に無敵中なら無効化
		if (isInvincible_) return;

		// ダメージ処理
		hp_ -= damage;
		if (hp_ <= 0.0f) {
			hp_ = 0.0f;
			isDead_ = true;
			return;
		}

		// 無敵有効
		isInvincible_ = true;
		invincibleDuration_ = maxInvincibleDuration_;
		// 被ダメージ演出有効
		isDamageTaken_ = true;
		damageTakenDuration_ = maxDamageTakenDuration_;
		// ポストエフェクトオン
		PostProcess::GetInstance()->SetChromaticAberrationEnabled(true);
	}

	void Player::OnParrySuccess(bool isCounter) {
		// 成功したので、受付が終わっても硬直しない
		isParrySucceeded_ = true;

		// ヒットストップ（カウンターの方が長く・強く）
		isHitStop_ = true;
		hitStopDuration_ = isCounter ? counterHitStopDuration_ : parryHitStopDuration_;
		hitStopTimer_ = hitStopDuration_;
		hitStopChromaticStrength_ = isCounter ? counterChromaticStrength_ : parryChromaticStrength_;
		SetTimeScale(hitStopTimeScale_);

		// 被ダメージ演出中はそちらの色収差を優先する
		if (!isDamageTaken_) {
			auto ppInstance = PostProcess::GetInstance();
			ppInstance->SetChromaticAberrationEnabled(true);
			ppInstance->SetChromaticAberrationStrength(hitStopChromaticStrength_);
		}
	}

	void Player::ResetStatus() {
		hp_ = maxHp_; isDead_ = false;

		velocity_ = { 0.0f,0.0f,0.0f };
		isParryActive_ = false;
		isParrySucceeded_ = false;
		parryTimer_ = 0.0f;
		parryCooldownTimer_ = 0.0f;
		isHitStop_ = false;
		hitStopTimer_ = 0.0f;
		isDamageTaken_ = false;
		damageTakenDuration_ = 0.0f;
		isInvincible_ = false;
		invincibleDuration_ = 0.0f;

		if (model_) model_->SetColor({ 1.0f,1.0f,1.0f,1.0f });
		auto ppInstance = PostProcess::GetInstance();
		ppInstance->SetChromaticAberrationEnabled(false);
		ppInstance->SetChromaticAberrationStrength(0.0f);
		SetTimeScale(1.0f);
	}

	void Player::RegisterInParamEditor() {
		ParamEditor::BeginGroup("Player");
		ParamEditor::RegisterValue("transform", &transform_);
		ParamEditor::RegisterValue("moveSpeed", &moveSpeed_, 0.1f, 0.0f, 30.0f);
		ParamEditor::RegisterValue("jumpPower", &jumpPower_, 0.1f, 0.0f, 60.0f);
		ParamEditor::RegisterValue("gravity", &gravity_, 0.1f, 0.0f, 200.0f);
		ParamEditor::RegisterValue("maxFallSpeed", &maxFallSpeed_, 0.1f, 0.0f, 120.0f);
		ParamEditor::RegisterValue("maxHp", &maxHp_, 1.0f, 1.0f, 500.0f);
		ParamEditor::RegisterValue("halfSize", &halfSize_, 0.01f, 0.1f, 5.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("PlayerDamage");
		ParamEditor::RegisterValue("effectDuration", &maxDamageTakenDuration_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("chromaticStrength", &maxDamageTakenChromaticAberrationStrength_, 0.01f, 0.0f, 2.0f);
		ParamEditor::RegisterValue("timeScale", &damageTakenTimeScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("invincibleDuration", &maxInvincibleDuration_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("blinkCount", &blinkCount_, 1.0f, 1, 20);
		ParamEditor::RegisterValue("minAlpha", &minAlpha_, 0.01f, 0.0f, 1.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("PlayerHpEffect");
		ParamEditor::RegisterValue("maxGrayScale", &maxGrayScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("maxNoise", &maxNoiseIntensity_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("startRatio", &hpEffectStartRaito_, 0.01f, 0.0f, 1.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("PlayerParry");
		ParamEditor::RegisterValue("window", &parryWindow_, 0.01f, 0.01f, 1.0f);
		ParamEditor::RegisterValue("cooldown", &parryCooldown_, 0.01f, 0.0f, 2.0f);
		ParamEditor::RegisterValue("range", &parryRange_, 0.01f, 0.0f, 5.0f);
		ParamEditor::RegisterValue("parryHitStop", &parryHitStopDuration_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("counterHitStop", &counterHitStopDuration_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("hitStopTimeScale", &hitStopTimeScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("parryChromatic", &parryChromaticStrength_, 0.01f, 0.0f, 2.0f);
		ParamEditor::RegisterValue("counterChromatic", &counterChromaticStrength_, 0.01f, 0.0f, 2.0f);
		ParamEditor::EndGroup();
	}

	void Player::Jump() {
		// 接地状態のみ判断
		if (onGround_ && Input::TriggerKey(DIK_W)) {
			velocity_.y = jumpPower_;
			onGround_ = false;
		}
	}

	void Player::HpEffectUpdate() {
		// ポストプロセスのインスタンス取得
		auto ppInstance = PostProcess::GetInstance();

		// 減った割合 0（満タン）→ 1（瀕死）
		float lost = std::clamp((hpEffectStartRaito_ - hp_ / maxHp_) / hpEffectStartRaito_, 0.0f, 1.0f);

		// 白黒化
		ppInstance->SetGrayscaleEnabled(lost > 0.0f);
		ppInstance->SetGrayscaleIntensity(maxGrayScale_ * lost);
		// ノイズ
		ppInstance->SetNoiseEnabled(lost > 0.0f);
		ppInstance->SetNoiseIntensity(maxNoiseIntensity_ * lost);
	}

	void Player::DamageTakenUpdate() {
		if (!isDamageTaken_) return;
		// ここはdeltaタイムを引く
		damageTakenDuration_ -= GetDeltaTime();
		float t = 1.0f - std::clamp(damageTakenDuration_ / maxDamageTakenDuration_, 0.0f, 1.0f);

		// ポストプロセスインスタンス取得
		auto ppInstance = PostProcess::GetInstance();
		// 色収差
		float strength = EaseOutCubic(t, maxDamageTakenChromaticAberrationStrength_, 0.0f);
		ppInstance->SetChromaticAberrationStrength(strength);
		// スロー
		SetTimeScale(damageTakenTimeScale_);

		// 被ダメージ時間の終了
		if (damageTakenDuration_ <= 0.0f) {
			damageTakenDuration_ = 0.0f;
			ppInstance->SetChromaticAberrationEnabled(false);
			ppInstance->SetChromaticAberrationStrength(0.0f);
			SetTimeScale(1.0f);

			isDamageTaken_ = false;
		}
	}

	void Player::InvincibleUpdate() {
		if (!isInvincible_)return;

		// ここはスケールタイムを引く
		invincibleDuration_ -= GetScaleTime();

		// 点滅
		float t = 1.0f - std::clamp(invincibleDuration_ / maxInvincibleDuration_, 0.0f, 1.0f);
		// cos で 1 → min → 1 を blinkCount_ 回
		float wave = (std::cos(t * blinkCount_ * 2.0f * std::numbers::pi_v<float>) + 1.0f) * 0.5f; // 1→0→1
		float alpha = minAlpha_ + (1.0f - minAlpha_) * wave;
		model_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });

		// 無敵終了
		if (invincibleDuration_ <= 0.0f) {
			invincibleDuration_ = 0.0f;
			isInvincible_ = false;
		}
	}

	void Player::ParryUpdate() {
		const float dt = GetScaleTime();

		if (isParryActive_) {
			// 受付中
			parryTimer_ -= dt;
			if (parryTimer_ <= 0.0f) {
				isParryActive_ = false;
				// 空振りしたときだけ硬直（連打で全部はじけないようにする）
				parryCooldownTimer_ = isParrySucceeded_ ? 0.0f : parryCooldown_;
			}
		} else if (parryCooldownTimer_ > 0.0f) {
			// 硬直中
			parryCooldownTimer_ -= dt;
		} else if (Input::TriggerKey(DIK_J)) {
			// 受付開始
			isParryActive_ = true;
			isParrySucceeded_ = false;
			parryTimer_ = parryWindow_;
		}

		// 色（無敵中は点滅を優先）
		if (isInvincible_) return;
		if (isParryActive_) {
			model_->SetColor(parryColor_);
		} else if (parryCooldownTimer_ > 0.0f) {
			model_->SetColor(cooldownColor_);
		} else {
			model_->SetColor({ 1.0f,1.0f,1.0f,1.0f });
		}
	}

	void Player::HitStopUpdate() {
		if (!isHitStop_) return;
		// 止まっている間も進むように deltaタイムを引く
		hitStopTimer_ -= GetDeltaTime();
		float t = 1.0f - std::clamp(hitStopTimer_ / hitStopDuration_, 0.0f, 1.0f);

		auto ppInstance = PostProcess::GetInstance();
		if (!isDamageTaken_) {
			ppInstance->SetChromaticAberrationStrength(EaseOutCubic(t, hitStopChromaticStrength_, 0.0f));
		}
		SetTimeScale(hitStopTimeScale_);

		// ヒットストップ終了
		if (hitStopTimer_ <= 0.0f) {
			hitStopTimer_ = 0.0f;
			isHitStop_ = false;
			SetTimeScale(1.0f);
			if (!isDamageTaken_) {
				ppInstance->SetChromaticAberrationEnabled(false);
				ppInstance->SetChromaticAberrationStrength(0.0f);
			}
		}
	}

}
