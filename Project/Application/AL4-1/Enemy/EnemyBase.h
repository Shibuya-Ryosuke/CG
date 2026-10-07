#pragma once
#include <RyoEngine.h>
#include <memory>

namespace AL4_1 {
	class EnemyBase {
	public:
		EnemyBase() = default;
		virtual ~EnemyBase() = default;
		// コピー禁止
		EnemyBase(const EnemyBase&) = delete;
		EnemyBase& operator=(const EnemyBase&) = delete;
		// ムーブは許可
		EnemyBase(EnemyBase&&) noexcept = default;
		EnemyBase& operator=(EnemyBase&&) noexcept = default;

		virtual void Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position) {
			// 位置を入れる
			transform_.translate = position;

			// インスタンスモデルを保持するオーナーをもらい、オーナーに自身を追加しハンドルをもらう
			owner_ = owner;
			handle_ = owner_->AddInstance(transform_.translate);
		}

		virtual void Update() {
			// 死亡アニメーション
			DestroyAnimation();
		}

		virtual void OnCollision(float damage) {
			hp_ -= damage;

			if (hp_ < 0.0f) {
				isDead_ = true;
			}
		}

		bool IsDead() const { return isDead_; }

	protected:
		void DestroyAnimation() {
			if (!isDestroy_)return;

			animTimer_ -= RyoEngine::GetDeltaTime();
			float t = animTimer_ / kAnimDuration_;

			if (t <= 0.0f) {
				t = 0.0f;
				isDead_ = true; // 死亡
			}

			// スケール用イージング（はじめ遅く終わり早く ＝ easeIn など。例えば t * t）
			float easeScale = RyoEngine::EaseInQuart(t, 0.0f, 1.0f);
			// スケールを 0 から 1 へ
			transform_.scale = { easeScale, easeScale, easeScale };
			// 自分のモデルにセット
			owner_->SetInstanceTransform(handle_, transform_);
		}

		float kMaxHp_ = 10.0f;
		float kMaxDamage_ = 10.0f;
		float kAnimDuration_ = 1.2f;

	protected:
		RyoEngine::InstancedModel* owner_ = nullptr;
		RyoEngine::InstancedModel::Handle handle_ = RyoEngine::InstancedModel::kInvalidHandle;

		RyoEngine::Transform transform_{
			.scale{1.0f,1.0f,1.0f}
		};

		float hp_ = kMaxHp_;
		float damage_ = kMaxDamage_;

		// 死亡アニメーション
		float animTimer_ = 0.0f;
		bool isDestroy_ = false;
		bool isDead_ = false;
	};
}