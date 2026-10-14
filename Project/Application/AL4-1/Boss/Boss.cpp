#include "Boss.h"
#include <algorithm>

using namespace RyoEngine;

namespace AL4_1 {
	Boss::Boss() = default;
	Boss::~Boss() = default;

	void Boss::Initialize() {
		model_ = Model::Create("Resources/ApplicationResources/AL4-1/Boss/boss.obj");

		hp_ = maxHp_;
		attackType_ = AttackType::None;
		prevAttackType_ = AttackType::None;
		transform_.scale = baseScale_;

		ChangeState(State::Appear);
	}

	void Boss::Update() {
		stateTimer_ += kDeltaTime;

		switch (state_) {
		case State::Appear: 
			UpdateAppear();
			break;
		case State::Idle:
			UpdateIdle();
			break;
		case State::Move:
			UpdateMove();
			break;
		case State::Attack:
			UpdateAttack();
			break;
		case State::Destroy:
			UpdateDestroy();
			break;
		case State::Dead:
			break;
		default:
			break;
		}

		obb_.center = transform_.translate;
		model_->SetTransform(transform_);
	}

	void Boss::Draw(RyoEngine::Camera& camera) {
		if (state_ == State::Dead) { return; }
		model_->TransferMatrix(camera);
		model_->Draw();
	}

	void Boss::OnDamage(float damage) {
		if (!IsCollidable()) { return; }

		hp_ -= damage;
		if (hp_ <= 0.0f) {
			hp_ = 0.0f;
			ChangeState(State::Destroy);
		}
	}

	//==================================================
	// 状態管理
	//==================================================

	void Boss::ChangeState(State next) {
		state_ = next;
		stateTimer_ = 0.0f;

		// 状態に入った瞬間の処理
		switch (state_) {
		case State::Appear:
			transform_.translate = homePosition_ + appearOffset_;
			transform_.scale = baseScale_;
			break;

		case State::Move:
			moveStart_ = transform_.translate;
			moveEnd_ = DecideMovePosition();
			break;

		case State::Attack:
			StartAttack();
			break;

		case State::Destroy:
			destroyBasePosition_ = transform_.translate;
			attackType_ = AttackType::None;
			break;

		default:
			break;
		}
	}

	void Boss::UpdateAppear() {
		// 上から降りてくる
		float t = std::clamp(stateTimer_ / appearTime_, 0.0f, 1.0f);
		transform_.translate = Lerp(homePosition_ + appearOffset_, homePosition_, EaseOutCubic(t,0.0f,1.0f));

		if (t >= 1.0f) {
			ChangeState(State::Idle);
		}
	}

	void Boss::UpdateIdle() {
		if (stateTimer_ >= idleTime_) {
			ChangeState(State::Move);
		}
	}

	void Boss::UpdateMove() {
		float t = std::clamp(stateTimer_ / moveTime_, 0.0f, 1.0f);
		transform_.translate = Lerp(moveStart_, moveEnd_, EaseInOutCubic(t,0.0f,1.0f));

		if (t >= 1.0f) {
			ChangeState(State::Attack);
		}
	}

	void Boss::UpdateAttack() {
		switch (attackType_) {
		case AttackType::Shot:
			Shot();
			break;
		case AttackType::HomingShot:
			HomingShot();
			break;
		case AttackType::Rush:
			Rush();
			break;
		default:
			break;
		}

		if (stateTimer_ >= attackTime_[static_cast<size_t>(attackType_)]) {
			prevAttackType_ = attackType_;
			attackType_ = AttackType::None;
			ChangeState(State::Idle);
		}
	}

	void Boss::UpdateDestroy() {
		float t = std::clamp(stateTimer_ / destroyTime_, 0.0f, 1.0f);

		// 揺れながら縮んで消える
		float shake = destroyShake_ * (1.0f - t);
		transform_.translate = {
			destroyBasePosition_.x + RandomFloat(-shake, shake),
			destroyBasePosition_.y + RandomFloat(-shake, shake),
			destroyBasePosition_.z,
		};
		float s = 1.0f - EaseInOutCubic(t,1.0f,0.0f);
		transform_.scale = { baseScale_.x * s, baseScale_.y * s, baseScale_.z * s };

		if (t >= 1.0f) {
			ChangeState(State::Dead);
		}
	}

	//==================================================
	// 攻撃
	//==================================================

	void Boss::StartAttack() {
		attackType_ = DecideNextAttack();
		shotTimer_ = 0.0f;

		if (attackType_ == AttackType::Rush) {
			rushStart_ = transform_.translate;
			// ターゲットがいればその位置へ、いなければ定位置へ
			rushEnd_ = target_ ? *target_ : homePosition_;
			rushEnd_.z = rushStart_.z;	// 2軸アクションなので奥行きは固定
		}
	}

	void Boss::Shot() {
		shotTimer_ -= kDeltaTime;
		if (shotTimer_ <= 0.0f) {
			FireBullet(false);
			shotTimer_ = shotInterval_;
		}
	}

	void Boss::HomingShot() {
		shotTimer_ -= kDeltaTime;
		if (shotTimer_ <= 0.0f) {
			FireBullet(true);
			shotTimer_ = shotInterval_ * 2.0f;
		}
	}

	void Boss::Rush() {
		float t = std::clamp(stateTimer_ / attackTime_[static_cast<size_t>(AttackType::Rush)], 0.0f, 1.0f);

		// 前半で突っ込み、後半で元の位置に戻る
		if (t < 0.5f) {
			transform_.translate = Lerp(rushStart_, rushEnd_, EaseInOutCubic(t * 2.0f,0.0f,1.0f));
		} else {
			transform_.translate = Lerp(rushEnd_, rushStart_, EaseInOutCubic((t - 0.5f) * 2.0f,0.0f,1.0f));
		}
	}

	void Boss::FireBullet(bool isHoming) {
		// TODO: 弾の生成（BossBullet を作ってリストに追加する等）
		(void)isHoming;
	}

	//==================================================
	// 抽選
	//==================================================

	Boss::AttackType Boss::DecideNextAttack() {
		// None(0) を除いた攻撃の種類数
		constexpr int kNumAttack = static_cast<int>(AttackType::Count) - 1;

		// 初回は全種類から
		if (prevAttackType_ == AttackType::None) {
			return static_cast<AttackType>(RandomInt32_t(1, kNumAttack));
		}

		// 前回を除いた (kNumAttack - 1) 種類から抽選し、
		// 前回以上の番号なら1つずらすことで前回と被らないようにする
		int prev = static_cast<int>(prevAttackType_);
		int index = RandomInt32_t(1, kNumAttack - 1);
		if (index >= prev) { ++index; }
		return static_cast<AttackType>(index);
	}

	Vector3 Boss::DecideMovePosition() {
		return {
			RandomFloat(moveRangeMin_.x, moveRangeMax_.x),
			RandomFloat(moveRangeMin_.y, moveRangeMax_.y),
			RandomFloat(moveRangeMin_.z, moveRangeMax_.z),
		};
	}
}