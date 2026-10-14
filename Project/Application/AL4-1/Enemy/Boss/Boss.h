#pragma once
#include <RyoEngine.h>
#include <random>
#include <array>

namespace AL4_1 {
	class Boss {
	public:
		Boss();
		~Boss();

		// コピー禁止（ユニークポインタを持つので）
		Boss(const Boss&) = delete;
		Boss& operator=(const Boss&) = delete;
		// ムーブは許可
		Boss(Boss&&) noexcept = default;
		Boss& operator=(Boss&&) noexcept = default;

		enum class AttackType {
			None,
			Shot,
			HomingShot,
			Rush,

			Count
		};

		enum class State {
			Appear,		// 登場アニメーション
			Idle,		// 待機（次の行動までのインターバル）
			Move,		// 攻撃前のランダム移動
			Attack,		// 攻撃中
			Destroy,	// 撃破アニメーション
			Dead,		// 完全に倒れた（描画・判定なし）

			Count,
		};

		void Initialize();
		void Update();
		void Draw(RyoEngine::Camera& camera);

		// ダメージを受ける（登場中・撃破中は無敵）
		void OnDamage(float damage);

		// 狙う対象（Rush や HomingShot で使う）
		void SetTarget(const RyoEngine::Vector3* target) { target_ = target; }

		// 移動先を抽選する範囲
		void SetMoveRange(const RyoEngine::Vector3& min, const RyoEngine::Vector3& max) {
			moveRangeMin_ = min;
			moveRangeMax_ = max;
		}

		State GetState() const { return state_; }
		bool IsDead() const { return state_ == State::Dead; }
		// 当たり判定を取ってよい状態か
		bool IsCollidable() const { return state_ != State::Appear && state_ != State::Destroy && state_ != State::Dead; }
		const RyoEngine::OBB& GetOBB() const { return obb_; }
		float GetHP() const { return hp_; }
		float GetDamage() const { return damage_; }

	private:
		// ---- 状態管理 ----
		void ChangeState(State next);
		void UpdateAppear();
		void UpdateIdle();
		void UpdateMove();
		void UpdateAttack();
		void UpdateDestroy();

		// ---- 攻撃 ----
		void StartAttack();
		void Shot();
		void HomingShot();
		void Rush();
		void FireBullet(bool isHoming);	// 弾生成（中身は後で）

		// ---- 抽選 ----
		AttackType DecideNextAttack();
		RyoEngine::Vector3 DecideMovePosition();

	private:
		std::unique_ptr<RyoEngine::Model> model_ = nullptr;

		RyoEngine::Transform transform_{};

		// 1フレームの経過時間（エンジン側にdeltaTimeがあれば差し替え）
		static constexpr float kDeltaTime = 1.0f / 60.0f;

		float maxHp_ = 100.0f;
		float hp_ = 100.0f;

		RyoEngine::OBB obb_{
			.size = {1.0f,1.0f,1.0f}
		};

		State state_ = State::Appear;
		float stateTimer_ = 0.0f;	// 今の状態に入ってからの経過時間

		AttackType attackType_ = AttackType::None;
		AttackType prevAttackType_ = AttackType::None;

		// ---- 時間パラメータ ----
		float appearTime_ = 2.0f;	// 登場アニメーション時間
		float idleTime_ = 1.5f;		// 待機時間
		float moveTime_ = 1.0f;		// 移動にかける時間
		float destroyTime_ = 2.0f;	// 撃破アニメーション時間
		// 攻撃ごとの継続時間（AttackTypeの順）
		std::array<float, static_cast<size_t>(AttackType::Count)> attackTime_
		{
			{
				0.0f,	// None
			    2.0f,	// Shot
			    3.0f,	// HomingShot
			    1.2f,	// Rush
			}
		};

		// ---- 移動 ----
		RyoEngine::Vector3 homePosition_{ 0.0f, 4.0f, 0.0f };	// 登場後の定位置
		RyoEngine::Vector3 appearOffset_{ 0.0f, 15.0f, 0.0f };	// 登場開始位置のオフセット（上から降りてくる）
		RyoEngine::Vector3 moveRangeMin_{ -8.0f, 2.0f, 0.0f };
		RyoEngine::Vector3 moveRangeMax_{ 8.0f, 8.0f, 0.0f };
		RyoEngine::Vector3 moveStart_{};
		RyoEngine::Vector3 moveEnd_{};

		// ---- 射撃 ----
		float shotInterval_ = 0.4f;
		float shotTimer_ = 0.0f;

		// ---- 突進 ----
		RyoEngine::Vector3 rushStart_{};
		RyoEngine::Vector3 rushEnd_{};

		// ---- 撃破演出 ----
		RyoEngine::Vector3 destroyBasePosition_{};
		RyoEngine::Vector3 baseScale_{ 1.0f, 1.0f, 1.0f };
		float destroyShake_ = 0.3f;

		const RyoEngine::Vector3* target_ = nullptr;

		std::mt19937 randomEngine_{ std::random_device{}() };

		float damage_ = 10.0f;
	};
}