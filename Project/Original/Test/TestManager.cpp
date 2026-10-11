#include "TestManager.h"
#include <algorithm>

using namespace RyoEngine;

namespace Test {
	void TestManager::Initialize() {

		// モデル初期化
		ground_ = Model::Create("Resources/EngineResources/Test/ground.obj");
		sky_ = Model::Create("Resources/EngineResources/Test/skydome.obj");

		// テクスチャ初期化
		flower.Initialize("Resources/EngineResources/Test/flower.png");
		// 弾用モデルを1回だけ読み込む (modelIndex を全弾で共有)
		bulletModelIndex_ = ModelManager::GetInstance()->LoadObj("Resources/EngineResources/Test/TR.obj");

		// プレイヤー初期化（自分自身のポインタを渡して、弾を発射できるようにする）
		player_.Initialize(this);

		ParamEditor::BeginGroup("sprite");
		ParamEditor::RegisterValue("transform", &transform_);
		ParamEditor::RegisterValue("uvTransform", &uvTransform_);
		ParamEditor::RegisterColor("color", &color_);
		ParamEditor::RegisterAnchor("anchor", &anchor_);
		ParamEditor::EndGroup();



		// gpuパーティクルマネージャーに登録
		GPUParticleManager::GetInstance()->RegisterFlag("test emit", &particleEmit_);
		GPUParticleManager::GetInstance()->RegisterPosition("tracked pos", &player_.GetTranslate());

		// ↓ パーティクルのobjファイルパス
		// Resources/EngineResources/Test/Particle/quad.obj
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

		if (Input::TriggerKey(DIK_L)) {
			particleEmit_ = !particleEmit_;
		}
		//fireEmitter_.Update(GetDeltaTime());
		// プレイヤー更新
		player_.Update();

		// すべての弾の更新
		for (auto& bullet : bullets_) {
			bullet->Update();
		}

		// 寿命などで死亡した弾を削除 (isDead_ が true のものを除去)
		bullets_.erase(
			std::remove_if(bullets_.begin(), bullets_.end(),
				[](const std::unique_ptr<TestBullet>& bullet) {
					return bullet->IsDead(); // 削除条件
				}),
			bullets_.end()
		);

		// gpuパーティクル
		GPUParticleManager::GetInstance()->Update(GetScaleTime());
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
		// 弾の描画提出 (同じモデル・テクスチャは自動でまとめて描画される)
		for (auto& bullet : bullets_) {
			bullet->Draw(GetDebugCamera());
		}

		//fireEmitter_.Draw(GetDebugCamera());

		// gpuパーティクル
		GPUParticleManager::GetInstance()->Draw(GetDebugCamera());
	}

	void TestManager::SpawnBullet(const Vector3& position) {
		auto bullet = std::make_unique<TestBullet>();
		bullet->Initialize(bulletModelIndex_, position);
		bullets_.push_back(std::move(bullet));
	}
}