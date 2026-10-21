#include "Boss.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace RyoEngine;

namespace AL4_1 {
	Boss::Boss() = default;
	Boss::~Boss() = default;

	void Boss::Initialize() {
		model_ = Model::Create("Resources/ApplicationResources/AL4-1/Enemy/Boss/boss.obj");

		attackType_ = AttackType::None;
		prevAttackType_ = AttackType::None;
		transform_.scale = baseScale_;

		RegisterInParamEditor();

		ChangeState(State::Appear);
	}

	void Boss::Reset() {
		attackType_ = AttackType::None;
		prevAttackType_ = AttackType::None;
		meleePhase_ = MeleePhase::None;
		hitFlashTimer_ = 0.0f;
		transform_.scale = baseScale_;

		ChangeState(State::Appear);
	}

	void Boss::Update() {
		const float dt = GetScaleTime();
		stateTimer_ += dt;

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
		case State::Stun:
			UpdateStun();
			break;
		case State::Destroy:
			UpdateDestroy();
			break;
		case State::Dead:
			break;
		default:
			break;
		}

		if (hitFlashTimer_ > 0.0f) {
			hitFlashTimer_ -= dt;
		}

		obb_.center = transform_.translate;
		UpdateMeleeOBB();
		UpdateColor();
		model_->SetTransform(transform_);
	}

	void Boss::Draw(RyoEngine::Camera& camera) {
		if (state_ == State::Dead) { return; }
		model_->TransferMatrix(camera);
		model_->Draw();

		// デバッグ：近接攻撃の判定（予備動作中はオレンジ、攻撃中は赤）
		if (state_ == State::Attack && attackType_ == AttackType::Melee) {
			if (meleePhase_ == MeleePhase::Windup) {
				PrimitiveRenderer::DrawOBB(meleeObb_, { 1.0f,0.5f,0.0f,1.0f }, PrimitiveDrawMode::Wireframe);
			} else if (meleePhase_ == MeleePhase::Active) {
				PrimitiveRenderer::DrawOBB(meleeObb_, { 1.0f,0.0f,0.0f,1.0f }, PrimitiveDrawMode::Wireframe);
			}
		}
	}

	void Boss::OnDamage(float damage) {
		if (!IsCollidable()) { return; }

		hp_ -= damage;
		hitFlashTimer_ = hitFlashTime_;
		if (hp_ <= 0.0f) {
			hp_ = 0.0f;
			ChangeState(State::Destroy);
		}
	}

	void Boss::OnCountered() {
		if (!IsMeleeActive()) { return; }

		// 近接攻撃を中断
		prevAttackType_ = AttackType::Melee;
		attackType_ = AttackType::None;
		meleePhase_ = MeleePhase::None;

		OnDamage(counterDamage_);
		// 倒れていなければ怯む
		if (state_ != State::Destroy) {
			ChangeState(State::Stun);
		}
	}

	//==================================================
	// 状態管理
	//==================================================

	void Boss::RegisterInParamEditor() {
		ParamEditor::BeginGroup("Boss");
		ParamEditor::RegisterValue("maxHp", &maxHp_, 1.0f, 1.0f, 2000.0f);
		ParamEditor::RegisterValue("damage", &damage_, 1.0f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("obbHalfSize", &obb_.size, 0.01f, 0.1f, 10.0f);
		ParamEditor::RegisterValue("baseScale", &baseScale_, 0.01f, 0.1f, 10.0f);
		ParamEditor::RegisterValue("homeOffset", &homeOffset_, 0.1f, -50.0f, 50.0f);
		ParamEditor::RegisterValue("appearOffset", &appearOffset_, 0.1f, -50.0f, 50.0f);
		ParamEditor::RegisterValue("moveRangeMin", &moveRangeMin_, 0.1f, -100.0f, 200.0f);
		ParamEditor::RegisterValue("moveRangeMax", &moveRangeMax_, 0.1f, -100.0f, 200.0f);
		ParamEditor::RegisterValue("targetAimOffsetY", &targetAimOffsetY_, 0.01f, -5.0f, 5.0f);
		ParamEditor::RegisterValue("appearTime", &appearTime_, 0.01f, 0.1f, 10.0f);
		ParamEditor::RegisterValue("idleTime", &idleTime_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("moveTime", &moveTime_, 0.01f, 0.1f, 10.0f);
		ParamEditor::RegisterValue("destroyTime", &destroyTime_, 0.01f, 0.1f, 10.0f);
		ParamEditor::RegisterValue("hitFlashTime", &hitFlashTime_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("destroyShake", &destroyShake_, 0.01f, 0.0f, 3.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("BossShot");
		ParamEditor::RegisterValue("attackTime_Shot", &attackTime_[static_cast<size_t>(AttackType::Shot)], 0.01f, 0.1f, 10.0f);
		ParamEditor::RegisterValue("attackTime_Spread", &attackTime_[static_cast<size_t>(AttackType::SpreadShot)], 0.01f, 0.1f, 10.0f);
		ParamEditor::RegisterValue("attackTime_Homing", &attackTime_[static_cast<size_t>(AttackType::HomingShot)], 0.01f, 0.1f, 10.0f);
		ParamEditor::RegisterValue("shotInterval", &shotInterval_, 0.01f, 0.05f, 5.0f);
		ParamEditor::RegisterValue("shotSpeed", &shotSpeed_, 0.1f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("spreadCount", &spreadCount_, 1.0f, 1, 20);
		ParamEditor::RegisterValue("spreadAngle", &spreadAngle_, 0.5f, 0.0f, 90.0f);
		ParamEditor::RegisterValue("spreadInterval", &spreadInterval_, 0.01f, 0.05f, 5.0f);
		ParamEditor::RegisterValue("spreadSpeed", &spreadSpeed_, 0.1f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("homingSpeed", &homingSpeed_, 0.1f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("homingTurnRate", &homingTurnRate_, 1.0f, 0.0f, 1000.0f);
		ParamEditor::RegisterValue("homingDuration", &homingDuration_, 0.01f, 0.0f, 10.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("BossMelee");
		ParamEditor::RegisterValue("windupTime", &meleeWindupTime_, 0.01f, 0.05f, 5.0f);
		ParamEditor::RegisterValue("activeTime", &meleeActiveTime_, 0.01f, 0.01f, 2.0f);
		ParamEditor::RegisterValue("recoveryTime", &meleeRecoveryTime_, 0.01f, 0.0f, 5.0f);
		ParamEditor::RegisterValue("glintTime", &meleeGlintTime_, 0.01f, 0.0f, 2.0f);
		ParamEditor::RegisterValue("distance", &meleeDistance_, 0.1f, 0.0f, 20.0f);
		ParamEditor::RegisterValue("heightOffset", &meleeHeightOffset_, 0.1f, -10.0f, 10.0f);
		ParamEditor::RegisterValue("backStep", &meleeBackStep_, 0.1f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("lunge", &meleeLunge_, 0.1f, 0.0f, 20.0f);
		ParamEditor::RegisterValue("reach", &meleeReach_, 0.1f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("halfSize", &meleeHalfSize_, 0.01f, 0.1f, 10.0f);
		ParamEditor::RegisterValue("damage", &meleeDamage_, 1.0f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("counterDamage", &counterDamage_, 1.0f, 0.0f, 500.0f);
		ParamEditor::RegisterValue("stunTime", &stunTime_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("stunKnockback", &stunKnockback_, 0.1f, 0.0f, 20.0f);
		ParamEditor::RegisterValue("stunKnockbackTime", &stunKnockbackTime_, 0.01f, 0.01f, 3.0f);
		ParamEditor::EndGroup();
	}

	void Boss::ChangeState(State next) {
		state_ = next;
		stateTimer_ = 0.0f;

		// 状態に入った瞬間の処理
		switch (state_) {
		case State::Appear:
			hp_ = maxHp_;
			homePosition_ = homeFeet_ + homeOffset_;
			transform_.translate = homePosition_ + appearOffset_;
			transform_.scale = baseScale_;
			break;

		case State::Move:
			// 次の攻撃を先に決め、近接なら プレイヤーの近く、それ以外ならランダムな位置へ
			attackType_ = DecideNextAttack();
			moveStart_ = transform_.translate;
			moveEnd_ = attackType_ == AttackType::Melee ? DecideMeleePosition() : DecideMovePosition();
			break;

		case State::Attack:
			StartAttack();
			break;

		case State::Stun:
		{
			// 攻撃していた向きと逆へ吹き飛ぶ
			stunStart_ = transform_.translate;
			stunEnd_ = stunStart_;
			stunEnd_.x = std::clamp(stunStart_.x - meleeDir_ * stunKnockback_, moveRangeMin_.x, moveRangeMax_.x);
			break;
		}

		case State::Destroy:
			destroyBasePosition_ = transform_.translate;
			attackType_ = AttackType::None;
			meleePhase_ = MeleePhase::None;
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
		case AttackType::SpreadShot:
			SpreadShot();
			break;
		case AttackType::HomingShot:
			HomingShot();
			break;
		case AttackType::Melee:
			Melee();
			break;
		default:
			break;
		}

		if (stateTimer_ >= GetAttackDuration(attackType_)) {
			prevAttackType_ = attackType_;
			attackType_ = AttackType::None;
			meleePhase_ = MeleePhase::None;
			ChangeState(State::Idle);
		}
	}

	void Boss::UpdateStun() {
		// 吹き飛んでから、その場で怯む
		float t = std::clamp(stateTimer_ / stunKnockbackTime_, 0.0f, 1.0f);
		transform_.translate = Lerp(stunStart_, stunEnd_, EaseOutCubic(t, 0.0f, 1.0f));

		if (stateTimer_ >= stunTime_) {
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
		float s = EaseInOutCubic(t,1.0f,0.0f);
		transform_.scale = { baseScale_.x * s, baseScale_.y * s, baseScale_.z * s };

		if (t >= 1.0f) {
			ChangeState(State::Dead);
		}
	}

	//==================================================
	// 攻撃
	//==================================================

	void Boss::StartAttack() {
		// 攻撃の種類は Move に入るときに決めている
		shotTimer_ = 0.0f;

		if (attackType_ == AttackType::Melee) {
			// プレイヤーのいる方を向いて構える
			Vector3 aim = GetAimPosition();
			meleeDir_ = aim.x >= transform_.translate.x ? 1.0f : -1.0f;
			meleeStart_ = transform_.translate;
			meleePhase_ = MeleePhase::Windup;
			meleeHitDone_ = false;
		}
	}

	void Boss::Shot() {
		shotTimer_ -= GetScaleTime();
		if (shotTimer_ <= 0.0f) {
			Vector3 dir = GetAimDirection();
			FireBullet({ dir.x * shotSpeed_, dir.y * shotSpeed_, 0.0f });
			shotTimer_ = shotInterval_;
		}
	}

	void Boss::SpreadShot() {
		shotTimer_ -= GetScaleTime();
		if (shotTimer_ <= 0.0f) {
			// プレイヤーの方向を中心に扇状に撃つ
			Vector3 dir = GetAimDirection();
			const float baseAngle = std::atan2(dir.y, dir.x);
			const float step = spreadAngle_ * std::numbers::pi_v<float> / 180.0f;
			for (int32_t i = 0; i < spreadCount_; ++i) {
				float angle = baseAngle + (static_cast<float>(i) - (spreadCount_ - 1) * 0.5f) * step;
				FireBullet({ std::cos(angle) * spreadSpeed_, std::sin(angle) * spreadSpeed_, 0.0f });
			}
			shotTimer_ = spreadInterval_;
		}
	}

	void Boss::HomingShot() {
		shotTimer_ -= GetScaleTime();
		if (shotTimer_ <= 0.0f) {
			Vector3 dir = GetAimDirection();
			Bullet* bullet = FireBullet({ dir.x * homingSpeed_, dir.y * homingSpeed_, 0.0f });
			if (bullet && target_) {
				bullet->SetHoming(target_, { 0.0f, targetAimOffsetY_, 0.0f }, homingTurnRate_, homingDuration_);
			}
			shotTimer_ = shotInterval_ * 2.0f;
		}
	}

	void Boss::Melee() {
		const float activeStart = meleeWindupTime_;
		const float activeEnd = activeStart + meleeActiveTime_;
		const Vector3 back = { meleeStart_.x - meleeDir_ * meleeBackStep_, meleeStart_.y, meleeStart_.z };
		const Vector3 front = { meleeStart_.x + meleeDir_ * meleeLunge_, meleeStart_.y, meleeStart_.z };

		if (stateTimer_ < activeStart) {
			// 予備動作：少し後ろに下がって溜める
			meleePhase_ = MeleePhase::Windup;
			float t = std::clamp(stateTimer_ / meleeWindupTime_, 0.0f, 1.0f);
			transform_.translate = Lerp(meleeStart_, back, EaseOutCubic(t, 0.0f, 1.0f));
		} else if (stateTimer_ < activeEnd) {
			// 攻撃：前へ踏み込む
			meleePhase_ = MeleePhase::Active;
			float t = std::clamp((stateTimer_ - activeStart) / meleeActiveTime_, 0.0f, 1.0f);
			transform_.translate = Lerp(back, front, EaseOutCubic(t, 0.0f, 1.0f));
		} else {
			// 後隙
			meleePhase_ = MeleePhase::Recovery;
		}
	}

	Bullet* Boss::FireBullet(const RyoEngine::Vector3& velocity) {
		if (!bulletManager_) { return nullptr; }
		return bulletManager_->Fire(transform_.translate, velocity);
	}

	float Boss::GetAttackDuration(AttackType type) const {
		if (type == AttackType::Melee) {
			return meleeWindupTime_ + meleeActiveTime_ + meleeRecoveryTime_;
		}
		return attackTime_[static_cast<size_t>(type)];
	}

	//==================================================
	// 狙い
	//==================================================

	Vector3 Boss::GetAimPosition() const {
		if (!target_) { return homePosition_; }
		return { target_->x, target_->y + targetAimOffsetY_, target_->z };
	}

	Vector3 Boss::GetAimDirection() const {
		Vector3 aim = GetAimPosition();
		float dx = aim.x - transform_.translate.x;
		float dy = aim.y - transform_.translate.y;
		float len = std::sqrt(dx * dx + dy * dy);
		if (len < 1e-4f) { return { -1.0f, 0.0f, 0.0f }; }
		return { dx / len, dy / len, 0.0f };
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

	Vector3 Boss::DecideMeleePosition() {
		Vector3 aim = GetAimPosition();

		// 今いる側からプレイヤーに寄る。移動範囲の外になるなら反対側から寄る
		float side = transform_.translate.x >= aim.x ? 1.0f : -1.0f;
		float x = aim.x + side * meleeDistance_;
		if (x < moveRangeMin_.x || x > moveRangeMax_.x) {
			x = aim.x - side * meleeDistance_;
		}
		x = std::clamp(x, moveRangeMin_.x, moveRangeMax_.x);

		return { x, aim.y + meleeHeightOffset_, transform_.translate.z };
	}

	//==================================================
	// 見た目
	//==================================================

	void Boss::UpdateMeleeOBB() {
		meleeObb_.center = { obb_.center.x + meleeDir_ * meleeReach_, obb_.center.y, obb_.center.z };
		meleeObb_.size = meleeHalfSize_;
	}

	void Boss::UpdateColor() {
		auto lerpColor = [](const Vector4& a, const Vector4& b, float t) {
			return Vector4{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t };
			};

		Vector4 color = baseColor_;
		if (state_ == State::Attack && attackType_ == AttackType::Melee) {
			if (meleePhase_ == MeleePhase::Windup) {
				// 徐々に色が変わり、攻撃の直前に光る（ここがパリィの合図）
				if (stateTimer_ >= meleeWindupTime_ - meleeGlintTime_) {
					color = glintColor_;
				} else {
					float t = std::clamp(stateTimer_ / (meleeWindupTime_ - meleeGlintTime_), 0.0f, 1.0f);
					color = lerpColor(baseColor_, windupColor_, t);
				}
			} else if (meleePhase_ == MeleePhase::Active) {
				color = meleeActiveColor_;
			}
		} else if (state_ == State::Stun) {
			color = stunColor_;
		}

		// 被弾したら一瞬赤く
		if (hitFlashTimer_ > 0.0f) {
			color = hitFlashColor_;
		}
		model_->SetColor(color);
	}
}
