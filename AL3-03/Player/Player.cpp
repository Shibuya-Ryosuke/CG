#include "Player.h"
#include "../../Original/Externals/imgui/imgui.h"
#include <algorithm>

using namespace RyoEngine;

void Player::Initialize() {
	model_ = Model::Create("resources/AL3-03/player.obj");
	model_->SetTex("resources/flower.png");
}

void Player::Update(DebugCamera& debugCamera) {
	// 座標取得
	Transform transform{
		.scale = model_->GetScale(),
		.rotate = model_->GetRotate(),
		.translate = model_->GetTranslate()
	};

	// Imgui
	ImGui::Begin("player");
	ImGui::DragFloat3("translate", &transform.translate.x, 0.1f, -1000.0f, 1000.0f);
	ImGui::End();

	// キャラクターの移動ベクトル
	Vector3 move{};

	// 押し方向で移動ベクトルを変更 (左右)
	if (Input::PushKey(DIK_LEFT)) {
		move.x -= kCharacterSpeed;
	} else if (Input::PushKey(DIK_RIGHT)) {
		move.x += kCharacterSpeed;
	}
	// 押した方向で移動ベクトルを変更 (上下)
	if (Input::PushKey(DIK_UP)) {
		move.y += kCharacterSpeed;
	} else if (Input::PushKey(DIK_DOWN)) {
		move.y -= kCharacterSpeed;
	}

	// 移動
	transform.translate += move;
	// 移動制限
	transform.translate.x = std::clamp(transform.translate.x, -kMoveLimit.x, kMoveLimit.x);
	transform.translate.y = std::clamp(transform.translate.y, -kMoveLimit.y, kMoveLimit.y);

	// セット
	model_->SetScale(transform.scale);
	model_->SetRotate(transform.rotate);
	model_->SetTranslate(transform.translate);

	// 更新
	model_->Update(debugCamera);
}

void Player::Update(Camera& camera) {
	// 座標取得
	Transform transform{
		.scale = model_->GetScale(),
		.rotate = model_->GetRotate(),
		.translate = model_->GetTranslate()
	};

	// Imgui
	ImGui::Begin("player");
	ImGui::DragFloat3("translate", &transform.translate.x, 0.1f, -1000.0f, 1000.0f);
	ImGui::End();

	// キャラクターの移動ベクトル
	Vector3 move{};

	// 押し方向で移動ベクトルを変更 (左右)
	if (Input::PushKey(DIK_LEFT)) {
		move.x -= kCharacterSpeed;
	} else if (Input::PushKey(DIK_RIGHT)) {
		move.x += kCharacterSpeed;
	}
	// 押した方向で移動ベクトルを変更 (上下)
	if (Input::PushKey(DIK_UP)) {
		move.y += kCharacterSpeed;
	} else if (Input::PushKey(DIK_DOWN)) {
		move.y -= kCharacterSpeed;
	}

	// 移動
	transform.translate += move;
	// 移動制限
	transform.translate.x = std::clamp(transform.translate.x, -kMoveLimit.x, kMoveLimit.x);
	transform.translate.y = std::clamp(transform.translate.y, -kMoveLimit.y, kMoveLimit.y);

	// セット
	model_->SetScale(transform.scale);
	model_->SetRotate(transform.rotate);
	model_->SetTranslate(transform.translate);

	model_->Update(camera);
}

void Player::Draw() {
	model_->Draw();
}
