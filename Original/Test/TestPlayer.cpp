#include "TestPlayer.h"
#include "TestManager.h"
#include <imgui.h>

using namespace RyoEngine;

void TestPlayer::Initialize(TestManager* manager) {
	manager_ = manager;

	// モデル初期化
	bunny_ = Model::Create("resources/test/bunny.obj");
	transform_.translate.y = 5.0f;
}

void TestPlayer::Update() {
#ifdef _DEBUG
	ImGui::Begin("test");
	// SRT
	ImGui::DragFloat3("bunny scale", &transform_.scale.x, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat3("bunny rotate", &transform_.rotate.x, 0.1f, -100.0f, 100.0f);
	ImGui::DragFloat3("bunny translate", &transform_.translate.x, 0.1f, -100.0f, 100.0f);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// 発光（周囲にライティングするわけではない）
	ImGui::DragFloat("emissive intensity", &emissiveIntensity_, 0.01f, 0.0f, 100.0f);
	ImGui::ColorEdit3("emissive color", &emissiveColor_.x);
	ImGui::End();

	// 発光セット
	bunny_->SetEmissive(emissiveColor_, emissiveIntensity_);
#endif

	// 移動
	// 左右
	if (Input::PushKey(DIK_A || DIK_LEFT)) {
		transform_.translate.x -= velocity_.x;
	}
	if (Input::PushKey(DIK_D || DIK_RIGHT)) {
		transform_.translate.x += velocity_.x;
	}

	// 前後
	if (Input::PushKey(DIK_W || DIK_UP)) {
		transform_.translate.z += velocity_.z;
	}
	if (Input::PushKey(DIK_S || DIK_DOWN)) {
		transform_.translate.z -= velocity_.z;
	}

	if (Input::TriggerKey(DIK_SPACE)) {
		if (manager_) {
			// プレイヤーの座標から少し前方に発射するなどの調整も可能
			manager_->SpawnBullet(transform_.translate);
		}
	}

	// SRT更新
	bunny_->SetTransform(transform_);
}

void TestPlayer::Draw(RyoEngine::Camera camera) {
	// 行列の確定
	bunny_->TransferMatrix(camera);

	// 描画
	bunny_->Draw();
}
