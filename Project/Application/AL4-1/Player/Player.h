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
		float GetMaxHP() const { return maxHp_; }
		bool IsDead() const { return isDead_; }

		// --- 攻撃（パリィを兼ねる） ---
		// 攻撃判定が出ているか（この間はパリィも受け付ける）
		bool IsAttackActive() const { return isAttacking_; }
		// 攻撃判定（パリィ判定も兼ねる）
		const RyoEngine::OBB& GetAttackOBB() const { return attackObb_; }
		float GetAttackDamage() const { return attackDamage_; }

		bool IsAttackHitDone() const { return isAttackHitDone_; }
		void SetAttackHitDone() { isAttackHitDone_ = true; }

		/// <summary>
		/// パリィ成功
		/// </summary>
		/// <param name="isCounter">近接攻撃をパリィした時trueを入れる</param>
		/// <returns>ゲージ満タンでのパリィ（フルパリィ）だったらtrue。ゲージは0に戻る</returns>
		bool OnParrySuccess(bool isCounter);

		// --- ゲージ（パリィで溜まり、満タンでパリィするとフルパリィ） ---
		float GetGaugeRatio() const { return maxGauge_ > 0.0f ? gauge_ / maxGauge_ : 0.0f; }
		bool IsGaugeMax() const { return gauge_ >= maxGauge_; }
		// フルパリィでボスに与える追加ダメージ
		float GetFullParryDamage() const { return fullParryDamage_; }

		void ResetStatus();

		// 操作を受け付けるか
		void SetControllable(bool controllable) { isControllable_ = controllable; }
		// カメラの方を向かせるか（クリア演出用）
		void SetFaceCamera(bool faceCamera) { isFaceCamera_ = faceCamera; }

		// クリア時に光ってると見にくいのでgameから消せるように
		void SetIntensity(float intensity) { model_->SetEmissive({ 0.0f,0.0f,0.0f }, intensity); }
	private:
		void RegisterInParamEditor();
		void Jump();
		void Attack();

		void HpEffectUpdate();
		void DamageTakenUpdate();
		void InvincibleUpdate();
		void AttackUpdate();
		void ParryEffectUpdate();
		// ゲージに合わせた発光とパーティクルのフラグ
		void GaugeUpdate();

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
		// 体の当たり判定の半分のサイズ
		RyoEngine::Vector3 halfSize_ = { 0.8f,1.0f,0.8f };

		// 速度（単位：ワールド座標/秒。1マス=2.0）
		RyoEngine::Vector3 velocity_ = { 0.0f,0.0f,0.0f };
		float moveSpeed_ = 12.0f;
		// 向いている方向（+1:右 -1:左）。最後に入力した方向
		float facing_ = 1.0f;
		// --- モデルの向き（facing_ に向かって補間しながら回る） ---
		float facingRightYaw_ = 90.0f;
		float facingLeftYaw_ = -90.0f;
		float turnDelay_ = 0.1f;
		float currentYaw_ = 90.0f;
		// カメラの方を向くときの向き（クリア演出）
		float faceCameraYaw_ = 180.0f;
		bool isFaceCamera_ = false;

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

		// --- 攻撃とパリイ ---
		RyoEngine::OBB attackObb_{
			.orientations = { {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {0.0f,0.0f,1.0f} },
			.size = {1.0f,1.0f,1.0f}
		};
		// 攻撃中か
		bool isAttacking_ = false;
		// 攻撃が一度当たったか
		bool isAttackHitDone_ = false;		// 今回の攻撃がもうボスに当たったか
		// 時間系
		float attackTimer_ = 0.0f;
		float attackCooldownTimer_ = 0.0f;
		float attackActiveTime_ = 0.35f;
		float attackCooldown_ = 0.35f;
		// ボスに与えるダメージ
		float attackDamage_ = 3.0f;

		float attackReach_ = 0.3f;
		float attackOffsetY_ = 0.0f;
		RyoEngine::Vector3 attackHalfSize_ = { 1.6f,1.6f,1.0f };	// 回転するので体のまわりを囲む大きさ

		// --- サマーソルト（攻撃中の1回転。見た目だけで判定は回さない） ---
		float spinEasePower_ = 2.0f;
		float spinDirection_ = 1.0f;
		// 回転の軌跡パーティクルの発生位置（体の中心から回転に合わせて円を描く）
		RyoEngine::Vector3 spinTrailPos_ = { 0.0f,0.0f,0.0f };
		// 体の中心から軌跡までの距離（足先あたりが通るくらい）
		float spinTrailRadius_ = 1.0f;
		// 攻撃中とクールタイム中で色を分ける
		RyoEngine::Vector4 attackColor_ = { 0.4f,0.9f,1.0f,1.0f };
		RyoEngine::Vector4 cooldownColor_ = { 0.6f,0.6f,0.6f,1.0f };

		// --- パリィ成功時の演出（色収差＋スロー） ---
		// エフェクトを出すか
		bool isParryEffect_ = false;
		// パリィが成功したフレームだけtrue（パーティクルの発生フラグ用。次のUpdateの冒頭で戻す）
		bool isParryTrigger_ = false;
		// 経過時間
		float parryEffectTimer_ = 0.0f;
		// 今回の演出の値（弾パリィかカウンターかで切り替わる）
		float effectDuration_ = 0.5f;
		float effectPeakTime_ = 0.25f;
		float effectMaxChromatic_ = 0.0f;
		float effectSlowScale_ = 1.0f;
		float effectHoldTime_ = 0.0f;	// 一番遅い状態を保つ時間（フルパリィのときだけ使う）
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

		// --- 発光 ---
		float emissiveIntensity_ = 0.0f;
		RyoEngine::Vector3 emissiveColor_{};

		// ライトの点灯用フラグ
		bool isIllumination_ = true;

		// --- ゲージ（パリィで溜まり、満タンでパリィするとフルパリィ） ---
		float maxGauge_ = 100.0f;
		float gauge_ = 0.0f;
		float parryGaugeGain_ = 10.0f;		// 弾をパリィしたとき
		float counterGaugeGain_ = 30.0f;	// 近接攻撃をカウンターしたとき
		float damageGaugePenalty_ = 0.0f;	// 被弾したときに減る量
		float fullParryDamage_ = 30.0f;		// フルパリィでボスに与える追加ダメージ
		// 溜まるほど強く光る（ゲージ0で発光0、満タンで chargeEmissiveIntensity_）
		RyoEngine::Vector3 chargeEmissiveColor_ = { 0.4f,0.9f,1.0f };
		float chargeEmissiveIntensity_ = 1.5f;
		// 満タンのときは脈打つ（最大の強さを超えないよう、下へ揺らす）
		float maxPulseSpeed_ = 3.0f;		// 1秒あたりの回数
		float maxPulseAmount_ = 0.3f;		// 最大の強さから下げる割合（0.3なら 1.5 → 1.05 の間で揺れる）
		float gaugeTimer_ = 0.0f;
		// パーティクル用の段階フラグ（1/3以上・2/3以上・満タン）。エミッターを重ねて、溜まるほどパリパリが増える
		bool isCharge1_ = false;
		bool isCharge2_ = false;
		bool isChargeMax_ = false;
		// フルパリィしたフレームだけtrue（パーティクルの発生フラグ用。次のUpdateの冒頭で戻す）
		bool isFullParryTrigger_ = false;
		// フルパリィの演出（通常のパリィ・カウンターより強く・遅く）
		float fullParryEffectDuration_ = 1.2f;
		float fullParryEffectPeakTime_ = 0.3f;
		float fullParryEffectHoldTime_ = 0.4f;	// 一番遅い状態を保つ時間（duration に含まれる）
		float fullParryMaxChromatic_ = 0.6f;
		float fullParrySlowScale_ = 0.03f;
	};
}