#pragma once
#include <RyoEngine.h>
#include <random>
#include <array>
#include "../../Bullet/BulletManager.h"

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
			Shot,		// 自機狙いの連射
			SpreadShot,	// 扇状にばらまく
			HomingShot,	// プレイヤーへ曲がってくる弾
			Melee,		// 近接攻撃（カウンターできる）

			Count
		};

		enum class State {
			Appear,		// 登場アニメーション
			Idle,		// 待機（次の行動までのインターバル）
			Move,		// 攻撃前のランダム移動
			Attack,		// 攻撃中
			Stun,		// カウンターされて怯み中
			Destroy,	// 撃破アニメーション
			Dead,		// 完全に倒れた（描画・判定なし）

			Count,
		};

		// 近接攻撃の段階
		enum class MeleePhase {
			None,
			Windup,		// 予備動作（光ったら攻撃が来る）
			Active,		// 攻撃判定が出ている（ここでパリィするとカウンター）
			Recovery,	// 後隙
		};

		void Initialize();
		void Update();
		void Draw(RyoEngine::Camera& camera);
		// 登場からやり直す（リトライ用）
		void Reset();

		// ダメージを受ける（登場中・撃破中は無敵）
		void OnDamage(float damage);
		// 近接攻撃をカウンターされた（攻撃判定中のみ有効）
		void OnCountered();

		// 狙う対象（Rush や HomingShot で使う）
		void SetTarget(const RyoEngine::Vector3* target) { target_ = target; }
		// 弾を撃つときに使う
		void SetBulletManager(BulletManager* bulletManager) { bulletManager_ = bulletManager; }
		// CSVのスポーン位置（足元）。登場後の定位置は homeFeet_ + homeOffset_
		void SetHomeFeet(const RyoEngine::Vector3& feet) { homeFeet_ = feet; }

		// 移動先を抽選する範囲
		void SetMoveRange(const RyoEngine::Vector3& min, const RyoEngine::Vector3& max) {
			moveRangeMin_ = min;
			moveRangeMax_ = max;
		}

		State GetState() const { return state_; }
		bool IsDead() const { return state_ == State::Dead; }
		// 当たり判定を取ってよい状態か
		bool IsCollidable() const { return state_ != State::Appear && state_ != State::Destroy && state_ != State::Dead; }
		// 体当たりでダメージを与える状態か（怯み中は与えない）
		bool CanContactDamage() const { return IsCollidable() && state_ != State::Stun; }
		const RyoEngine::OBB& GetOBB() const { return obb_; }
		// 体の中心（反射弾の追尾先）。アドレスは変わらない
		const RyoEngine::Vector3* GetCenter() const { return &obb_.center; }
		float GetHP() const { return hp_; }
		float GetMaxHP() const { return maxHp_; }
		float GetDamage() const { return damage_; }

		// --- 近接攻撃 ---
		bool IsMeleeActive() const {
			return state_ == State::Attack && attackType_ == AttackType::Melee && meleePhase_ == MeleePhase::Active;
		}
		const RyoEngine::OBB& GetMeleeOBB() const { return meleeObb_; }
		float GetMeleeDamage() const { return meleeDamage_; }
		// 1回の近接攻撃で何度もダメージを与えないためのフラグ
		bool IsMeleeHitDone() const { return meleeHitDone_; }
		void SetMeleeHitDone() { meleeHitDone_ = true; }

		void RegisterInParamEditor();
	private:
		// ---- 状態管理 ----
		void ChangeState(State next);
		void UpdateAppear();
		void UpdateIdle();
		void UpdateMove();
		void UpdateAttack();
		void UpdateStun();
		void UpdateDestroy();

		// ---- 攻撃 ----
		void StartAttack();
		void Shot();
		void SpreadShot();
		void HomingShot();
		void Melee();
		// 弾を1発撃つ。撃てなかったら nullptr
		Bullet* FireBullet(const RyoEngine::Vector3& velocity);
		// 攻撃ごとの継続時間（近接は 予備動作+攻撃+後隙）
		float GetAttackDuration(AttackType type) const;

		// ---- 狙い ----
		RyoEngine::Vector3 GetAimPosition() const;	// プレイヤーの体の中心
		RyoEngine::Vector3 GetAimDirection() const;	// 自分 → プレイヤー の向き（xy、長さ1）

		// ---- 抽選 ----
		AttackType DecideNextAttack();
		RyoEngine::Vector3 DecideMovePosition();
		RyoEngine::Vector3 DecideMeleePosition();

		// ---- 見た目 ----
		void UpdateMeleeOBB();
		void UpdateColor();

	private:
		std::unique_ptr<RyoEngine::Model> model_ = nullptr;

		RyoEngine::Transform transform_{};

		float maxHp_ = 100.0f;
		float hp_ = 100.0f;

		RyoEngine::OBB obb_{
			.orientations = { {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {0.0f,0.0f,1.0f} },
			.size = {1.0f,1.0f,1.0f}
		};

		State state_ = State::Appear;
		float stateTimer_ = 0.0f;	// 今の状態に入ってからの経過時間

		AttackType attackType_ = AttackType::None;
		AttackType prevAttackType_ = AttackType::None;

		// ---- 時間パラメータ ----
		float appearTime_ = 2.0f;	// 登場アニメーション時間
		float idleTime_ = 1.0f;		// 待機時間
		float moveTime_ = 0.8f;		// 移動にかける時間
		float destroyTime_ = 2.0f;	// 撃破アニメーション時間
		// 攻撃ごとの継続時間（AttackTypeの順）
		std::array<float, static_cast<size_t>(AttackType::Count)> attackTime_
		{
			{
				0.0f,	// None
			    2.0f,	// Shot
			    2.4f,	// SpreadShot
			    3.0f,	// HomingShot
			    0.0f,	// Melee（meleeWindupTime_ + meleeActiveTime_ + meleeRecoveryTime_ を使う）
			}
		};

		// ---- 移動 ----
		RyoEngine::Vector3 homeFeet_{ 0.0f, 0.0f, 0.0f };		// CSVのスポーン位置（足元）
		RyoEngine::Vector3 homeOffset_{ 0.0f, 8.0f, 0.0f };	// スポーン位置から定位置までのずれ（空中に浮く）
		RyoEngine::Vector3 homePosition_{ 0.0f, 4.0f, 0.0f };	// 登場後の定位置
		RyoEngine::Vector3 appearOffset_{ 0.0f, 15.0f, 0.0f };	// 登場開始位置のオフセット（上から降りてくる）
		RyoEngine::Vector3 moveRangeMin_{ 4.0f, 10.0f, 0.0f };
		RyoEngine::Vector3 moveRangeMax_{ 58.0f, 20.0f, 0.0f };
		RyoEngine::Vector3 moveStart_{};
		RyoEngine::Vector3 moveEnd_{};

		// ---- 狙い ----
		float targetAimOffsetY_ = 1.0f;	// 対象の足元から体の中心までの高さ

		// ---- 射撃 ----
		BulletManager* bulletManager_ = nullptr;
		float shotInterval_ = 0.4f;
		float shotTimer_ = 0.0f;
		float shotSpeed_ = 16.0f;
		// 扇状
		int32_t spreadCount_ = 5;
		float spreadAngle_ = 15.0f;		// 弾と弾の間の角度（度）
		float spreadInterval_ = 0.8f;
		float spreadSpeed_ = 12.0f;
		// 追尾
		float homingSpeed_ = 10.0f;
		float homingTurnRate_ = 90.0f;	// 度/秒
		float homingDuration_ = 1.2f;	// 曲がり続ける時間

		// ---- 近接攻撃 ----
		MeleePhase meleePhase_ = MeleePhase::None;
		float meleeWindupTime_ = 0.7f;		// 予備動作
		float meleeActiveTime_ = 0.2f;		// 攻撃判定が出ている時間
		float meleeRecoveryTime_ = 0.6f;	// 後隙
		float meleeGlintTime_ = 0.15f;		// 予備動作の最後に光る時間（パリィの合図）
		float meleeDistance_ = 3.5f;		// プレイヤーからどれだけ離れた位置に寄るか
		float meleeHeightOffset_ = 0.0f;	// 寄るときの高さのずれ
		float meleeBackStep_ = 0.8f;		// 予備動作で後ろに下がる距離
		float meleeLunge_ = 2.0f;			// 攻撃で前に踏み込む距離
		float meleeReach_ = 1.8f;			// 体の中心から攻撃判定の中心までの距離
		RyoEngine::Vector3 meleeHalfSize_{ 1.4f, 1.2f, 1.0f };
		float meleeDamage_ = 20.0f;
		float meleeDir_ = -1.0f;			// 攻撃する向き（+1:右 -1:左）
		bool meleeHitDone_ = false;
		RyoEngine::Vector3 meleeStart_{};
		RyoEngine::OBB meleeObb_{
			.orientations = { {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {0.0f,0.0f,1.0f} },
			.size = {1.0f,1.0f,1.0f}
		};

		// ---- カウンターされたとき ----
		float counterDamage_ = 15.0f;
		float stunTime_ = 1.5f;				// 怯み時間
		float stunKnockback_ = 3.0f;		// 吹き飛ぶ距離
		float stunKnockbackTime_ = 0.3f;
		RyoEngine::Vector3 stunStart_{};
		RyoEngine::Vector3 stunEnd_{};

		// ---- 色 ----
		float hitFlashTimer_ = 0.0f;
		float hitFlashTime_ = 0.1f;
		RyoEngine::Vector4 baseColor_{ 1.0f, 1.0f, 1.0f, 1.0f };
		RyoEngine::Vector4 windupColor_{ 1.0f, 0.5f, 0.2f, 1.0f };	// 予備動作で徐々にこの色へ
		RyoEngine::Vector4 glintColor_{ 1.0f, 1.0f, 0.3f, 1.0f };	// パリィの合図
		RyoEngine::Vector4 meleeActiveColor_{ 1.0f, 0.1f, 0.1f, 1.0f };
		RyoEngine::Vector4 stunColor_{ 0.5f, 0.5f, 1.0f, 1.0f };
		RyoEngine::Vector4 hitFlashColor_{ 1.0f, 0.3f, 0.3f, 1.0f };

		// ---- 撃破演出 ----
		RyoEngine::Vector3 destroyBasePosition_{};
		RyoEngine::Vector3 baseScale_{ 1.0f, 1.0f, 1.0f };
		float destroyShake_ = 0.3f;

		const RyoEngine::Vector3* target_ = nullptr;

		std::mt19937 randomEngine_{ std::random_device{}() };

		float damage_ = 10.0f;
	};
}
