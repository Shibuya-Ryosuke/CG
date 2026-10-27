#pragma once
#include <RyoEngine.h>
#include "../Player/Player.h"
#include "../Enemy/Boss/Boss.h"
#include "../Bullet/Bullet.h"
#include "../Bullet/BulletManager.h"
#include "../MapChipField/MapChipField.h"
#include "../FollowCamera/FollowCamera.h"
#include "../UI/GameUI.h"

namespace AL4_1 {
	class Game {
	public:
		// ゲームシーン内の進行
		enum class Phase {
			Wait,		// フェードインが終わるのを待つ（ボスはまだ出さない）
			Intro,		// ボス登場演出
			Battle,		// 戦闘
			BossDefeat,	// ボス撃破演出
			ClearResult,	// クリア結果（プレイヤーのズーム・クリアタイム・メニュー）
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
		// ゲームオーバーからリトライしたいか（フェードを挟んで Start() し直してもらう）
		bool IsRequestRetry() const { return isRequestRetry_; }
		// フェードインが終わったら呼ぶ（ボス登場演出を始める）
		void BeginIntro();
		// キー入力を受け付けるか（フェード中は false にする）
		void SetInputEnabled(bool enabled) { isInputEnabled_ = enabled; }

		// ゲームシーン用の追従カメラ
		RyoEngine::Camera& GetCamera() { return followCamera_.GetCamera(); }
		// プレイヤーの足元の位置（背景のループの基準に使う）
		const RyoEngine::Vector3& GetPlayerPosition() const { return player_.GetPosition(); }

		// ゲームオーバー演出などで変えたポストエフェクトを元に戻す
		void ResetPostEffects();

		void ResetPlayer();

		// ---- BGM（音量そのものは SceneManager が決める。ここでは倍率だけ持つ） ----
		// ゲームのBGMの倍率（撃破・ゲームオーバーで0まで下げる）
		float GetGameBGMRate() const { return gameBGMRate_; }
		// クリアのBGMの倍率（流し始めてから0→1に上げる）
		float GetClearBGMRate() const { return clearBGMRate_; }
		// クリアのBGMを流し始めるフレームだけ true
		bool IsClearBGMStartTrigger() const { return isClearBGMStartTrigger_; }
		// クリアのBGMを流しているか
		bool IsClearBGMPlaying() const { return isClearBGMPlaying_; }
	private:
		void SpawnFromMap();

		void Restart();

		void CheckAllCollisions();

		// ---- 進行 ----
		void ChangePhase(Phase next);
		void UpdateIntro();
		void UpdateBattle();
		void UpdateBossDefeat();
		void UpdateClearResult();
		// メニューの操作（W/Sで選んでSpaceで決定）。クリア時とゲームオーバー時で共通
		void UpdateMenu();
		void UpdateGameOver();

		void RegisterInParamEditor();

		// ---- BGM ----
		// ゲームのBGMを下げ始める
		void StartGameBGMFadeOut();
		// BGMの倍率を進める（クリアのSEが鳴った後にクリアのBGMを流す）
		void UpdateBGM();
		// BGMの倍率を最初の状態に戻す
		void ResetBGM();

		// ---- 振動（小・中・大） ----
		struct Vibration {
			float low = 0.0f;		// 左の重いモーター（0～1）
			float high = 0.0f;		// 右の軽いモーター（0～1）
			float duration = 0.0f;	// 秒（実時間）
		};
		void Vibrate(const Vibration& vibration) { RyoEngine::Input::Vibrate(vibration.low, vibration.high, vibration.duration); }
		void RegisterVibration(const char* name, Vibration& vibration);
	private:
		// マップ
		MapChipField mapChipField_{};

		Player player_{};
		Boss boss_{};

		// プレイヤー追従カメラ
		FollowCamera followCamera_{};

		// UI（HPバー・クリア/失敗の文字）
		GameUI gameUI_{};

		// ボスの弾（プールで使い回す）
		BulletManager bulletManager_{};
		// 弾の最大数（ParamEditorで変えたら再起動で反映）
		int32_t maxBulletCount_ = 128;

		// リトライ用にプレイヤーのスポーン位置を覚えておく
		RyoEngine::Vector3 playerSpawnPosition_{};

		// ---- 進行 ----
		Phase phase_ = Phase::Wait;
		float phaseTimer_ = 0.0f;		// 今のフェーズに入ってからの経過時間（実時間）
		bool isRequestTitle_ = false;
		bool isRequestRetry_ = false;
		// クリアタイム（戦闘開始からボス撃破まで。実時間で計る）
		float clearTime_ = 0.0f;
		// メニューで選んでいる項目（GameUI::MenuItem の番号）
		int32_t menuSelection_ = 0;
		// フルパリィで跳ね返した弾（ボスに当たったら大ダメージ＋感電）。弾はプールなので、消えたら nullptr に戻す
		const Bullet* fullParryBullet_ = nullptr;
		// キー入力を受け付けるか
		bool isInputEnabled_ = true;
		// メニューをLスティックで動かすとき、倒した瞬間だけ反応させるための前フレームの状態
		bool isMenuStickUp_ = false;
		bool isMenuStickDown_ = false;
		float menuStickThreshold_ = 0.5f;	// これ以上倒したら「倒した」とみなす

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

		// BGM（倍率は SceneManager の音量に掛ける）
		float gameBGMRate_ = 1.0f;
		float clearBGMRate_ = 0.0f;
		bool isGameBGMFadingOut_ = false;	// ゲームのBGMを下げている途中か
		float gameBGMFadeTimer_ = 0.0f;
		bool isWaitingClearBGM_ = false;	// クリアのSEが鳴って、クリアのBGMを待っているか
		float clearBGMWaitTimer_ = 0.0f;
		bool isClearBGMStartTrigger_ = false;
		bool isClearBGMPlaying_ = false;
		float clearBGMFadeTimer_ = 0.0f;
		float bgmFadeOutTime_ = 0.8f;		// ゲームのBGMを0まで下げる時間
		float clearBGMDelay_ = 1.5f;		// クリアのSEが鳴ってから、クリアのBGMを流すまでの時間
		float clearBGMFadeInTime_ = 1.0f;	// クリアのBGMを上げきるまでの時間

		// 振動
		Vibration vibrationSmall_{ 0.15f, 0.35f, 0.08f };	// 攻撃・跳ね返した弾がボスに当たった
		Vibration vibrationMedium_{ 0.5f, 0.5f, 0.25f };	// プレイヤーが被弾した
		Vibration vibrationLarge_{ 1.0f, 0.8f, 0.5f };		// フルパリィした
	};
}