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

		// パリィ受付中か（この間に弾・近接攻撃が parryObb_ に触れたらパリィ成功）
		bool IsParryActive() const { return isParryActive_; }
		const RyoEngine::OBB& GetParryOBB() const { return parryObb_; }
		/// <summary>パリィ成功時に Game から呼ぶ。isCounter はボスの近接攻撃へのカウンター</summary>
		void OnParrySuccess(bool isCounter);

		void ResetStatus();
	private:
		void RegisterInParamEditor();
		void Jump();
		void Attack() {};

		void HpEffectUpdate();
		void DamageTakenUpdate();
		void InvincibleUpdate();
		void ParryUpdate();
		void HitStopUpdate();

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

		// hp
		float maxHp_ = 100.0f;
		float hp_ = maxHp_;

		// 死亡フラグ
		bool isDead_ = false;

		// --- ジャンプ関係 ---
		// 接地判定（マップとの判定結果で毎フレーム更新される）
		bool onGround_ = false;
		// 初速（ワールド座標/秒）
		float jumpPower_ = 28.0f;
		// 重力（ワールド座標/秒^2）
		float gravity_ = 80.0f;
		// 落下速度の上限
		float maxFallSpeed_ = 60.0f;


		// 被ダメージ時色収差、スロー
		bool isDamageTaken_ = false;
		float maxDamageTakenDuration_ = 0.5f;
		float damageTakenDuration_ = maxDamageTakenDuration_;
		float maxDamageTakenChromaticAberrationStrength_ = 0.5f;
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

		// --- パリィ ---
		// 判定（体の判定を parryRange_ だけ広げたもの）
		RyoEngine::OBB parryObb_{
			.orientations = { {1.0f,0.0f,0.0f}, {0.0f,1.0f,0.0f}, {0.0f,0.0f,1.0f} },
			.size = {1.0f,1.0f,1.0f}
		};
		bool isParryActive_ = false;		// 受付中
		bool isParrySucceeded_ = false;		// 今回の受付中に1回でも成功したか（成功したら硬直なし）
		float parryTimer_ = 0.0f;			// 受付の残り時間
		float parryCooldownTimer_ = 0.0f;	// 硬直の残り時間（連打対策）
		float parryWindow_ = 0.15f;			// 受付時間（秒）
		float parryCooldown_ = 0.35f;		// 失敗したときの硬直（秒）
		RyoEngine::Vector3 parryRange_ = { 0.6f,0.4f,0.6f };	// 体の判定より広げる量
		RyoEngine::Vector4 parryColor_ = { 0.4f,0.9f,1.0f,1.0f };	// 受付中の色
		RyoEngine::Vector4 cooldownColor_ = { 0.6f,0.6f,0.6f,1.0f };	// 硬直中の色

		// --- パリィ成功時のヒットストップ ---
		bool isHitStop_ = false;
		float hitStopTimer_ = 0.0f;
		float hitStopDuration_ = 0.0f;		// 今回のヒットストップの長さ
		float parryHitStopDuration_ = 0.06f;	// 弾をパリィしたとき（実時間）
		float counterHitStopDuration_ = 0.18f;	// カウンターしたとき（実時間）
		float hitStopTimeScale_ = 0.05f;
		float hitStopChromaticStrength_ = 0.0f;	// 今回の色収差の強さ
		float parryChromaticStrength_ = 0.2f;
		float counterChromaticStrength_ = 0.6f;
	};
}
