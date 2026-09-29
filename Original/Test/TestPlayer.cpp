#include "TestPlayer.h"
#include "TestManager.h"
#include <imgui.h>

using namespace RyoEngine;

void TestPlayer::Initialize(TestManager* manager) {
	manager_ = manager;

	// モデル初期化
	bunny_ = Model::Create("Resources/EngineResources/Test/bunny.obj");
	transform_.translate.y = 5.0f;
	transform_.scale = { 1.0f,1.0f,1.0f };
	transform_.rotate = { 0.0f,2.8f,0.0f };

	// ライト追加
	bunnyLightId_ = LightManager::AddLight(LightType::Point);

	// パラメータ操作の登録
	// SRT
	ParamEditor::BeginGroup("bunny");
	ParamEditor::RegisterValue("transform", &transform_);
	ParamEditor::RegisterColor("color", &color_);
	// 発光（周囲にライティングするわけではない）
	ParamEditor::BeginGroup("emissive");
	ParamEditor::RegisterValue("intensity", &emissiveIntensity_);
	ParamEditor::RegisterColor("color", &emissiveColor_);
	ParamEditor::EndGroup();
	ParamEditor::EndGroup(); // BeginGroupの数だけEndGroupを呼ぶ
}

void TestPlayer::Update() {

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
	// モデル自体の色更新
	bunny_->SetColor(color_);
	// 発光更新
	bunny_->SetEmissive(emissiveColor_, emissiveIntensity_);

	// ライト追従
	LightManager::SetLightPosition(bunnyLightId_, bunny_->GetTranslate());
}

void TestPlayer::Draw(RyoEngine::Camera& camera) {
	// 行列の確定
	bunny_->TransferMatrix(camera);

	// 描画
	bunny_->Draw();
}
