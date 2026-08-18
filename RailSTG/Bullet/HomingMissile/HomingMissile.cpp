#include "HomingMissile.h"
#include "../../Time/TimeEnum.h"
#include "../../Time/TimeManager.h"
#include "../../Enemy/BaseEnemy.h"
#include "../../GameMath/GameMath.h"

using namespace RyoEngine;

HomingMissile::HomingMissile() = default;

void HomingMissile::Initialize() {
	// デフォルトの初期化（必要であれば）
	if (model_ == nullptr) {
		model_ = Model::Create("resources/RailSTG/Bullet/bullet.obj"); // ミサイル用のモデルパス
		model_->SetTex("resources/uvChecker.png");
	}
}

void HomingMissile::Initialize(const RyoEngine::Vector3& spawnPos, BaseEnemy* targetEnemy) {
	Initialize();

	// 発生位置のセット
	model_->SetTranslate(spawnPos);
	model_->SetScale({ 0.5f,0.5f,0.5f });
	// ターゲットのセット
	targetEnemy_ = targetEnemy;

	// 初期速度（最初は前方に勢いよく飛び出すなど、お好みで調整）
	// 例としてカメラ前方や、上方向に少し飛び出す挙動にしてもカッコいいです
	velocity_ = { 0.0f, 5.0f, 10.0f };
}

void HomingMissile::Finalize() {}

void HomingMissile::Update(const RyoEngine::Camera& camera) {
	// 寿命の減少（BaseBulletの機能）
	BaseBullet::Update(camera);

	// ターゲットが存在し、かつ生存している場合のみホーミングする
	if (targetEnemy_ != nullptr && !targetEnemy_->IsDead()) {
		// 1. ミサイルの現在位置から敵への理想の方向ベクトル
		Vector3 toEnemy = targetEnemy_->GetWorldPos() - model_->GetTranslate();
		toEnemy = Normalize(toEnemy);

		// 2. 現在の進行方向（正規化）
		Vector3 currentDir = Normalize(velocity_);

		// 3. 旋回性能（turnRate_）を効かせて、少しずつ敵の方向へ曲げる
		// turnRate_ を小さくするほど大回り（無駄な曲がり）になります
		Vector3 newDir = Normalize(currentDir + (toEnemy - currentDir) * turnRate_);

		// 4. 速度ベクトルを更新
		velocity_ = newDir * speed_;
	}
	// ※もし途中でターゲットが消滅したら、最後の velocity_ のまま直進します

	// 座標の移動
	Vector3 translate = model_->GetTranslate();
	translate += velocity_ * TimeManager::GetDeltaTime();
	model_->SetTranslate(translate);

	model_->Update(camera);

	// OBBの更新
	UpdateOBB(obb_, model_.get());
}

void HomingMissile::Draw() {
	BaseBullet::Draw();
	// デバッグ用OBB表示（必要に応じて）
	PrimitiveRenderer::DrawOBB(obb_, { 1.0f, 0.5f, 0.0f, 1.0f }, PrimitiveDrawMode::Wireframe);
}