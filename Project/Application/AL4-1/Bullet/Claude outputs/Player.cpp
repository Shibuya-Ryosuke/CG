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
		// 向きは最後に入力した方向
		if (moveX != 0.0f) {
			facing_ = moveX;
		}
		// ジャンプ
		Jump();
		// hp残量に応じた演出のアップデート
		HpEffectUpdate();
		// 無敵時間のアップデート
		InvincibleUpdate();
		// 被ダメージ時のアップデート
		DamageTakenUpdate();
		// 攻撃（パリィ兼用）のアップデート
		AttackUpdate();
		// パリィ成功時の演出（ブラー＋スロー）のアップデート
		ParryEffectUpdate();

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
		// 攻撃判定は向いている方向の前、パリィ判定は体の中心に合わせて体より少し大きく
		attackObb_.center = { obb_.center.x + facing_ * attackReach_, obb_.center.y + attackOffsetY_, obb_.center.z };
		attackObb_.size = attackHalfSize_;
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
		// デバッグ：攻撃中は攻撃判定を赤、パリィ判定を黄色で表示
		if (isAttacking_) {
			PrimitiveRenderer::DrawOBB(attackObb_, { 1.0f,0.0f,0.0f,1.0f }, PrimitiveDrawMode::Wireframe);
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
		// 演出を最初から（カウンターの方が強く・遅く）
		isParryEffect_ = true;
		parryEffectTimer_ = 0.0f;
		effectDuration_ = isCounter ? counterEffectDuration_ : parryEffectDuration_;
		effectPeakTime_ = isCounter ? counterEffectPeakTime_ : parryEffectPeakTime_;
		effectMaxBlur_ = isCounter ? counterMaxBlur_ : parryMaxBlur_;
		effectSlowScale_ = isCounter ? counterSlowScale_ : parrySlowScale_;

		PostProcess::GetInstance()->SetBlurEnabled(true);
	}

	void Player::ResetStatus() {
		hp_ = maxHp_; isDead_ = false;

		velocity_ = { 0.0f,0.0f,0.0f };
		facing_ = 1.0f;
		isAttacking_ = false;
		isAttackHitDone_ = false;
		attackTimer_ = 0.0f;
		attackCooldownTimer_ = 0.0f;
		isParryEffect_ = false;
		parryEffectTimer_ = 0.0f;
		isDamageTaken_ = false;
		damageTakenDuration_ = 0.0f;
		isInvincible_ = false;
		invincibleDuration_ = 0.0f;

		if (model_) model_->SetColor({ 1.0f,1.0f,1.0f,1.0f });
		auto ppInstance = PostProcess::GetInstance();
		ppInstance->SetChromaticAberrationEnabled(false);
		ppInstance->SetChromaticAberrationStrength(0.0f);
		ppInstance->SetBlurEnabled(false);
		ppInstance->SetBlurStrength(0.0f);
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

		ParamEditor::BeginGroup("PlayerAttack");
		ParamEditor::RegisterValue("activeTime", &attackActiveTime_, 0.01f, 0.01f, 1.0f);
		ParamEditor::RegisterValue("cooldown", &attackCooldown_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("damage", &attackDamage_, 0.1f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("reach", &attackReach_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("offsetY", &attackOffsetY_, 0.01f, -5.0f, 5.0f);
		ParamEditor::RegisterValue("halfSize", &attackHalfSize_, 0.01f, 0.05f, 10.0f);
		ParamEditor::RegisterValue("parryRange", &parryRange_, 0.01f, 0.0f, 5.0f);
		ParamEditor::RegisterValue("attackColor", &attackColor_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("cooldownColor", &cooldownColor_, 0.01f, 0.0f, 1.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("PlayerParryEffect");
		ParamEditor::RegisterValue("parryDuration", &parryEffectDuration_, 0.01f, 0.01f, 3.0f);
		ParamEditor::RegisterValue("parryPeakTime", &parryEffectPeakTime_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("parryMaxBlur", &parryMaxBlur_, 0.0005f, 0.0f, 0.1f);
		ParamEditor::RegisterValue("parrySlowScale", &parrySlowScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("counterDuration", &counterEffectDuration_, 0.01f, 0.01f, 3.0f);
		ParamEditor::RegisterValue("counterPeakTime", &counterEffectPeakTime_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("counterMaxBlur", &counterMaxBlur_, 0.0005f, 0.0f, 0.1f);
		ParamEditor::RegisterValue("counterSlowScale", &counterSlowScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::EndGroup();
	}

	void Player::Jump() {
		// 接地状態のみ判断
		if (onGround_ && Input::TriggerKey(DIK_W)) {
			velocity_.y = jumpPower_;
			onGround_ = false;
		}
	}

	void Player::Attack() {
		isAttacking_ = true;
		isAttackHitDone_ = false;
		attackTimer_ = attackActiveTime_;
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

	void Player::AttackUpdate() {
		const float dt = GetScaleTime();

		if (isAttacking_) {
			// 攻撃判定が出ている間
			attackTimer_ -= dt;
			if (attackTimer_ <= 0.0f) {
				isAttacking_ = false;
				// 1回攻撃するごとにクールタイム
				attackCooldownTimer_ = attackCooldown_;
			}
		} else if (attackCooldownTimer_ > 0.0f) {
			// クールタイム中
			attackCooldownTimer_ -= dt;
		} else if (Input::TriggerKey(DIK_SPACE)) {
			Attack();
		}

		// 色（無敵中は点滅を優先）
		if (isInvincible_) return;
		if (isAttacking_) {
			model_->SetColor(attackColor_);
		} else if (attackCooldownTimer_ > 0.0f) {
			model_->SetColor(cooldownColor_);
		} else {
			model_->SetColor({ 1.0f,1.0f,1.0f,1.0f });
		}
	}

	void Player::ParryEffectUpdate() {
		if (!isParryEffect_) return;
		// スローで演出が伸びないように deltaタイムを足す
		parryEffectTimer_ += GetDeltaTime();

		// 0 → 1（peakTime） → 0（duration）の山なりの強さ
		const float peak = std::clamp(effectPeakTime_, 0.0f, effectDuration_);
		float w = 0.0f;
		if (parryEffectTimer_ < peak) {
			w = parryEffectTimer_ / peak;
		} else if (effectDuration_ > peak) {
			w = 1.0f - (parryEffectTimer_ - peak) / (effectDuration_ - peak);
		}
		w = std::clamp(w, 0.0f, 1.0f);

		auto ppInstance = PostProcess::GetInstance();
		// ブラー
		ppInstance->SetBlurStrength(effectMaxBlur_ * w);
		// スロー（被ダメージのスロー中はそちらを優先）
		if (!isDamageTaken_) {
			SetTimeScale(1.0f + (effectSlowScale_ - 1.0f) * w);
		}

		// 演出終了
		if (parryEffectTimer_ >= effectDuration_) {
			isParryEffect_ = false;
			parryEffectTimer_ = 0.0f;
			ppInstance->SetBlurEnabled(false);
			ppInstance->SetBlurStrength(0.0f);
			if (!isDamageTaken_) {
				SetTimeScale(1.0f);
			}
		}
	}

}
