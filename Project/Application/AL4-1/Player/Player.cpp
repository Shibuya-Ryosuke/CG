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
		// 観察
		Observation();

		// --- 速度の更新 ---
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

		// modelにセット
		model_->SetTransform(transform_);
	}

	void Player::Draw(RyoEngine::Camera& camera) {
		// 行列の確定
		model_->TransferMatrix(camera);

		// 描画
		model_->Draw();
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

	void Player::Observation() {
		// ポストプロセスインスタンス
		auto instance = PostProcess::GetInstance();

		if (Input::TriggerKey(DIK_SPACE)) {
			isObsevation_ = !isObsevation_;

			if (isObsevation_) {
				// 白黒化
				instance->SetGrayscaleEnabled(true);
				instance->SetGrayscaleIntensity(observationGrayIntensity_);
				// 時間速度の低下
				SetTimeScale(observationTimeScale_);
			} else {
				// 白黒化解除
				instance->SetGrayscaleEnabled(false);
				instance->SetGrayscaleIntensity(0.0f);
				// 時間速度等倍
				SetTimeScale(1.0f);
			}
		}

		// ゲージの増減
		if (isObsevation_) {
			if (observationGuarge_ > 0.0f) {
				observationGuarge_ -= observationGuargeDecreaseValue_;
			}
		} else {
			if (observationGuarge_ < observationGuargeLimit_) {
				observationGuarge_ += observationGuargeIncreaseValue_;
			}
		}

		// 色収差の強さ計算（ゲージ量に連動）
		instance->SetChromaticAberrationEnabled(true);
		float chormaticAberrationStrength = observationGuarge_ / observationGuargeLimit_ - 0.5f;
		if (chormaticAberrationStrength >= 0.0f) {
			chormaticAberrationStrength = 0.0f;
		} else {
			// これで最大でも-1.000に収まる
			chormaticAberrationStrength /= 5.0f;
		}
		instance->SetChromaticAberrationStrength(chormaticAberrationStrength);
	}
}
