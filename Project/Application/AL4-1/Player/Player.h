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

	private:
		void RegisterInParamEditor();
		void Jump();
		void Attack() {};
		
		void DamageTakenUpdate();
		void InvincibleUpdate();

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
		float hp_ = 100.0f;
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
		float damageTakenDuration_ = 0.5f;
		float maxDamageTakenDuration_ = 0.5f;
		float maxDamageTakenChromaticAberrationStrength_ = 0.2f;
		float damageTakenTimeScale_ = 0.1f;

		// 無敵
		bool isInvincible_ = false;
		float invincibleDuration_ = 2.0f;
		float maxInvincibleDuration_ = 2.0f;

		// hp量に応じて画面白黒化、ほんの少しのノイズ
		float maxGrayScale_ = 0.5f;
		float maxNoiseIntensity_ = 0.3f;
	};
}