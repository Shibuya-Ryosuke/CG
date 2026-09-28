#include "TestManager.h"

using namespace RyoEngine;

void TestManager::Initialize() {
	// モデル初期化
	ground_ = Model::Create("Resources/EngineResources/Test/ground.obj");
	sky_ = Model::Create("Resources/EngineResources/Test/skydome.obj");

	// テクスチャ初期化
	flower.Initialize("Resources/EngineResources/Test/flower.png");
	// 弾用のInstancedModel初期化
	bulletInstancedModel_.Initialize("Resources/EngineResources/Test/TR.obj",5000);

	// プレイヤー初期化（自分自身のポインタを渡して、弾を発射できるようにする）
	player_.Initialize(this);

	ParamEditor::BeginGroup("sprite");
	ParamEditor::BeginGroup("transform");
	ParamEditor::RegisterValue("scale", &scale_);
	ParamEditor::RegisterValue("rotate", &rotate_);
	ParamEditor::RegisterValue("translate", &translate_,1.0f,0.0f,1280.0f);
	ParamEditor::EndGroup();
	ParamEditor::BeginGroup("uvTransform");
	ParamEditor::RegisterValue("scale", &uvScale_);
	ParamEditor::RegisterValue("rotate", &uvRotate_);
	ParamEditor::RegisterValue("translate", &uvTranslate_,1.0f,0.0f,1280.0f);
	ParamEditor::EndGroup();
	ParamEditor::RegisterColor("color",&color_);
	ParamEditor::EndGroup();
}

void TestManager::Update() {
	// 画像の更新
	flower.SetTranslate(translate_);
	flower.SetRotate(rotate_);
	flower.SetScale(scale_);
	flower.SetUVTranslate(uvTranslate_);
	flower.SetUVRotate(uvRotate_);
	flower.SetUVScale(uvScale_);
	flower.SetColor(color_);
	flower.TransferMatrix();

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

void TestManager::Draw(RyoEngine::Camera camera) {
	// 行列の確定
	ground_->TransferMatrix(camera);
	sky_->TransferMatrix(camera);

	// 描画
	ground_->Draw();
	sky_->Draw();
	player_.Draw(camera);

	flower.Draw();
	// 弾のインスタンス描画予約
	bulletInstancedModel_.Draw(camera);
}

void TestManager::SpawnBullet(const Vector3& position) {
	TestBullet newBullet;
	// TestBullet::Initialize 内で bulletInstancedModel_ を使って AddInstance する
	newBullet.Initialize(&bulletInstancedModel_, position);
	bullets_.push_back(newBullet);
}