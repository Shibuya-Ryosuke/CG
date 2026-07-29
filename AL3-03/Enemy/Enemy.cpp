#include "Enemy.h"
#include "EnemyState.h"
#include "../Math.h"
#include <cassert>

#include "../Player/Player.h"

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

	// 発射タイマーカウントダウン
	enemy->CountDownFire();
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

void Enemy::CountDownFire() {
	fireTimer--;
	if (fireTimer <= 0) {
		Fire();
		fireTimer = kFireInterval_;
	}
}

void Enemy::Fire() {
	assert(player_);

	// 弾の速さ（調整項目）
	const float kBulletSpeed = 1.0f; // 必要に応じて数値は調整してください

	// 自キャラのワールド座標を取得する
	Vector3 playerPosition = player_->GetWorldposition();
	// 敵キャラのワールド座標を取得する
	Vector3 position = GetWorldposition();

	// 敵キャラから自キャラへの差分ベクトルを求める
	Vector3 velocity = playerPosition - position;

	// ベクトルの正規化
	velocity = Normalize(velocity);

	// ベクトルの長さを、速さに合わせる
	velocity *= kBulletSpeed;

	// 弾を生成し、初期化
	EnemyBullet* newBullet = new EnemyBullet();
	newBullet->Initialize(position, velocity);

	// 弾を登録する
	bullets_.push_back(newBullet);
}

float Enemy::GetPositionZ() const {
	return model_ ? model_->GetTranslate().z : 0.0f;
}


Enemy::~Enemy() {
	// bulletの開放
	for (EnemyBullet* bullet : bullets_) {
		delete bullet;
	}
	bullets_.clear();

	delete model_;
	model_ = nullptr;
}

void Enemy::Initialize(const Vector3& position, const Vector3& velocity) {
	model_ = Model::Create("resources/AL3-03/enemy/enemy.obj");
	model_->SetTex("resources/AL3-03/enemy/enemy.png");

	model_->SetTranslate(position);
	velocity_ = velocity;

	state_ = EnemyStateApproach::GetInstance();
	ApproachPhaseInitialize();
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
