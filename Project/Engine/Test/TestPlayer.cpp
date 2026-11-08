#include "TestPlayer.h"
#include "TestBullet.h"
#include "TestLayer.h"
#include <algorithm>

using namespace RyoEngine;

namespace Test {
	void TestPlayer::Initialize() {
		SetName("Player"); // GameObjectManager::FindByName 用
		SetTag("Player");  // 当たった相手が見分ける用

		// 座標は GameObject が持つ (回転は度)
		GetTransform().GetLocal().rotate = { 0.0f, 160.0f, 0.0f };
		GetTransform().GetLocal().translate.y = 5.0f;

		// モデルを GameObject にくっつける (SetName は付けない。Model Layout の保存値が二重にかかるため)
		bunny_ = Model::Create("Resources/EngineResources/Test/bunny.obj");
		bunny_->SetParent(&GetTransform());

		// 当たり判定: 回転する箱。自分は Player、Enemy と当たる
		AddOBBCollider({ 0.81f, 0.81f, 0.62f }, kLayerPlayer, kLayerEnemy, { -0.15f, 0.83f, 0.07f });

		// ライト・パーティクルから選べるように登録 (どれを使うかは各 ImGui で選ぶ)
		BindingRegistry::RegisterPosition("player pos", &GetTransform().GetLocal().translate);
		BindingRegistry::RegisterFlag("player light", &isIllumination_);

		ParamEditor::BeginGroup("【3D】プレイヤー");
		ParamEditor::RegisterColor("色", &color_);
		ParamEditor::RegisterValue("発光の強さ", &emissiveIntensity_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterColor("発光の色", &emissiveColor_);
		ParamEditor::RegisterValue("移動の速さ", &velocity_, 0.1f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("弾の発射間隔", &fireInterval_, 0.01f, 0.01f, 2.0f);
		ParamEditor::RegisterValue("弾の速さ", &bulletSpeed_, 0.1f, 0.0f, 200.0f);
		ParamEditor::RegisterValue("弾が消えるまでの時間", &bulletLifeTime_, 0.1f, 0.1f, 20.0f);
		ParamEditor::RegisterColor("敵に触れている間の色", &touchColor_);
		ParamEditor::RegisterValue("敵に触れている間の光の強さ", &touchIntensity_, 0.1f, 0.0f, 10.0f);
		ParamEditor::EndGroup();
	}

	void TestPlayer::Update() {
		Move();
		Shoot();

		if (Input::TriggerKey(DIK_RSHIFT)) {
			isIllumination_ = !isIllumination_;
		}

		// 見た目 (敵に触れている間は赤く光る)
		bunny_->SetColor(color_);
		if (touchingEnemyCount_ > 0) {
			bunny_->SetEmissive(touchColor_, touchIntensity_);
		} else {
			bunny_->SetEmissive(emissiveColor_, emissiveIntensity_);
		}

		bunny_->TransferMatrix(); // 行列の確定 (カリングは Draw で行われる)
	}

	void TestPlayer::Move() {
		// キーボード (-1〜1) とゲームパッドの左スティック (-1〜1) を足す
		Vector2 input = { Input::GetLeftStickX(), Input::GetLeftStickY() };
		if (Input::PushKey(DIK_A) || Input::PushKey(DIK_LEFT)) { input.x -= 1.0f; }
		if (Input::PushKey(DIK_D) || Input::PushKey(DIK_RIGHT)) { input.x += 1.0f; }
		if (Input::PushKey(DIK_W) || Input::PushKey(DIK_UP)) { input.y += 1.0f; }
		if (Input::PushKey(DIK_S) || Input::PushKey(DIK_DOWN)) { input.y -= 1.0f; }
		input.x = std::clamp(input.x, -1.0f, 1.0f);
		input.y = std::clamp(input.y, -1.0f, 1.0f);

		Vector3& translate = GetTransform().GetLocal().translate;
		translate.x += input.x * velocity_.x * GetDeltaTime();
		translate.z += input.y * velocity_.z * GetDeltaTime();
	}

	void TestPlayer::Shoot() {
		// 押している間、一定間隔で撃つ
		fireTimer_ -= GetDeltaTime();
		if (!Input::PushKey(DIK_SPACE) || fireTimer_ > 0.0f) {
			return;
		}
		fireTimer_ = fireInterval_;

		// Update 中に作った GameObject は、次のフレームから動く
		const Vector3& translate = GetTranslate();
		const Vector3 muzzle = { translate.x, translate.y + 0.5f, translate.z };
		GameObjectManager::Create<TestBullet>(muzzle, Vector3{ 0.0f, 0.0f, bulletSpeed_ }, bulletLifeTime_);
	}

	void TestPlayer::Draw() {
		bunny_->Draw();
	}

	void TestPlayer::Finalize() {
		// 消えた後に読まれないよう、登録を解除する
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
