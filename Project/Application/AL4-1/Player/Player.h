#pragma once
#include <RyoEngine.h>
#include <memory>
#include "../MapChipField/MapChipField.h"

namespace AL4_1 {
	class Player {
	public:
		Player();
		~Player();
		// コピー禁止（ユニークポインタを持つので）
		Player(const Player&) = delete;
		Player& operator=(const Player&) = delete;
		// ムーブは許可
		Player(Player&&) noexcept = default;
		Player& operator=(Player&&) noexcept = default;

		void Initialize();
		void Update();
		void Draw(RyoEngine::Camera& camera);

		// 判定を取るマップを渡す（Gameが所有）
		void SetMap(const MapChipField* map) { map_ = map; }
		// 足元の位置（カメラの追従などに使う）
		const RyoEngine::Vector3& GetPosition() const { return transform_.translate; }

		// 当たり判定の大きさ（幅,高さ,奥行き）を設定する。モデルの大きさとは無関係に変更できる
		void SetCollideSize(const RyoEngine::Vector3& size) {
			halfSize_ = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
		}
		// 足元の位置を設定（スポーン用）
		void SetPosition(const RyoEngine::Vector3& feet) { transform_.translate = feet; }


		void OnDamage(float damage);
		const RyoEngine::OBB GetOBB()const { return obb_; }
		float GetHP() const { return hp_; }
		bool IsDead() const { return isDead_; }

		// --- 攻撃（パリィを兼ねる） ---
		// 攻撃判定が出ているか（この間はパリィも受け付ける）
		bool IsAttackActive() const { return isAttacking_; }
		// 攻撃判定（パリィ判定も兼ねる）
		const RyoEngine::OBB& GetAttackOBB() const { return attackObb_; }
		float GetAttackDamage() const { return attackDamage_; }
		// 1回の攻撃で何度もダメージを与えないためのフラグ
		bool IsAttackHitDone() const { return isAttackHitDone_; }
		void SetAttackHitDone() { isAttackHitDone_ = true; }
		/// <summary>パリィ成功時に Game から呼ぶ。isCounter はボスの近接攻撃へのカウンター</summary>
		void OnParrySuccess(bool isCounter);

		void ResetStatus();

		// 操作を受け付けるか（演出中は false。重力・演出の更新は続く）
		void SetControllable(bool controllable) { isControllable_ = controllable; }
	private:
		void RegisterInParamEditor();
		void Jump();
		void Attack();

		void HpEffectUpdate();
		void DamageTakenUpdate();
		void InvincibleUpdate();
		void AttackUpdate();
		void ParryEffectUpdate();

	private:
		// 自身
		std::unique_ptr<RyoEngine::Model> model_ = nullptr;
		// 判定を取るマップ
		const MapChipField* map_ = nullptr;

