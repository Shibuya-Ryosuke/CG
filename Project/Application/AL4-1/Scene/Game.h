#pragma once
#include <RyoEngine.h>
#include "../Player/Player.h"
#include "../Enemy/Boss/Boss.h"
#include "../Bullet/Bullet.h"
#include "../Bullet/BulletManager.h"
#include "../MapChipField/MapChipField.h"
#include "../FollowCamera/FollowCamera.h"

namespace AL4_1 {
	class Game {
	public:
		// ゲームシーン内の進行
		enum class Phase {
			Intro,		// ボス登場演出
			Battle,		// 戦闘
			BossDefeat,	// ボス撃破演出
			GameOver,	// ゲームオーバー演出
			Clear,		// クリア
		};

		Game();
		~Game();
		// コピー禁止（ユニークポインタを持つので）
		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;
		// ムーブは許可
		Game(Game&&) noexcept = default;
		Game& operator=(Game&&) noexcept = default;

		void Initialize();
		void Update();
		void Draw(RyoEngine::Camera& camera);
		void Finalize();

		// シーンに入るときに呼ぶ（最初から）
		void Start() { Restart(); }
		// クリアしたか（リザルトへ）
		bool IsClear() const { return phase_ == Phase::Clear; }
		// ゲームオーバーからタイトルへ戻りたいか
		bool IsRequestTitle() const { return isRequestTitle_; }

		// ゲームシーン用の追従カメラ
		RyoEngine::Camera& GetCamera() { return followCamera_.GetCamera(); }

	private:
		void SpawnFromMap();

		void Restart();

		void CheckAllCollisions();

		// ---- 進行 ----
		void ChangePhase(Phase next);
		void UpdateIntro();
		void UpdateBattle();
		void UpdateBossDefeat();
		void UpdateGameOver();
		// ゲームオーバー演出などで変えたポストエフェクトを元に戻す
		void ResetPostEffects();

		void RegisterInParamEditor();
	private:
		// マップ
		MapChipField mapChipField_{};

		Player player_{};
		Boss boss_{};

		// プレイヤー追従カメラ
		FollowCamera followCamera_{};

		// ボスの弾（プールで使い回す）
		BulletManager bulletManager_{};
		// 弾の最大数（ParamEditorで変えたら再起動で反映）
		int32_t maxBulletCount_ = 128;

		// リトライ用にプレイヤーのスポーン位置を覚えておく
		RyoEngine::Vector3 playerSpawnPosition_{};

		// ---- 進行 ----
		Phase phase_ = Phase::Intro;
		float phaseTimer_ = 0.0f;		// 今のフェーズに入ってからの経過時間（実時間）
		bool isRequestTitle_ = false;

		// 登場演出
		// ボスが降りきってからアップを続ける時間 → プレイヤーへ戻るのを待つ時間
		float introHoldTime_ = 1.0f;
		float introReturnTime_ = 1.2f;
		// 撃破演出
		// ボスが消えてからアップを続ける時間 → プレイヤーへ戻るのを待つ時間
		float defeatHoldTime_ = 1.0f;
		float defeatReturnTime_ = 1.2f;
		// ゲームオーバー演出
		// この時間でノイズとブラーが最大まで強まる
		float gameOverTime_ = 1.5f;
		float gameOverMaxNoise_ = 0.8f;
		float gameOverMaxBlur_ = 0.02f;
	};
}
