#include "TestPlayer.h"
#include "TestManager.h"
#include <imgui.h>

using namespace RyoEngine;

void TestPlayer::Initialize(TestManager* manager) {
	manager_ = manager;

	// モデル初期化
	bunny_ = Model::Create("resources/test/bunny.obj");
	transform_.translate.y = 5.0f;
	transform_.scale = { 1.0f,1.0f,1.0f };
	transform_.rotate = { 0.0f,2.8f,0.0f };

	bunnyLightId_ = LightManager::AddLight(LightType::Point);
}

void TestPlayer::Update() {
#ifdef _DEBUG
	ImGui::Begin("test");
	// SRT
	ImGui::DragFloat3("bunny scale", &transform_.scale.x, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat3("bunny rotate", &transform_.rotate.x, 0.01f, -100.0f, 100.0f);
	ImGui::DragFloat3("bunny translate", &transform_.translate.x, 0.01f, -100.0f, 100.0f);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Text("isVisible %d", bunny_->IsVisible());
	// 発光（周囲にライティングするわけではない）
	ImGui::DragFloat("emissive intensity", &emissiveIntensity_, 0.01f, 0.0f, 100.0f);
	ImGui::ColorEdit3("emissive color", &emissiveColor_.x);
	ImGui::End();

	// 発光セット
	bunny_->SetEmissive(emissiveColor_, emissiveIntensity_);
#endif

	// 移動
	// 左右
	if (Input::PushKey(DIK_A) || Input::PushKey(DIK_LEFT)) {
		transform_.translate.x -= velocity_.x * GetDeltaTime();
	}
	if (Input::PushKey(DIK_D) || Input::PushKey(DIK_RIGHT)) {
		transform_.translate.x += velocity_.x * GetDeltaTime();
	}

	// 前後
	if (Input::PushKey(DIK_W) || Input::PushKey(DIK_UP)) {
		transform_.translate.z += velocity_.z * GetDeltaTime();
	}
	if (Input::PushKey(DIK_S) || Input::PushKey(DIK_DOWN)) {
		transform_.translate.z -= velocity_.z * GetDeltaTime();
	}

	if (Input::PushKey(DIK_SPACE)) {
		if (manager_) {
			for (int32_t i = 0;i < 20;i++) {
				Vector3 randomPos{};
				randomPos.x = RandomFloat(-50.0f, 50.0f);
				randomPos.y = RandomFloat(1.0f, 10.0f);
				randomPos.z = RandomFloat(-50.0f, 50.0f);

				manager_->SpawnBullet(randomPos);
			}
		}
	}

	// SRT更新
	bunny_->SetTransform(transform_);

	// ライト追従
	LightManager::SetLightPosition(bunnyLightId_, bunny_->GetTranslate());

}

void TestPlayer::Draw(RyoEngine::Camera camera) {
	// 行列の確定
	bunny_->TransferMatrix(camera);

	// 描画
	bunny_->Draw();
}
