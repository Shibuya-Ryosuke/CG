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

		// カメラ初期化 (最初に作ったカメラが自動でアクティブになる)
		mainCam_ = CameraManager::CreateCamera("Main");
		CameraManager::GetCamera(mainCam_)->SetRotate({ 0.2f, 0.0f, 0.0f }); // 少し見下ろす
		subCam_ = CameraManager::CreateCamera("Sub");
		CameraManager::GetCamera(subCam_)->SetTranslate({ 0.0f, 40.0f, -40.0f });
		CameraManager::GetCamera(subCam_)->SetRotate({ 0.8f, 0.0f, 0.0f });  // 俯瞰

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

		// 地面と空の行列の確定 (カメラ不要。カリングはDraw()で行われる)
		ground_->TransferMatrix();
		sky_->TransferMatrix();

		// --- カメラ ---
		// 追従カメラはプレイヤーが動いた後に位置を合わせる(遅れないように)
		CameraManager::GetCamera(mainCam_)->SetTranslate(player_.GetTranslate() + followOffset_);
		// F1:俯瞰カメラへ一時的に切り替え / F2:元に戻す
		if (Input::TriggerKey(DIK_F1)) {
			CameraManager::PushActive(subCam_);
		}
		if (Input::TriggerKey(DIK_F2)) {
			CameraManager::PopActive();
		}
		// NOTE: カメラの行列計算・描画カメラの確定は、game->Update()の後にエンジン側(EndUpdate)で行われる

		// gpuパーティクル
		GPUParticleManager::GetInstance()->Update(GetScaleTime());
	}

	void TestManager::Draw() {
		// 描画 (カメラはEndUpdateで確定済みの描画カメラが自動で使われる)
		ground_->Draw();
		sky_->Draw();
		player_.Draw();

		flower.Draw();
		// 弾の描画提出 (同じモデル・テクスチャは自動でまとめて描画される)
		for (auto& bullet : bullets_) {
			bullet->Draw();
		}

		//fireEmitter_.Draw(GetDebugCamera());

		// gpuパーティクル
		GPUParticleManager::GetInstance()->Draw();
	}

	void TestManager::SpawnBullet(const Vector3& position) {
		auto bullet = std::make_unique<TestBullet>();
		bullet->Initialize(bulletModelIndex_, position);
		bullets_.push_back(std::move(bullet));
	}
}