		// 敵や弾との当たり判定
		RyoEngine::OBB obb_{
			.orientations = { {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {0.0f,0.0f,1.0f} },
			.size = {1.0f,1.0f,1.0f}
		};

		// transform（translateは「足元中心」として扱う）
		RyoEngine::Transform transform_{
			.scale = {1.0f,1.0f,1.0f}
		};
		// 体の当たり判定の半分のサイズ（足元中心から左右・上方向）。SetCollideSize で変更できる
		// player.obj は 2x2x2（足元が原点）なので、見た目より少し細い幅1.6・高さ2.0にしている
		RyoEngine::Vector3 halfSize_ = { 0.8f,1.0f,0.8f };

		// 速度（単位：ワールド座標/秒。1マス=2.0）
		RyoEngine::Vector3 velocity_ = { 0.0f,0.0f,0.0f };
		float moveSpeed_ = 12.0f;
		// 向いている方向（+1:右 -1:左）。最後に入力した方向
		float facing_ = 1.0f;
		// --- モデルの向き（facing_ に向かって補間しながら回る） ---
		float facingRightYaw_ = 90.0f;	// 右向きのときのY回転（度）
		float facingLeftYaw_ = -90.0f;	// 左向きのときのY回転（度）。右→左の間を補間するので、0°側（カメラ側）を通って振り向く
		float turnDelay_ = 0.1f;		// 振り向きの遅れ（秒）。0で即座に向く
		float currentYaw_ = 90.0f;		// 今のモデルのY回転（度）

		// hp
		float maxHp_ = 100.0f;
		float hp_ = maxHp_;

		// 死亡フラグ
		bool isDead_ = false;
		// 操作を受け付けるか
		bool isControllable_ = true;

		// --- ジャンプ関係 ---
		// 接地判定（マップとの判定結果で毎フレーム更新される）
		bool onGround_ = false;
		// 初速（ワールド座標/秒）
		float jumpPower_ = 28.0f;
		// 重力（ワールド座標/秒^2）
		float gravity_ = 80.0f;
		// 落下速度の上限
		float maxFallSpeed_ = 60.0f;


		// 被ダメージ時ブラー、スロー
		bool isDamageTaken_ = false;
		float maxDamageTakenDuration_ = 0.5f;
		float damageTakenDuration_ = maxDamageTakenDuration_;
		float maxDamageTakenBlurStrength_ = 0.012f;
		float damageTakenTimeScale_ = 0.08f;

		// 無敵
		bool isInvincible_ = false;
		float maxInvincibleDuration_ = 2.0f;
		float invincibleDuration_ = maxInvincibleDuration_;
		// アルファ往復回数、アルファ最低保証値
		int32_t blinkCount_ = 5;
		float minAlpha_ = 0.5f;
		// hp量に応じて画面白黒化、ほんの少しのノイズ
		float maxGrayScale_ = 0.7f;
		float maxNoiseIntensity_ = 0.3f;
		float hpEffectStartRaito_ = 0.5f;  // hp残量エフェクトの開始はhpが5割を切ってから

		// --- 攻撃（パリィを兼ねる） ---
		RyoEngine::OBB attackObb_{
			.orientations = { {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {0.0f,0.0f,1.0f} },
			.size = {1.0f,1.0f,1.0f}
		};
		bool isAttacking_ = false;			// 攻撃判定が出ている（＝回転中。その場で停止する）
		bool isAttackHitDone_ = false;		// 今回の攻撃がもうボスに当たったか
		float attackTimer_ = 0.0f;			// 攻撃判定の残り時間
		float attackCooldownTimer_ = 0.0f;	// クールタイムの残り時間
		float attackActiveTime_ = 0.35f;	// 攻撃判定（＝パリィ受付）が出ている時間（秒）。1回転にかかる時間も同じ
		float attackCooldown_ = 0.35f;		// 攻撃が終わってから次に攻撃できるまで（秒）。成功・失敗に関係なく毎回
		float attackDamage_ = 3.0f;			// 通常攻撃でボスに与えるダメージ
		float attackReach_ = 0.3f;			// 体の中心から攻撃判定の中心までの横の距離（0で体の中心）
		float attackOffsetY_ = 0.0f;		// 攻撃判定の高さのずれ
		RyoEngine::Vector3 attackHalfSize_ = { 1.6f,1.6f,1.0f };	// 回転するので体のまわりを囲む大きさ

		// --- サマーソルト（攻撃中の1回転。見た目だけで判定は回さない） ---
		float spinEasePower_ = 2.0f;		// 回り方（1:等速 大きいほど出だしが速く最後がゆっくり）
		float spinDirection_ = 1.0f;		// 回る向き（後ろ宙返りにならなければ -1 にする）
		RyoEngine::Vector4 attackColor_ = { 0.4f,0.9f,1.0f,1.0f };	// 攻撃中の色
		RyoEngine::Vector4 cooldownColor_ = { 0.6f,0.6f,0.6f,1.0f };	// クールタイム中の色

		// --- パリィ成功時の演出（色収差＋スロー） ---
		// 色収差：始めが一番強く、duration にかけて EaseInOutCubic で弱まる
		// スロー：0 → peakTime で一番遅くなり、peakTime → duration で元に戻る（どちらも実時間）
		bool isParryEffect_ = false;
		float parryEffectTimer_ = 0.0f;		// 演出開始からの経過時間
		// 今回の演出の値（弾パリィかカウンターかで切り替わる）
		float effectDuration_ = 0.5f;
		float effectPeakTime_ = 0.25f;
		float effectMaxChromatic_ = 0.0f;
		float effectSlowScale_ = 1.0f;
		// 弾をパリィしたとき
		float parryEffectDuration_ = 0.5f;
		float parryEffectPeakTime_ = 0.25f;
		float parryMaxChromatic_ = 0.2f;	// 色収差の最大の強さ
		float parrySlowScale_ = 0.3f;		// 一番遅くなったときのタイムスケール
		// 近接攻撃にカウンターしたとき
		float counterEffectDuration_ = 0.5f;
		float counterEffectPeakTime_ = 0.25f;
		float counterMaxChromatic_ = 0.4f;
		float counterSlowScale_ = 0.1f;
	};
}
