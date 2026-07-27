#include "Enemy.h"
#include "EnemyState.h"
#include "../Math.h"

using namespace RyoEngine;

// --- 接近状態 (Approach) ---
EnemyStateApproach* EnemyStateApproach::GetInstance() {
	static EnemyStateApproach instance;
	return &instance;
}

void EnemyStateApproach::Update(Enemy* enemy) {
	// パブリックなゲッター経由で計算
	Vector3 approachVelocity = enemy->GetVelocity() * enemy->GetApproachSpeedRate();

	// 専用の関数経由で移動させる
	enemy->MoveTranslate(approachVelocity);

	// 専用の関数経由でZ座標をチェック
	if (enemy->GetPositionZ() < 0.0f) {
		enemy->ChangeState(EnemyStateLeave::GetInstance());
	}
}

// --- 離脱状態 (Leave) ---
EnemyStateLeave* EnemyStateLeave::GetInstance() {
	static EnemyStateLeave instance;
	return &instance;
}

void EnemyStateLeave::Update(Enemy* enemy) {
	Vector3 leaveVelocity{
		.x = -enemy->GetMoveSpeed(),
		.y = enemy->GetMoveSpeed(),
		.z = -enemy->GetMoveSpeed(),
	};
	leaveVelocity *= enemy->GetLeaveSpeedRate();

	// 専用の関数経由で移動させる
	enemy->MoveTranslate(leaveVelocity);
}

void Enemy::MoveTranslate(const Vector3& translation) {
	if (model_) {
		model_->SetTranslate(model_->GetTranslate() + translation);
	}
}

void Enemy::Fire() {
	// 現在の座標をコピー
	Vector3 position = model_->GetTranslate();

	// 弾を生成
	EnemyBullet* newBullet = new EnemyBullet();
	// 弾の速度
	Vector3 velocity(0, 0, -newBullet->GetBulletSpeed());
	// 速度ベクトルを自機の向きに合わせて回転させる
	velocity = TransformNormal(velocity, model_->GetWorldMatrix());
	// 初期化
	newBullet->Initialize(position, velocity);
	// 弾を登録
	bullets_.push_back(newBullet);
}

float Enemy::GetPositionZ() const {
	return model_ ? model_->GetTranslate().z : 0.0f;
}


Enemy::~Enemy() {
	delete model_;
	model_ = nullptr;
}

void Enemy::Initialize(const Vector3& position, const Vector3& velocity) {
	model_ = Model::Create("resources/AL3-03/enemy/enemy.obj");
	model_->SetTex("resources/AL3-03/enemy/enemy.png");

	model_->SetTranslate(position);
	velocity_ = velocity;

	state_ = EnemyStateApproach::GetInstance();
	Fire();
}

void Enemy::ChangeState(IEnemyState* newState) {
	if (newState) {
		state_ = newState;
	}
}

void Enemy::UpdateState() {
	if (state_) {
		state_->Update(this);
	}
}

void Enemy::Update(RyoEngine::Camera& camera) {
	bullets_.remove_if([](EnemyBullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
		});

	UpdateState();
	model_->Update(camera);

	for (EnemyBullet* bullet : bullets_) {
		bullet->Update(camera);
	}
}

void Enemy::Update(RyoEngine::DebugCamera& debugCamera) {
	bullets_.remove_if([](EnemyBullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
		});

	UpdateState();
	model_->Update(debugCamera);

	for (EnemyBullet* bullet : bullets_) {
		bullet->Update(debugCamera);
	}
}

void Enemy::Draw() {
	model_->Draw();
	// 弾描画
	for (EnemyBullet* bullet : bullets_) {
		bullet->Draw();
	}
}
