#include "Bullet.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace RyoEngine;

namespace AL4_1 {
	namespace {
		// xy平面で velocity の向きを toTarget の方へ最大 maxAngle(rad) だけ回し、速さを speed にする
		Vector3 SteerXY(const Vector3& velocity, const Vector3& toTarget, float maxAngle, float speed) {
			const float pi = std::numbers::pi_v<float>;
			float current = std::atan2(velocity.y, velocity.x);
			float desired = std::atan2(toTarget.y, toTarget.x);
			float diff = desired - current;
			while (diff > pi) diff -= 2.0f * pi;
			while (diff < -pi) diff += 2.0f * pi;
			diff = std::clamp(diff, -maxAngle, maxAngle);

			float angle = current + diff;
			return { std::cos(angle) * speed, std::sin(angle) * speed, 0.0f };
		}

		float LengthXY(const Vector3& v) { return std::sqrt(v.x * v.x + v.y * v.y); }

		float ToRad(float deg) { return deg * std::numbers::pi_v<float> / 180.0f; }
	}

	Bullet::Bullet() = default;
	Bullet::~Bullet() = default;

	void Bullet::Initialize(RyoEngine::InstancedModel* owner) {
		owner_ = owner;
		handle_ = owner_->AddInstance(transform_.translate);
		isDead_ = true;
		Hide();
	}

	void Bullet::Fire(const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity) {
		transform_.translate = position;
		transform_.scale = { 1.0f,1.0f,1.0f };
		velocity_ = velocity;
		damage_ = param_.damage;
		lifeTime_ = param_.lifeTime;
		obb_.size = param_.halfSize;
		obb_.center = position;

		isDead_ = false;
		isReflected_ = false;
		homingTarget_ = nullptr;
		homingTimer_ = 0.0f;

		owner_->SetInstanceTransform(handle_, transform_);
		owner_->SetInstanceColor(handle_, { 0.6f,0.2f,0.8f,1.0f });
	}

	void Bullet::SetHoming(const RyoEngine::Vector3* target, const RyoEngine::Vector3& offset, float turnRateDeg, float duration) {
		homingTarget_ = target;
		homingOffset_ = offset;
		homingTurnRate_ = turnRateDeg;
		homingTimer_ = duration;
	}

	void Bullet::Update() {
		if (isDead_) return;
		const float dt = GetScaleTime();

		lifeTime_ -= dt;
		if (lifeTime_ <= 0.0f) {
			OnCollision();
			return;
		}

		if (isReflected_) {
			// 反射弾：だんだん速く、だんだん曲がりやすくしてボスへ
			reflectSpeed_ = std::min(reflectSpeed_ + param_.reflectAccel * dt, param_.reflectMaxSpeed);
			reflectTurnRate_ += param_.reflectTurnAccel * dt;
			if (homingTarget_) {
				Vector3 toTarget = {
					homingTarget_->x - transform_.translate.x,
					homingTarget_->y - transform_.translate.y,
					0.0f };
				velocity_ = SteerXY(velocity_, toTarget, ToRad(reflectTurnRate_) * dt, reflectSpeed_);
			}
		} else if (homingTimer_ > 0.0f && homingTarget_) {
			// ボスの追尾弾：一定時間だけプレイヤーへ曲がる
			homingTimer_ -= dt;
			Vector3 toTarget = {
				homingTarget_->x + homingOffset_.x - transform_.translate.x,
				homingTarget_->y + homingOffset_.y - transform_.translate.y,
				0.0f };
			velocity_ = SteerXY(velocity_, toTarget, ToRad(homingTurnRate_) * dt, LengthXY(velocity_));
		}

		transform_.translate += velocity_ * dt;
		obb_.center = transform_.translate;

		owner_->SetInstanceTransform(handle_, transform_);
	}

	void Bullet::Reflect(const RyoEngine::Vector3* target) {
		if (isDead_ || isReflected_) return;

		isReflected_ = true;
		homingTarget_ = target;
		homingTimer_ = 0.0f;
		reflectSpeed_ = param_.reflectStartSpeed;
		reflectTurnRate_ = param_.reflectTurnRate;

		// 来た方向へはね返す（そこから曲がってボスへ向かう）
		float len = LengthXY(velocity_);
		Vector3 dir = len > 1e-4f ? Vector3{ -velocity_.x / len, -velocity_.y / len, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
		velocity_ = { dir.x * reflectSpeed_, dir.y * reflectSpeed_, 0.0f };

		damage_ = param_.reflectDamage;
		lifeTime_ = param_.reflectLifeTime;

		const float s = param_.reflectScale;
		transform_.scale = { s, s, s };
		obb_.size = { param_.halfSize.x * s, param_.halfSize.y * s, param_.halfSize.z * s };
	}

	void Bullet::OnCollision() {
		isDead_ = true;
		isReflected_ = false;
		Hide();
	}

	void Bullet::Hide() {
		transform_.scale = { 0.0f,0.0f,0.0f };
		if (owner_) owner_->SetInstanceTransform(handle_, transform_);
	}

	void Bullet::RegisterInParamEditor() {
		ParamEditor::BeginGroup("Bullet");
		ParamEditor::RegisterValue("damage", &param_.damage, 1.0f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("lifeTime", &param_.lifeTime, 0.1f, 0.1f, 20.0f);
		ParamEditor::RegisterValue("halfSize", &param_.halfSize, 0.01f, 0.05f, 3.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("BulletReflect");
		ParamEditor::RegisterValue("damage", &param_.reflectDamage, 1.0f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("lifeTime", &param_.reflectLifeTime, 0.1f, 0.1f, 20.0f);
		ParamEditor::RegisterValue("startSpeed", &param_.reflectStartSpeed, 0.1f, 0.0f, 100.0f);
		ParamEditor::RegisterValue("maxSpeed", &param_.reflectMaxSpeed, 0.1f, 0.0f, 150.0f);
		ParamEditor::RegisterValue("accel", &param_.reflectAccel, 0.1f, 0.0f, 500.0f);
		ParamEditor::RegisterValue("turnRate", &param_.reflectTurnRate, 1.0f, 0.0f, 2000.0f);
		ParamEditor::RegisterValue("turnAccel", &param_.reflectTurnAccel, 1.0f, 0.0f, 5000.0f);
		ParamEditor::RegisterValue("scale", &param_.reflectScale, 0.01f, 0.1f, 5.0f);
		ParamEditor::EndGroup();
	}
}
