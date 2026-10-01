#include "TestManager.h"

using namespace RyoEngine;

namespace Test {
	void TestManager::Initialize() {

		// モデル初期化
		ground_ = Model::Create("Resources/EngineResources/Test/ground.obj");
		sky_ = Model::Create("Resources/EngineResources/Test/skydome.obj");

		// テクスチャ初期化
		flower.Initialize("Resources/EngineResources/Test/flower.png");
		// 弾用のInstancedModel初期化
		bulletInstancedModel_.Initialize("Resources/EngineResources/Test/TR.obj", 5000);

		// プレイヤー初期化（自分自身のポインタを渡して、弾を発射できるようにする）
		player_.Initialize(this);

		ParamEditor::BeginGroup("sprite");
		ParamEditor::RegisterValue("transform", &transform_);
		ParamEditor::RegisterValue("uvTransform", &uvTransform_);
		ParamEditor::RegisterColor("color", &color_);
		ParamEditor::RegisterAnchor("anchor", &anchor_);
		ParamEditor::EndGroup();

		fireEmitter_.Initialize("Resources/EngineResources/Test/Particle/quad.obj", 2000, /*billboard=*/true, GPUParticleCommon::BlendMode::Additive);
		fireEmitter_.SetGravity(-2.0f); // 上昇する炎なら負の値(浮力っぽさ)も試してみてください
		fireEmitter_.Emit({0.0f,2.0f,0.0f}, {1.0f,2.0f,0.0f}, {1.0f, 0.6f, 0.1f, 1.0f}, 0.5f, {0,0,0}, 1.2f);
	}

	void TestManager::Update() {
		// （Normalはアルファブレンド）
		// 3Dオブジェクトたちのブレンドモード
		if (Input::TriggerKey(DIK_0)) {
			Change3DBlendMode(BlendMode::None);
		} else if (Input::TriggerKey(DIK_1)) {
			Change3DBlendMode(BlendMode::Normal);
		} else if (Input::TriggerKey(DIK_2)) {
			Change3DBlendMode(BlendMode::Add);
		} else if (Input::TriggerKey(DIK_3)) {
			Change3DBlendMode(BlendMode::Subtract);
		} else if (Input::TriggerKey(DIK_4)) {
			Change3DBlendMode(BlendMode::Multiply);
		} else if (Input::TriggerKey(DIK_5)) {
			Change3DBlendMode(BlendMode::Screen);
		}

		// 画像のブレンドモード
		if (Input::TriggerKey(DIK_Z)) {
			Change2DBlendMode(BlendMode::None);
		} else if (Input::TriggerKey(DIK_X)) {
			Change2DBlendMode(BlendMode::Normal);
		} else if (Input::TriggerKey(DIK_C)) {
			Change2DBlendMode(BlendMode::Add);
		} else if (Input::TriggerKey(DIK_V)) {
			Change2DBlendMode(BlendMode::Subtract);
		} else if (Input::TriggerKey(DIK_B)) {
			Change2DBlendMode(BlendMode::Multiply);
		} else if (Input::TriggerKey(DIK_N)) {
			Change2DBlendMode(BlendMode::Screen);
		}

		// 画像の更新
		flower.SetTransform2D(transform_);
		flower.SetUVTransform(uvTransform_);
		flower.SetColor(color_);
		flower.SetAnchor(anchor_);
		flower.TransferMatrix();

		fireEmitter_.Update(GetDeltaTime());
		// プレイヤー更新
		player_.Update();

		// すべての弾の更新
		for (auto& bullet : bullets_) {
			bullet.Update();
		}

		// 寿命などで死亡した弾を削除 (isDead_ が true のものを除去)
		bullets_.erase(
			std::remove_if(bullets_.begin(), bullets_.end(), [](const TestBullet& b) {
				return b.IsDead();
				}),
			bullets_.end()
		);

		// 最後に一括でバッファを更新
		bulletInstancedModel_.UpdateBuffer();
	}

	void TestManager::Draw() {
		// テスト要のやつなので、使用するカメラはデバッグカメラのみ
		// 行列の確定
		ground_->TransferMatrix(GetDebugCamera());
		sky_->TransferMatrix(GetDebugCamera());

		// 描画
		ground_->Draw();
		sky_->Draw();
		player_.Draw(GetDebugCamera());

		flower.Draw();
		// 弾のインスタンス描画予約
		bulletInstancedModel_.Draw(GetDebugCamera());

		fireEmitter_.Draw(GetDebugCamera());
	}

	void TestManager::SpawnBullet(const Vector3& position) {
		TestBullet newBullet;
		// TestBullet::Initialize 内で bulletInstancedModel_ を使って AddInstance する
		newBullet.Initialize(&bulletInstancedModel_, position);
		bullets_.push_back(newBullet);
	}
}