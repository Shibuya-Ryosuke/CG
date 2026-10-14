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
		// 無敵時間のアップデート
		InvincibleUpdate();
		// 被ダメージ時のアップデート
		DamageTakenUpdate();

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
		// modelにセット
		model_->SetTransform(transform_);
	}

	void Player::Draw(RyoEngine::Camera& camera) {
		// 行列の確定
		model_->TransferMatrix(camera);

		// 描画
		model_->Draw();
		PrimitiveRenderer::DrawOBB(obb_, { 0.0f,1.0f,0.0f,1.0f },PrimitiveDrawMode::Wireframe);
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

	void Player::RegisterInParamEditor() {
		ParamEditor::BeginGroup("Player");
		ParamEditor::RegisterValue("transform", &transform_);
		ParamEditor::RegisterValue("moveSpeed", &moveSpeed_, 0.1f, 0.0f, 20.0f);
		ParamEditor::RegisterValue("jumpPower", &jumpPower_, 0.1f, 0.0f, 40.0f);
		ParamEditor::RegisterValue("gravity_", &gravity_, 0.1f, 0.0f, 100.0f);
		ParamEditor::EndGroup();
	}

	void Player::Jump() {
		// 接地状態のみ判断
		if (onGround_ && Input::TriggerKey(DIK_W)) {
			velocity_.y = jumpPower_;
			onGround_ = false;
		}
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
		// 無敵終了
		if (invincibleDuration_ <= 0.0f) {
			invincibleDuration_ = 0.0f;
			isInvincible_ = false;
		}
	}

}
