#include "Player.h"
#include "../../Original/Externals/imgui/imgui.h"
#include <algorithm>

using namespace RyoEngine;

void Player::Initialize() {
	model_ = Model::Create("resources/AL3-03/player.obj");
	model_->SetTex("resources/flower.png");
}

void Player::Update(DebugCamera& debugCamera) {
	Translate();

	// 更新
	model_->Update(debugCamera);
}

void Player::Update(Camera& camera) {
	Translate();
	model_->Update(camera);
}

void Player::Draw() {
	model_->Draw();
}

void Player::Translate() {
	Vector3 translate = model_->GetTranslate();

	// Imgui
	ImGui::Begin("player");
	ImGui::DragFloat3("translate", &translate.x, 0.1f, -1000.0f, 1000.0f);
	ImGui::End();

	// キャラクターの移動ベクトル
	Vector3 move{};

	// 押し方向で移動ベクトルを変更 (左右)
	if (Input::PushKey(DIK_LEFT)) {
		move.x -= kCharacterSpeed_;
	} else if (Input::PushKey(DIK_RIGHT)) {
		move.x += kCharacterSpeed_;
	}
	// 押した方向で移動ベクトルを変更 (上下)
	if (Input::PushKey(DIK_UP)) {
		move.y += kCharacterSpeed_;
	} else if (Input::PushKey(DIK_DOWN)) {
		move.y -= kCharacterSpeed_;
	}

	// 移動
	translate += move;
	// 移動制限
	translate.x = std::clamp(translate.x, -kMoveLimit_.x, kMoveLimit_.x);
	translate.y = std::clamp(translate.y, -kMoveLimit_.y, kMoveLimit_.y);

	// セット
	model_->SetTranslate(translate);
}

void Player::Rotate() {
	// 回転取得
	Vector3 rotate = model_->GetRotate();

	if (Input::PushKey(DIK_A)) {
		rotate.y -= kRotSpeed_;
	} else if (Input::PushKey(DIK_D)) {
		rotate.y += kRotSpeed_;
	}

	// セット
	model_->SetRotate(rotate);
}
