#include "TestPlayer.h"
#include "TestBullet.h"
#include "TestLayer.h"
#include <imgui.h>

using namespace RyoEngine;

namespace Test {
	void TestPlayer::Initialize() {
		// 名前 (GameObjectManager::FindByName 用) とタグ (当たった相手が見分ける用)
		SetName("Player");
		SetTag("Player");

		// 初期値 (座標は GameObject が持つ。動かすのは GetTransform())
		GetTransform().GetLocal().scale = { 1.0f,1.0f,1.0f };
		GetTransform().GetLocal().rotate = { 0.0f,160.0f,0.0f };   // 度数法 (旧 2.8rad ≒ 160°)
		GetTransform().GetLocal().translate.y = 5.0f;

		// モデル初期化
		bunny_ = Model::Create("Resources/EngineResources/Test/bunny.obj");
		// GameObject の座標にくっつける (モデル自体のSRTは 0 のまま)
		// ※ SetName は付けない (Model Layout の保存値が「親から見た値」として二重に掛かるため)
		bunny_->SetParent(&GetTransform());

		// 当たり判定 (回転する箱。bunny.obj の大きさに合わせている)
		// 自分はプレイヤー、敵と当たりたい
		AddOBBCollider({ 0.81f, 0.81f, 0.62f }, kLayerPlayer, kLayerEnemy, { -0.15f, 0.83f, 0.07f });

		
		// パラメータ操作の登録
		ParamEditor::BeginGroup("bunny");
		ParamEditor::RegisterColor("color", &color_);
		// 発光（周囲にライティングするわけではない）
		ParamEditor::BeginGroup("emissive");
		ParamEditor::RegisterValue("intensity", &emissiveIntensity_);
		ParamEditor::RegisterColor("color", &emissiveColor_);
		ParamEditor::EndGroup();
		ParamEditor::EndGroup(); // BeginGroupの数だけEndGroupを呼ぶ

		// 座標と、ライトの点灯フラグを登録する (ライト・パーティクルの ImGui で選べるようになる)
		// ライトの追従・点灯はここでは書かない。LightManager の ImGui で「追従する座標」「点灯フラグ」を選ぶ
		BindingRegistry::RegisterPosition("player pos", &GetTransform().GetLocal().translate);
		BindingRegistry::RegisterFlag("player light", &isIllumination_);

		ParamEditor::BeginGroup("TestPlayer");
		ParamEditor::RegisterValue("弾の発射間隔", &fireInterval_, 0.01f, 0.01f, 2.0f);
		ParamEditor::RegisterValue("弾の速さ", &bulletSpeed_, 0.1f, 0.0f, 200.0f);
		ParamEditor::RegisterValue("弾が消えるまでの時間", &bulletLifeTime_, 0.1f, 0.1f, 20.0f);
		ParamEditor::RegisterColor("敵に触れている間の色", &touchColor_);
		ParamEditor::RegisterValue("敵に触れている間の光の強さ", &touchIntensity_, 0.1f, 0.0f, 10.0f);
		ParamEditor::EndGroup();
	}

	void TestPlayer::Update() {
		Vector3& translate = GetTransform().GetLocal().translate;

		// 移動
		// 左右
		if (Input::PushKey(DIK_A) || Input::PushKey(DIK_LEFT)) {
			translate.x -= velocity_.x * GetDeltaTime();
		}
		if (Input::PushKey(DIK_D) || Input::PushKey(DIK_RIGHT)) {
			translate.x += velocity_.x * GetDeltaTime();
		}

		// 前後
		if (Input::PushKey(DIK_W) || Input::PushKey(DIK_UP)) {
			translate.z += velocity_.z * GetDeltaTime();
		}
		if (Input::PushKey(DIK_S) || Input::PushKey(DIK_DOWN)) {
			translate.z -= velocity_.z * GetDeltaTime();
		}

		// 弾 (押している間、一定間隔で前(+Z)へ撃つ)
		fireTimer_ -= GetDeltaTime();
		if (Input::PushKey(DIK_SPACE) && fireTimer_ <= 0.0f) {
			fireTimer_ = fireInterval_;
			const Vector3 muzzle = { translate.x, translate.y + 0.5f, translate.z };
			// GameObjectManager に作ってもらう (Update の途中で作ったものは、次のフレームから動く)
			GameObjectManager::Create<TestBullet>(muzzle, Vector3{ 0.0f, 0.0f, bulletSpeed_ }, bulletLifeTime_);
		}

		if (Input::TriggerKey(DIK_RSHIFT)) {
			isIllumination_ = !isIllumination_;
		}

		// モデル自体の色更新
		bunny_->SetColor(color_);
		// 発光更新 (敵に触れている間は赤く光る)
		if (touchingEnemyCount_ > 0) {
			bunny_->SetEmissive(touchColor_, touchIntensity_);
		} else {
			bunny_->SetEmissive(emissiveColor_, emissiveIntensity_);
		}

		// 行列の確定 (カメラ不要。カリングはDraw()で行われる)
		bunny_->TransferMatrix();
	}

	void TestPlayer::Draw() {
		// 描画
		bunny_->Draw();
	}

	void TestPlayer::Finalize() {
		// 消えた後に読みにこられないよう、登録を解除する
		BindingRegistry::Unregister("player pos");
		BindingRegistry::Unregister("player light");
	}

	void TestPlayer::OnCollisionEnter(GameObject* other) {
		if (other->GetTag() == "Enemy") {
			++touchingEnemyCount_;
		}
	}

	void TestPlayer::OnCollisionExit(GameObject* other) {
		if (other->GetTag() == "Enemy") {
			--touchingEnemyCount_;
		}
	}

}
