#pragma once
#include <RyoEngine.h>
#include <memory>
#include <algorithm>
#include "../MapChipField/MapChipField.h"

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

		// 判定を取るマップを渡す（Initializeより前でも後でもOK）
		void SetMap(const MapChipField* map) { map_ = map; }

		// 当たり判定の大きさ（幅,高さ,奥行き）。モデルの大きさとは無関係に変更できる
		void SetCollideSize(const RyoEngine::Vector3& size) {
			halfSize_ = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
		}

		// position は「足元中心」
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
				isDestroy_ = true;
			}
		}

		bool IsDead() const { return isDead_; }

		const RyoEngine::OBB& GetOBB() const { return obb_; }
		float GetDamage() const { return damage_; }
		bool IsCollidable() const { return !isDestroy_ && !isDead_; }

	protected:
		/// <summary>
		/// 重力＋マップ判定つきで移動する。全ての敵で共通利用する
		/// </summary>
		/// <param name="vx">横方向の速度（ワールド座標/秒）。縦方向は重力で自動計算</param>
		/// <returns>どの面でブロックに当たったか</returns>
		MapHit MoveWithMap(float vx) {
			const float dt = RyoEngine::GetScaleTime();
			MapHit hit;

			velocityY_ -= gravity_ * dt;
			velocityY_ = std::max(velocityY_, -maxFallSpeed_);
			RyoEngine::Vector3 delta = { vx * dt, velocityY_ * dt, 0.0f };

			if (map_) {
				transform_.translate = map_->MoveAndCollide(transform_.translate, delta, halfSize_, hit);
				if (hit.bottom || (hit.top && velocityY_ > 0.0f)) velocityY_ = 0.0f;
				onGround_ = hit.bottom;
			} else {
				transform_.translate.x += delta.x;
				transform_.translate.y += delta.y;
			}
			return hit;
		}

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

		// transform_.translate は足元中心
		RyoEngine::Transform transform_{
			.scale{1.0f,1.0f,1.0f}
		};

		// プレイヤーとの当たり判定用
		RyoEngine::OBB obb_{
			.size = {1.0f,1.0f,1.0f}
		};
		// --- マップ判定用 ---
		const MapChipField* map_ = nullptr;
		RyoEngine::Vector3 halfSize_ = { 0.8f,0.8f,0.8f };  // 体の当たり判定の半分のサイズ（SetCollideSizeで変更）
		float velocityY_ = 0.0f;
		float gravity_ = 80.0f;
		float maxFallSpeed_ = 60.0f;
		bool onGround_ = false;

		float hp_ = kMaxHp_;
		float damage_ = kMaxDamage_;

		// 死亡アニメーション
		float animTimer_ = 0.0f;
		bool isDestroy_ = false;
		bool isDead_ = false;
	};
}