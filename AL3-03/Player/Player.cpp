#include "Player.h"
#include "../../Original/Externals/imgui/imgui.h"
#include "../Math.h"
#include <algorithm>

using namespace RyoEngine;

Player::~Player() {
	// bulletの開放
	for (PlayerBullet* bullet : bullets_) {
		delete bullet;
	}
	bullets_.clear();
}

void Player::Initialize() {
	model_ = Model::Create("resources/AL3-03/player/player.obj");
	model_->SetTex("resources/AL3-03/player/player.png");
}

void Player::Update(DebugCamera& debugCamera) {
	// デスフラグの立った弾を削除
	bullets_.remove_if([](PlayerBullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
	});

	// 各種処理
	Rotate();
	Translate();
	Attack();

	// 更新
	model_->Update(debugCamera);
	// 弾更新
	for (PlayerBullet * bullet : bullets_) {
		bullet->Update(debugCamera);
	}
}

void Player::Update(Camera& camera) {
	bullets_.remove_if([](PlayerBullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
	});

	Rotate();
	Translate();
	Attack();

	model_->Update(camera);
	for (PlayerBullet* bullet : bullets_) {
		bullet->Update(camera);
	}
}

void Player::Draw() {
	model_->Draw();

	// 弾描画
	for (PlayerBullet* bullet : bullets_) {
		bullet->Draw();
	}
}

void Player::Translate() {
	Vector3 translate = model_->GetTranslate();

	// Imgui
	ImGui::Begin("player");
	ImGui::DragFloat3("translate", &translate.x, 0.1f, -1000.0f, 1000.0f);
	ImGui::End();

	// キャラクターの移動ベクトル
	Vector3 move{};

	// 押し方向で移動ベクトルを変更 (左右)
	if (Input::PushKey(DIK_LEFT)) {
		move.x -= kMoveSpeed_;
	} else if (Input::PushKey(DIK_RIGHT)) {
		move.x += kMoveSpeed_;
	}
	// 押した方向で移動ベクトルを変更 (上下)
	if (Input::PushKey(DIK_UP)) {
		move.y += kMoveSpeed_;
	} else if (Input::PushKey(DIK_DOWN)) {
		move.y -= kMoveSpeed_;
	}

	// 移動
	translate += move;
	// 移動制限
	translate.x = std::clamp(translate.x, -kMoveLimit_.x, kMoveLimit_.x);
	translate.y = std::clamp(translate.y, -kMoveLimit_.y, kMoveLimit_.y);

	// セット
	model_->SetTranslate(translate);
}

void Player::Attack() {
	if (Input::TriggerKey(DIK_SPACE)) {
		// 自キャラの座標をコピー
		Vector3 position = model_->GetTranslate();

		// 弾を生成
		PlayerBullet* newBullet = new PlayerBullet();
		// 弾の速度
		Vector3 velocity(0, 0, newBullet->GetBulletSpeed());
		// 速度ベクトルを自機の向きに合わせて回転させる
		velocity = TransformNormal(velocity, model_->GetWorldMatrix());
		// 初期化
		newBullet->Initialize(position,velocity);
		// 弾を登録
		bullets_.push_back(newBullet);
	}
}

void Player::Rotate() {
	// 回転取得
	Vector3 rotate = model_->GetRotate();

	if (Input::PushKey(DIK_A)) {
		rotate.y -= kRotSpeed_;
	} else if (Input::PushKey(DIK_D)) {
		rotate.y += kRotSpeed_;
	}

	// セット
	model_->SetRotate(rotate);
}
