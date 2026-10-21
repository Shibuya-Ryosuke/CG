#pragma once
#include <RyoEngine.h>

namespace AL4_1 {
	/// <summary>
	/// ボスの弾。BulletManager がプールとして持ち、使い回す
	/// ・パリィされると「反射弾」になり、ボスに向かって追尾で飛んでいく
	/// </summary>
	class Bullet {
	public:
		Bullet();
		~Bullet();

		/// <summary>プールを作るときに1回だけ呼ぶ。インスタンスを確保して非表示にしておく</summary>
		void Initialize(RyoEngine::InstancedModel* owner);
		/// <summary>発射する（ボスの弾として飛ぶ）</summary>
		void Fire(const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity);
		/// <summary>発射後に呼ぶと、duration 秒の間 target + offset に向かって曲がる</summary>
		void SetHoming(const RyoEngine::Vector3* target, const RyoEngine::Vector3& offset, float turnRateDeg, float duration);
		void Update();

		/// <summary>パリィされた。target に向かって追尾で飛んでいく</summary>
		void Reflect(const RyoEngine::Vector3* target);

		const RyoEngine::OBB& GetOBB() const { return obb_; }
		const RyoEngine::Vector3& GetPosition() const { return transform_.translate; }
		float GetDamage() const { return damage_; }
		bool IsDead() const { return isDead_; }
		// パリィされてボスに向かっている弾か
		bool IsReflected() const { return isReflected_; }
		void OnCollision();

		static void RegisterInParamEditor();

	private:
		// スケール0にして見えなくする（インスタンスは消さずに使い回す）
		void Hide();

	private:
		struct Param {
			float damage = 10.0f;
			float lifeTime = 3.0f;
			RyoEngine::Vector3 halfSize = { 0.3f, 0.3f, 0.3f };

			// --- パリィされた後 ---
			float reflectDamage = 5.0f;			// ボスに与えるダメージ
			float reflectLifeTime = 5.0f;
			float reflectStartSpeed = 8.0f;		// はね返した直後の速さ
			float reflectMaxSpeed = 40.0f;
			float reflectAccel = 60.0f;			// 速さの増え方（/秒）
			float reflectTurnRate = 180.0f;		// 曲がる速さ（度/秒）
			float reflectTurnAccel = 720.0f;	// 曲がる速さの増え方（度/秒^2）。時間が経つほど曲がりやすくなり、必ず当たる
			float reflectScale = 1.5f;			// 反射弾の見た目と判定の倍率
		};
		inline static Param param_{};

		RyoEngine::Transform transform_{
			.scale={1.0f,1.0f,1.0f}
		};
		RyoEngine::Vector3 velocity_{};
		RyoEngine::OBB obb_{
			.orientations = { {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {0.0f,0.0f,1.0f} },
			.size={1.0f,1.0f,1.0f}
		};

		RyoEngine::InstancedModel* owner_ = nullptr;
		RyoEngine::InstancedModel::Handle handle_ = RyoEngine::InstancedModel::kInvalidHandle;

		float damage_ = 10.0f;
		// プールの中で未使用のときも true
		bool isDead_ = true;

		float lifeTime_ = 3.0f;

		// --- 追尾 ---
		const RyoEngine::Vector3* homingTarget_ = nullptr;
		RyoEngine::Vector3 homingOffset_{};
		float homingTurnRate_ = 0.0f;	// 度/秒
		float homingTimer_ = 0.0f;		// 残りの追尾時間（ボスの弾）

		// --- 反射 ---
		bool isReflected_ = false;
		float reflectSpeed_ = 0.0f;
		float reflectTurnRate_ = 0.0f;	// 度/秒
	};
}
