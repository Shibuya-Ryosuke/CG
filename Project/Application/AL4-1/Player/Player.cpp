#include "Player.h"

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	Player::Player() = default;
	Player::~Player() = default;

	void Player::Initialize() {
		model_ = Model::Create("Resources/ApplicationResources/AL4-1/Player/bunny.obj");
		RegisterInParamEditor();
	}

	void Player::Update() {
		Move();
		Jump();

		// 空中にいるとき
		if (!onGround_) {
			velocity_.y -= gravity_;

			transform_.translate.y += velocity_.y;


			// 地面以下になったとき（後々マップチップでやりたい）
			if (transform_.translate.y < 0.0f) {
				// 0に戻す
				transform_.translate.y = 0.0f;

				velocity_.y = 0.0f;
				onGround_ = true;
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
		ParamEditor::RegisterValue("velocity", &velocity_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("jumpPower", &jumpPower_, 0.1f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("gravity_", &gravity_, 0.01f, 0.0f, 10.0f);
	}

	void Player::Move() {
		if (Input::PushKey(DIK_D)) {
			transform_.translate.x += velocity_.x * GetScaleTime();
		}
		if (Input::PushKey(DIK_A)) {
			transform_.translate.x -= velocity_.x * GetScaleTime();
		}
	}

	void Player::Jump() {
		// 接地状態のみ判断
		if (onGround_) {
			if (Input::TriggerKey(DIK_W)) {
				velocity_.y = jumpPower_;
				onGround_ = false;
			}
		}
	}
}