#include "Player.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	Player::Player() {
		GPUParticleManager::GetInstance()->RegisterPosition("playerPos", &transform_.translate);
		GPUParticleManager::GetInstance()->RegisterPosition("playerSpinPos", &spinTrailPos_);
		GPUParticleManager::GetInstance()->RegisterFlag("isParrying", &isParryEffect_);
		GPUParticleManager::GetInstance()->RegisterFlag("isParryTrigger", &isParryTrigger_);
		GPUParticleManager::GetInstance()->RegisterFlag("isPlayerAttacking", &isAttacking_);
		GPUParticleManager::GetInstance()->RegisterFlag("isPlayerInvincible", &isInvincible_);
		GPUParticleManager::GetInstance()->RegisterFlag("isPlayerCharge1", &isFirstCharge_);
		GPUParticleManager::GetInstance()->RegisterFlag("isPlayerCharge2", &isSecondCharge_);
		GPUParticleManager::GetInstance()->RegisterFlag("isPlayerChargeMax", &isMaxCharge_);
		GPUParticleManager::GetInstance()->RegisterFlag("isPlayerFullParry", &isFullParryTrigger_);
	}
	Player::~Player() {
		GPUParticleManager::GetInstance()->UnregisterPosition("playerPos");
		GPUParticleManager::GetInstance()->UnregisterPosition("playerSpinPos");
		GPUParticleManager::GetInstance()->UnregisterFlag("isParrying");
		GPUParticleManager::GetInstance()->UnregisterFlag("isParryTrigger");
		GPUParticleManager::GetInstance()->UnregisterFlag("isPlayerAttacking");
		GPUParticleManager::GetInstance()->UnregisterFlag("isPlayerInvincible");
		GPUParticleManager::GetInstance()->UnregisterFlag("isPlayerCharge1");
		GPUParticleManager::GetInstance()->UnregisterFlag("isPlayerCharge2");
		GPUParticleManager::GetInstance()->UnregisterFlag("isPlayerChargeMax");
		GPUParticleManager::GetInstance()->UnregisterFlag("isPlayerFullParry");
	}

	void Player::Initialize() {
		model_ = Model::Create("Resources/ApplicationResources/AL4-1/Player/player.obj");
		RegisterInParamEditor();
	}

	void Player::Update() {
		const float dt = GetScaleTime();

		// 1フレームだけのフラグを戻す（前フレームで立てた分は、このフレームの頭でパーティクルが読み終わっている）
		isParryTrigger_ = false;
		isFullParryTrigger_ = false;

		// 移動
		float moveX = 0.0f;
		if (Input::PushKey(DIK_D)) {
			moveX += 1.0f;
		}
		if (Input::PushKey(DIK_A)) {
			moveX -= 1.0f;
		}
		// 演出中は操作できない
		if (!isControllable_) {
			moveX = 0.0f;
		}
		// 向きは最後に入力した方向（攻撃中は向きを変えない）
		if (moveX != 0.0f && !isAttacking_) {
			facing_ = moveX;
		}
		// ジャンプ（攻撃中はできない）
		if (!isAttacking_ && isControllable_) {
			Jump();
		}
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

		// 攻撃（回転）中はその場で停止する（移動も落下もしない）
		if (isAttacking_) {
			velocity_ = { 0.0f,0.0f,0.0f };
			delta = { 0.0f,0.0f,0.0f };
		}

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
		// 攻撃判定（パリィ判定も兼ねる）。体の中心から向いている方向へ attackReach_ ずらす
		attackObb_.center = { obb_.center.x + facing_ * attackReach_, obb_.center.y + attackOffsetY_, obb_.center.z };
		attackObb_.size = attackHalfSize_;

		// モデルの向きを facing_ の方へ補間（フレームレートに依存しない指数平滑）
		float targetYaw = facing_ > 0.0f ? facingRightYaw_ : facingLeftYaw_;
		// クリア演出ではカメラの方を向く（左向きから回るときは、背中を見せないよう -360 した角度へ）
		if (isFaceCamera_) {
			targetYaw = currentYaw_ < 0.0f ? faceCameraYaw_ - 360.0f : faceCameraYaw_;
		}
		const float turnAlpha = turnDelay_ <= 0.0f ? 1.0f : 1.0f - std::exp(-dt / turnDelay_);
		currentYaw_ += (targetYaw - currentYaw_) * turnAlpha;

		// 見た目用のトランスフォーム（攻撃中はサマーソルトで1回転）
		Transform drawTransform = transform_;
		drawTransform.rotate.y += currentYaw_ * std::numbers::pi_v<float> / 180.0f;
		// 回転の軌跡パーティクルの発生位置（攻撃していないときは体の中心）
		spinTrailPos_ = { translate.x, translate.y + halfSize_.y, translate.z };
		if (isAttacking_) {
			// 0 → 1 の進み具合
			float t = 1.0f - std::clamp(attackTimer_ / attackActiveTime_, 0.0f, 1.0f);
			t = 1.0f - std::pow(1.0f - t, spinEasePower_);
			// 右向きなら頭が左（後ろ）へ回る後ろ宙返り
			float angle = facing_ * spinDirection_ * t * 2.0f * std::numbers::pi_v<float>;
			drawTransform.rotate.z += angle;

			// モデルの原点は足元なので、体の中心を軸に回って見えるよう位置を補正する
			const float pivot = halfSize_.y;
			drawTransform.translate.x += pivot * std::sin(angle);
			drawTransform.translate.y += pivot - pivot * std::cos(angle);

			// 足先が通る位置（モデルの回転と同じ向きに円を描く）
			spinTrailPos_.x += spinTrailRadius_ * std::sin(angle);
			spinTrailPos_.y -= spinTrailRadius_ * std::cos(angle);
		}

		// modelにセット
		model_->SetTransform(drawTransform);

		// プレイヤーに付けるライト
		LightManager::SetLightPosition("playerLight", model_->GetTranslate());
		LightManager::SetLightEnabled("playerLight", isIllumination_);
		// 発光
		// （ゲージが溜まるほど強くなる。パーティクルの段階フラグもここで更新）
		GaugeUpdate();
	}

	void Player::Draw(RyoEngine::Camera& camera) {
		// 行列の確定
		model_->TransferMatrix(camera);

		// 描画
		model_->Draw();
#ifdef _DEBUG
		PrimitiveRenderer::DrawOBB(obb_, { 0.0f,1.0f,0.0f,1.0f }, PrimitiveDrawMode::Wireframe);
		// デバッグ：攻撃中は攻撃（パリィ）判定を赤で表示
		if (isAttacking_) {
			PrimitiveRenderer::DrawOBB(attackObb_, { 1.0f,0.0f,0.0f,1.0f }, PrimitiveDrawMode::Wireframe);
		}
#endif
	}

	void Player::OnDamage(float damage) {
		// 既に無敵中なら無効化
		if (isInvincible_) return;

		// ダメージ処理
		hp_ -= damage;
		// 被弾したらゲージも少し減る（damageGaugePenalty_ が0なら減らない）
		gauge_ = max(0.0f, gauge_ - damageGaugePenalty_);
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
		PostProcess::GetInstance()->SetBlurEnabled(true);
	}

	bool Player::OnParrySuccess(bool isCounter) {
		// ゲージ満タンでのパリィはフルパリィ（ゲージを全部使う）
		const bool isFullParry = IsGaugeMax();

		// 演出を最初から（カウンターの方が強く・遅く）
		isParryEffect_ = true;
		isParryTrigger_ = true;
		parryEffectTimer_ = 0.0f;
		effectDuration_ = isCounter ? counterEffectDuration_ : parryEffectDuration_;
		effectPeakTime_ = isCounter ? counterEffectPeakTime_ : parryEffectPeakTime_;
		effectMaxChromatic_ = isCounter ? counterMaxChromatic_ : parryMaxChromatic_;
		effectSlowScale_ = isCounter ? counterSlowScale_ : parrySlowScale_;
		effectHoldTime_ = 0.0f;

		if (isFullParry) {
			gauge_ = 0.0f;
			isFullParryTrigger_ = true;
			// フルパリィは一番強く・遅く
			effectDuration_ = fullParryEffectDuration_;
			effectPeakTime_ = fullParryEffectPeakTime_;
			effectMaxChromatic_ = fullParryMaxChromatic_;
			effectSlowScale_ = fullParrySlowScale_;
			effectHoldTime_ = fullParryEffectHoldTime_;
		} else {
			// ゲージを溜める（近接のカウンターの方が多く溜まる）
			gauge_ = min(maxGauge_, gauge_ + (isCounter ? counterGaugeGain_ : parryGaugeGain_));
		}

		PostProcess::GetInstance()->SetChromaticAberrationEnabled(true);
		return isFullParry;
	}

	void Player::ResetStatus() {
		hp_ = maxHp_; isDead_ = false;

		velocity_ = { 0.0f,0.0f,0.0f };
		facing_ = 1.0f;
		currentYaw_ = facingRightYaw_;
		isFaceCamera_ = false;
		isAttacking_ = false;
		isAttackHitDone_ = false;
		attackTimer_ = 0.0f;
		attackCooldownTimer_ = 0.0f;
		isParryEffect_ = false;
		isParryTrigger_ = false;
		parryEffectTimer_ = 0.0f;
		isDamageTaken_ = false;
		damageTakenDuration_ = 0.0f;
		isInvincible_ = false;
		invincibleDuration_ = 0.0f;
		gauge_ = 0.0f;
		gaugeTimer_ = 0.0f;
		isFirstCharge_ = false;
		isSecondCharge_ = false;
		isMaxCharge_ = false;
		isFullParryTrigger_ = false;

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
		ParamEditor::RegisterValue("facingRightYaw", &facingRightYaw_, 1.0f, -360.0f, 360.0f);
		ParamEditor::RegisterValue("facingLeftYaw", &facingLeftYaw_, 1.0f, -360.0f, 360.0f);
		ParamEditor::RegisterValue("turnDelay", &turnDelay_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("faceCameraYaw", &faceCameraYaw_, 1.0f, -360.0f, 360.0f);

		ParamEditor::BeginGroup("Emissive");
		ParamEditor::RegisterValue("intensity", &emissiveIntensity_, 0.01f);
		ParamEditor::RegisterColor("color", &emissiveColor_);
		ParamEditor::RegisterFlag("isIllumination", &isIllumination_);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("Damage");
		ParamEditor::RegisterValue("effectDuration", &maxDamageTakenDuration_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("blurStrength", &maxDamageTakenBlurStrength_, 0.0005f, 0.0f, 0.1f);
		ParamEditor::RegisterValue("timeScale", &damageTakenTimeScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("invincibleDuration", &maxInvincibleDuration_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("blinkCount", &blinkCount_, 1.0f, 1, 20);
		ParamEditor::RegisterValue("minAlpha", &minAlpha_, 0.01f, 0.0f, 1.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("HpEffect");
		ParamEditor::RegisterValue("maxGrayScale", &maxGrayScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("maxNoise", &maxNoiseIntensity_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("startRatio", &hpEffectStartRaito_, 0.01f, 0.0f, 1.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("Attack");
		ParamEditor::RegisterValue("activeTime", &attackActiveTime_, 0.01f, 0.01f, 1.0f);
		ParamEditor::RegisterValue("cooldown", &attackCooldown_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("damage", &attackDamage_, 0.1f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("reach", &attackReach_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("offsetY", &attackOffsetY_, 0.01f, -5.0f, 5.0f);
		ParamEditor::RegisterValue("halfSize", &attackHalfSize_, 0.01f, 0.05f, 10.0f);
		ParamEditor::RegisterValue("spinEasePower", &spinEasePower_, 0.01f, 0.1f, 10.0f);
		ParamEditor::RegisterValue("spinDirection", &spinDirection_, 2.0f, -1.0f, 1.0f);
		ParamEditor::RegisterValue("spinTrailRadius", &spinTrailRadius_, 0.01f, 0.0f, 5.0f);
		ParamEditor::RegisterColor("attackColor", &attackColor_);
		ParamEditor::RegisterColor("cooldownColor", &cooldownColor_);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("ParryEffect");
		ParamEditor::RegisterValue("parryDuration", &parryEffectDuration_, 0.01f, 0.01f, 3.0f);
		ParamEditor::RegisterValue("parryPeakTime", &parryEffectPeakTime_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("parryMaxChromatic", &parryMaxChromatic_, 0.01f, 0.0f, 2.0f);
		ParamEditor::RegisterValue("parrySlowScale", &parrySlowScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("counterDuration", &counterEffectDuration_, 0.01f, 0.01f, 3.0f);
		ParamEditor::RegisterValue("counterPeakTime", &counterEffectPeakTime_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("counterMaxChromatic", &counterMaxChromatic_, 0.01f, 0.0f, 2.0f);
		ParamEditor::RegisterValue("counterSlowScale", &counterSlowScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("Gauge");
		ParamEditor::RegisterValue("max", &maxGauge_, 1.0f, 1.0f, 1000.0f);
		ParamEditor::RegisterValue("parryGain", &parryGaugeGain_, 0.5f, 0.0f, 1000.0f);
		ParamEditor::RegisterValue("counterGain", &counterGaugeGain_, 0.5f, 0.0f, 1000.0f);
		ParamEditor::RegisterValue("damagePenalty", &damageGaugePenalty_, 0.5f, 0.0f, 1000.0f);
		ParamEditor::RegisterValue("fullParryDamage", &fullParryDamage_, 0.5f, 0.0f, 1000.0f);
		ParamEditor::RegisterColor("chargeEmissiveColor", &chargeEmissiveColor_);
		ParamEditor::RegisterValue("chargeEmissiveIntensity", &chargeEmissiveIntensity_, 0.01f, 0.0f, 20.0f);
		ParamEditor::RegisterValue("maxPulseSpeed", &maxPulseSpeed_, 0.1f, 0.0f, 20.0f);
		ParamEditor::RegisterValue("maxPulseAmount", &maxPulseAmount_, 0.01f, 0.0f, 20.0f);
		ParamEditor::RegisterValue("fullParryDuration", &fullParryEffectDuration_, 0.01f, 0.01f, 3.0f);
		ParamEditor::RegisterValue("fullParryPeakTime", &fullParryEffectPeakTime_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("fullParryHoldTime", &fullParryEffectHoldTime_, 0.01f, 0.0f, 3.0f);
		ParamEditor::RegisterValue("fullParryMaxChromatic", &fullParryMaxChromatic_, 0.01f, 0.0f, 2.0f);
		ParamEditor::RegisterValue("fullParrySlowScale", &fullParrySlowScale_, 0.01f, 0.0f, 1.0f);
		ParamEditor::EndGroup();

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
		// ブラー
		float strength = EaseOutCubic(t, maxDamageTakenBlurStrength_, 0.0f);
		ppInstance->SetBlurStrength(strength);
		// スロー
		SetTimeScale(damageTakenTimeScale_);

		// 被ダメージ時間の終了
		if (damageTakenDuration_ <= 0.0f) {
			damageTakenDuration_ = 0.0f;
			ppInstance->SetBlurEnabled(false);
			ppInstance->SetBlurStrength(0.0f);
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
		} else if (isControllable_ && Input::TriggerKey(DIK_SPACE)) {
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

		// 0 → 1（peakTime） → 0（duration）の山なりの強さ（スローに使う）
		const float peak = std::clamp(effectPeakTime_, 0.0f, effectDuration_);
		// 一番遅い状態を保つ終わりの時間（フルパリィ以外は holdTime が0なので peak と同じ）
		const float holdEnd = std::clamp(peak + effectHoldTime_, peak, effectDuration_);
		float w = 0.0f;
		if (parryEffectTimer_ < peak) {
			w = parryEffectTimer_ / peak;
		} else if (parryEffectTimer_ < holdEnd) {
			w = 1.0f;
		} else if (effectDuration_ > holdEnd) {
			w = 1.0f - (parryEffectTimer_ - holdEnd) / (effectDuration_ - holdEnd);
		}
		w = std::clamp(w, 0.0f, 1.0f);

		auto ppInstance = PostProcess::GetInstance();
		// 色収差（始めが一番強く、終わりに向かって弱まる）
		const float chromaticT = effectDuration_ > 0.0f ? parryEffectTimer_ / effectDuration_ : 1.0f;
		ppInstance->SetChromaticAberrationStrength(EaseInOutCubic(chromaticT, effectMaxChromatic_, 0.0f));
		// スロー（被ダメージのスロー中はそちらを優先）
		if (!isDamageTaken_) {
			SetTimeScale(1.0f + (effectSlowScale_ - 1.0f) * w);
		}

		// 演出終了
		if (parryEffectTimer_ >= effectDuration_) {
			isParryEffect_ = false;
			parryEffectTimer_ = 0.0f;
			ppInstance->SetChromaticAberrationEnabled(false);
			ppInstance->SetChromaticAberrationStrength(0.0f);
			if (!isDamageTaken_) {
				SetTimeScale(1.0f);
			}
		}
	}

	void Player::GaugeUpdate() {
		const float ratio = GetGaugeRatio();

		// パーティクルの段階（エミッターを重ねて、溜まるほどパリパリが増えるようにする）
		isFirstCharge_ = ratio >= 1.0f / 3.0f;
		isSecondCharge_ = ratio >= 2.0f / 3.0f;
		isMaxCharge_ = IsGaugeMax();

		// 溜まるほど強く光る（ゲージ0で0、満タンで chargeEmissiveIntensity_）
		// 満タンのときは最大を超えないように下へ脈打つ（スローの影響を受けないように実時間）
		gaugeTimer_ += GetDeltaTime();
		float intensity = chargeEmissiveIntensity_ * ratio;
		if (isMaxCharge_) {
			const float wave = 0.5f + 0.5f * std::sin(gaugeTimer_ * maxPulseSpeed_ * 2.0f * std::numbers::pi_v<float>);
			intensity *= 1.0f - maxPulseAmount_ * wave;
		}
		model_->SetEmissive(chargeEmissiveColor_, intensity);
	}

}