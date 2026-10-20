#include "TestPlayer.h"
#include "TestScene.h"
#include <imgui.h>

using namespace RyoEngine;

namespace Test {
	void TestPlayer::Initialize(TestScene* scene) {
		scene_ = scene;

		// モデル初期化
		bunny_ = Model::Create("Resources/EngineResources/Test/bunny.obj");
		// 保存用の名前 (ギズモで動かした初期配置を Model Layout に保存するときのキー)
		bunny_->SetName("TestPlayer");
		// 初期値 (保存済みの配置があれば、最初のTransferMatrixでそちらが優先される)
		bunny_->SetScale({ 1.0f,1.0f,1.0f });
		bunny_->SetRotate({ 0.0f,160.0f,0.0f });   // 度数法 (旧 2.8rad ≒ 160°)
		bunny_->SetTranslateY(5.0f);


		// パラメータ操作の登録
		// SRT(transform)はModelが持ち、ギズモ+Model Layoutで管理するので登録しない
		ParamEditor::BeginGroup("bunny");
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
			bunny_->GetTransform().translate.x -= velocity_.x * GetDeltaTime();
		}
		if (Input::PushKey(DIK_D) || Input::PushKey(DIK_RIGHT)) {
			bunny_->GetTransform().translate.x += velocity_.x * GetDeltaTime();
		}

		// 前後
		if (Input::PushKey(DIK_W) || Input::PushKey(DIK_UP)) {
			bunny_->GetTransform().translate.z += velocity_.z * GetDeltaTime();
		}
		if (Input::PushKey(DIK_S) || Input::PushKey(DIK_DOWN)) {
			bunny_->GetTransform().translate.z -= velocity_.z * GetDeltaTime();
		}

		if (Input::PushKey(DIK_SPACE)) {
			if (scene_) {
				for (int32_t i = 0;i < 20;i++) {
					Vector3 randomPos{};
					randomPos.x = RandomFloat(-50.0f, 50.0f);
					randomPos.y = RandomFloat(1.0f, 10.0f);
					randomPos.z = RandomFloat(-50.0f, 50.0f);

					scene_->SpawnBullet(randomPos);
				}
			}
		}

		if (Input::TriggerKey(DIK_RSHIFT)) {
			isIllumination_ = !isIllumination_;
		}

		// モデル自体の色更新
		bunny_->SetColor(color_);
		// 発光更新
		bunny_->SetEmissive(emissiveColor_, emissiveIntensity_);

		// ライト追従
		LightManager::SetLightPosition("bunnyLight", bunny_->GetTranslate());
		// ライト点灯フラグ
		LightManager::SetLightEnabled("bunnyLight", isIllumination_);

		// 行列の確定 (カメラ不要。カリングはDraw()で行われる)
		bunny_->TransferMatrix();
	}

	void TestPlayer::Draw() {
		// 描画
		bunny_->Draw();
	}

}
