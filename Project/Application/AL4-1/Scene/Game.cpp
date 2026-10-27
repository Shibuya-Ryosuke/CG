#include "Game.h"
#include "../GameResources/GameTex.h"
#include "../GameResources/GameSound.h"
#include <algorithm>

using namespace RyoEngine;
using namespace std;

namespace AL4_1 {
	// CSVに書くスポーン指定の値
	namespace SpawnId {
		constexpr int kPlayer = 2;
		constexpr int kBoss = 5;
	}

	Game::Game() = default;
	Game::~Game() = default;

	void Game::Initialize() {
		// UIなどのテクスチャ読み込み（2回目以降は何もしない）
		GameImage::Initialize();

		// 弾の共有パラメータを登録（インスタンスが増減するので1回だけ）
		Bullet::RegisterInParamEditor();
		RegisterInParamEditor();
		// UIのパラメータを登録
		gameUI_.RegisterInParamEditor();

		// マップ読み込み（ブロックのモデルは 1x1x1・原点が中心のobjを用意する）
		mapChipField_.Load(
			"Resources/ApplicationResources/AL4-1/Csv/stage2.csv",
			"Resources/ApplicationResources/AL4-1/Stage/Block/block.obj");

		// プレイヤーとボス初期化
		player_.Initialize();
		player_.SetMap(&mapChipField_);
		boss_.Initialize();
		boss_.SetTarget(&player_.GetPosition());
		boss_.SetBulletManager(&bulletManager_);

		// 登録が済んだのでスポーン前にロード。プレイヤーhp、死亡フラグをリセット
		ParamEditor::Load();
		player_.ResetStatus();

		// ボスの弾のプール（最大数をロードした後に作る）
		bulletManager_.Initialize("Resources/ApplicationResources/AL4-1/Bullet/bullet.obj", static_cast<uint32_t>(maxBulletCount_));
		bulletManager_.SetMap(&mapChipField_);

		// プレイヤーとボスのスポーン
		SpawnFromMap();
		// ボスはスポーン位置が決まってから登場し直す
		boss_.Reset();

		// 追従カメラ（スポーン位置が決まった後に初期化する）
		followCamera_.Initialize(&mapChipField_);

		// UI（登場演出が終わるまでは非表示）
		gameUI_.Initialize();

		// ボス登場演出から始める
		// （フェードインが終わるまでは Wait で待ち、BeginIntro() で登場演出に入る）
		ChangePhase(Phase::Wait);
	}

	void Game::BeginIntro() {
		if (phase_ != Phase::Wait) { return; }
		ChangePhase(Phase::Intro);
	}

	void Game::Update() {
		// 操作できるのは戦闘中で、入力を受け付けているときだけ（フェード中は止める）
		player_.SetControllable(phase_ == Phase::Battle && isInputEnabled_);

		// 死んだら操作できないようにする
		if (!player_.IsDead()) {
			player_.Update();
		}

		// プレイヤーの移動後にカメラを更新
		followCamera_.Update(player_.GetPosition());

		// ボスはフェードインが終わるまで出さない
		if (phase_ != Phase::Wait) {
			boss_.Update();
		}
		bulletManager_.Update();

		// UI（HPの割合を渡す）
		gameUI_.Update(player_.GetHP() / player_.GetMaxHP(), boss_.GetHP() / boss_.GetMaxHP(), player_.GetGaugeRatio());
		// クリア・失敗の文字が出た瞬間に音を鳴らす
		if (gameUI_.IsClearTextAppeared()) {
			GameSound::PlaySE(GameSound::SE::Clear);
		}
		if (gameUI_.IsFailedTextAppeared()) {
			GameSound::PlaySE(GameSound::SE::Failed);
		}

		// 当たり判定走査（戦闘中だけ）
		if (phase_ == Phase::Battle) {
			CheckAllCollisions();
		}

		// 進行
		switch (phase_) {
		case Phase::Intro:
			UpdateIntro();
			break;
		case Phase::Battle:
			UpdateBattle();
			break;
		case Phase::BossDefeat:
			UpdateBossDefeat();
			break;
		case Phase::ClearResult:
			UpdateClearResult();
			break;
		case Phase::GameOver:
			UpdateGameOver();
			break;
		default:
			break;
		}
	}

	void Game::Draw(RyoEngine::Camera& camera) {
		mapChipField_.Draw(camera);

		player_.Draw(camera);
		// ボスはフェードインが終わるまで出さない
		if (phase_ != Phase::Wait) {
			boss_.Draw(camera);
		}

		bulletManager_.Draw(camera);

#ifdef _DEBUG
		if (boss_.IsCollidable()) {
			PrimitiveRenderer::DrawOBB(boss_.GetOBB(), { 0.0f,1.0f,0.0f,1.0f }, PrimitiveDrawMode::Wireframe);
		}
#endif
		// UI
		gameUI_.Draw();
	}

	void Game::Finalize() {}

	void Game::SpawnFromMap() {
		// CSVのスポーン指定からプレイヤーとボスを配置
		for (const auto& spawn : mapChipField_.GetSpawns()) {
			switch (spawn.id) {
			case SpawnId::kPlayer:
				player_.SetPosition(spawn.position);
				playerSpawnPosition_ = spawn.position;
				break;
			case SpawnId::kBoss:
				boss_.SetHomeFeet(spawn.position);   // 位置だけ覚えておく（定位置は homeOffset 分だけ上）
				break;
			default:
				break;
			}
		}
	}

	void Game::Restart() {
		bulletManager_.Clear();
		ResetPostEffects();

		player_.ResetStatus();
		player_.SetPosition(playerSpawnPosition_);
		boss_.Reset();

		isRequestTitle_ = false;
		isRequestRetry_ = false;
		fullParryBullet_ = nullptr;
		// 前のゲームの振動を残さない
		Input::StopVibration();
		// フェードインが終わるまでは Wait で待つ
		ChangePhase(Phase::Wait);
		// 登場演出はボスのアップから始まるので、ボスの位置へ瞬間移動
		followCamera_.Snap();
	}

	void Game::RegisterInParamEditor() {
		ParamEditor::BeginGroup("Game");
		ParamEditor::RegisterValue("maxBulletCount", &maxBulletCount_, 1.0f, 1, 1000);
		ParamEditor::RegisterValue("menuStickThreshold", &menuStickThreshold_, 0.01f, 0.1f, 1.0f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("GameFlow");
		ParamEditor::RegisterValue("introHoldTime", &introHoldTime_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("introReturnTime", &introReturnTime_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("defeatHoldTime", &defeatHoldTime_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("defeatReturnTime", &defeatReturnTime_, 0.01f, 0.0f, 10.0f);
		ParamEditor::RegisterValue("gameOverTime", &gameOverTime_, 0.01f, 0.01f, 10.0f);
		ParamEditor::RegisterValue("gameOverMaxNoise", &gameOverMaxNoise_, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("gameOverMaxBlur", &gameOverMaxBlur_, 0.0005f, 0.0f, 0.1f);
		ParamEditor::EndGroup();

		ParamEditor::BeginGroup("Vibration");
		RegisterVibration("Small", vibrationSmall_);
		RegisterVibration("Medium", vibrationMedium_);
		RegisterVibration("Large", vibrationLarge_);
		ParamEditor::EndGroup();
	}

	void Game::RegisterVibration(const char* name, Vibration& vibration) {
		ParamEditor::BeginGroup(name);
		ParamEditor::RegisterValue("low", &vibration.low, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("high", &vibration.high, 0.01f, 0.0f, 1.0f);
		ParamEditor::RegisterValue("duration", &vibration.duration, 0.01f, 0.0f, 3.0f);
		ParamEditor::EndGroup();
	}

	//==================================================
	// 進行
	//==================================================

	void Game::ChangePhase(Phase next) {
		phase_ = next;
		phaseTimer_ = 0.0f;

		// フェーズに入った瞬間の処理
		switch (phase_) {
		case Phase::Wait:
			player_.SetControllable(false);
			boss_.SetCanAct(false);
			// 登場位置を先に映しておく（ボスは描画しない）
			followCamera_.SetFocus(boss_.GetCenter());
			// 登場演出が終わるまでUIは出さない
			gameUI_.Reset();
			break;

		case Phase::Intro:
			player_.SetControllable(false);
			boss_.SetCanAct(false);
			followCamera_.SetFocus(boss_.GetCenter());
			// 登場演出が終わるまでUIは出さない
			gameUI_.Reset();
			break;

		case Phase::Battle:
			player_.SetControllable(true);
			boss_.SetCanAct(true);
			// 登場演出が終わったのでHPバーを出す
			gameUI_.SetHpVisible(true);
			// クリアタイムを計り始める
			clearTime_ = 0.0f;
			break;

		case Phase::BossDefeat:
			player_.SetControllable(false);
			bulletManager_.Clear();
			followCamera_.SetFocus(boss_.GetCenter());
			// 撃破アニメーションが始まったらHPバーとゲージを消していく
			gameUI_.SetHpVisible(false);
			break;

		case Phase::ClearResult:
			player_.SetControllable(false);
			// プレイヤーをこちらに向かせて、横に情報を出せるようにズームする
			player_.SetFaceCamera(true);
			followCamera_.SetZoomFocus(&player_.GetPosition());
			// クリアの文字 → クリアタイム → メニューの順に出す（HPバーとゲージは撃破時に消している）
			gameUI_.ShowClearResult(clearTime_);
			// 振動を残さない
			Input::StopVibration();
			menuSelection_ = 0;
			gameUI_.ShowMenu(true);
			gameUI_.SetMenuSelection(menuSelection_);
			break;

		case Phase::GameOver:
		{
			boss_.SetCanAct(false);
			// 振動を残さない
			Input::StopVibration();
			// 致命傷を受けたのでHPバーとゲージを消していき、失敗の文字を出す
			gameUI_.SetHpVisible(false);
			gameUI_.ShowFailedText();
			// 被弾・パリィの演出が途中で止まったままにならないように戻す
			SetTimeScale(1.0f);
			auto ppInstance = PostProcess::GetInstance();
			ppInstance->SetChromaticAberrationEnabled(false);
			ppInstance->SetChromaticAberrationStrength(0.0f);
			ppInstance->SetNoiseEnabled(true);
			ppInstance->SetBlurEnabled(true);
			ppInstance->SetGlitchEnabled(true);
			break;
		}

		default:
			break;
		}
	}

	void Game::UpdateIntro() {
		// ボスが降りきるまではアップのまま
		if (boss_.GetState() == Boss::State::Appear) { return; }

		phaseTimer_ += GetDeltaTime();
		// 少しアップを続けてから、プレイヤーへ戻る
		if (followCamera_.IsFocusing() && phaseTimer_ >= introHoldTime_) {
			followCamera_.ClearFocus();
		}
		// 戻りきったら戦闘開始
		if (phaseTimer_ >= introHoldTime_ + introReturnTime_) {
			ChangePhase(Phase::Battle);
		}
	}

	void Game::UpdateBattle() {
		// クリアタイム（パリィのスローで得をしないように実時間で計る）
		clearTime_ += GetDeltaTime();

		if (player_.IsDead()) {
			ChangePhase(Phase::GameOver);
			return;
		}
		// 撃破アニメーションが始まったら撃破演出へ
		if (boss_.GetState() == Boss::State::Destroy) {
			ChangePhase(Phase::BossDefeat);
		}
	}

	void Game::UpdateBossDefeat() {
		// 撃破アニメーションが終わるまではアップのまま
		if (!boss_.IsDead()) { return; }

		phaseTimer_ += GetDeltaTime();
		// 少しアップを続けてから、プレイヤーのズーム（クリア結果）へ
		if (phaseTimer_ >= defeatHoldTime_) {
			ResetPostEffects();
			ResetPlayer();
			ChangePhase(Phase::ClearResult);
		}
	}

	void Game::UpdateClearResult() {
		player_.SetIntensity(0.0f);
		UpdateMenu();
	}

	void Game::UpdateGameOver() {
		phaseTimer_ += GetDeltaTime();

		// ノイズとブラーをだんだん強くする
		float t = std::clamp(phaseTimer_ / gameOverTime_, 0.0f, 1.0f);
		auto ppInstance = PostProcess::GetInstance();
		ppInstance->SetNoiseIntensity(EaseInCubic(t, 0.0f, gameOverMaxNoise_));
		ppInstance->SetBlurStrength(EaseInCubic(t, 0.0f, gameOverMaxBlur_));

		// 演出が終わったらメニューを出す（W/Sで選んでSpaceで決定）
		if (t < 1.0f) { return; }
		if (!gameUI_.IsMenuVisible()) {
			menuSelection_ = 0;
			gameUI_.ShowMenu(false);
			gameUI_.SetMenuSelection(menuSelection_);
		}
		UpdateMenu();
	}

	void Game::UpdateMenu() {
		// Lスティックは倒した瞬間だけ反応させる（前フレームの状態はメニューが出る前から更新しておく）
		const float stickY = Input::GetLeftStickY();
		const bool isStickUp = stickY > menuStickThreshold_;
		const bool isStickDown = stickY < -menuStickThreshold_;
		const bool stickUpTrigger = isStickUp && !isMenuStickUp_;
		const bool stickDownTrigger = isStickDown && !isMenuStickDown_;
		isMenuStickUp_ = isStickUp;
		isMenuStickDown_ = isStickDown;

		// フェード中と、メニューが出きるまでは受け付けない
		if (!isInputEnabled_ || !gameUI_.IsMenuReady()) { return; }

		const int32_t count = static_cast<int32_t>(GameUI::MenuItem::Count);
		// ゲームパッドは十字キーの上下かLスティック
		const bool padUp = Input::GetJoystickTrigger(XINPUT_GAMEPAD_DPAD_UP) || stickUpTrigger;
		const bool padDown = Input::GetJoystickTrigger(XINPUT_GAMEPAD_DPAD_DOWN) || stickDownTrigger;
		if (Input::TriggerKey(DIK_W) || Input::TriggerKey(DIK_UP) || padUp) {
			menuSelection_ = (menuSelection_ + count - 1) % count;
			GameSound::PlaySE(GameSound::SE(GameSound::SE::Cursor));
		}
		if (Input::TriggerKey(DIK_S) || Input::TriggerKey(DIK_DOWN) || padDown) {
			menuSelection_ = (menuSelection_ + 1) % count;
			GameSound::PlaySE(GameSound::SE(GameSound::SE::Cursor));
		}
		gameUI_.SetMenuSelection(menuSelection_);

		// 決定（ゲームパッドはA）
		if (Input::TriggerKey(DIK_SPACE) || Input::GetJoystickTrigger(XINPUT_GAMEPAD_A)) {
			if (menuSelection_ == static_cast<int32_t>(GameUI::MenuItem::Retry)) {
				// リトライはフェードを挟むので、SceneManager に Start() し直してもらう
				isRequestRetry_ = true;
			} else {
				isRequestTitle_ = true;
			}
			GameSound::PlaySE(GameSound::SE::Decide);
		}
	}

	void Game::ResetPostEffects() {
		// ポストプロセスのインスタンス取得
		auto ppInstance = PostProcess::GetInstance();
		// 各ポストプロセス初期化
		ppInstance->SetGrayscaleEnabled(false);
		ppInstance->SetGrayscaleIntensity(0.0f);
		ppInstance->SetNoiseEnabled(false);
		ppInstance->SetNoiseIntensity(0.0f);
		ppInstance->SetBlurEnabled(false);
		ppInstance->SetBlurStrength(0.0f);
		ppInstance->SetChromaticAberrationEnabled(false);
		ppInstance->SetChromaticAberrationStrength(0.0f);
		ppInstance->SetGlitchEnabled(false);
		// 時間も
		SetTimeScale(1.0f);
	}

	void Game::ResetPlayer() {
		player_.ResetStatus();
		player_.InvincibeleDisabled();
	}

	void Game::CheckAllCollisions() {
		if (player_.IsDead()) return;

		const RyoEngine::OBB& playerOBB = player_.GetOBB();
		// 攻撃中はパリィも受け付ける
		const bool canParry = player_.IsAttackActive();
		// 攻撃判定がそのままパリィ判定になる
		const RyoEngine::OBB& parryOBB = player_.GetAttackOBB();

		// プレイヤーとボスの弾、パリィで跳ね返した弾とボスの判定
		for (auto& bullet : bulletManager_.GetBullets()) {
			if (bullet.IsDead()) {
				// フルパリィの弾がボス以外で消えたら覚えておくのをやめる（プールで使い回されるので）
				if (&bullet == fullParryBullet_) { fullParryBullet_ = nullptr; }
				continue;
			}

			// パリィされた弾はボスにだけ当たる
			if (bullet.IsReflected()) {
				if (boss_.IsCollidable() && IsCollision(bullet.GetOBB(), boss_.GetOBB())) {
					// フルパリィで跳ね返した弾なら大ダメージ＋感電
					const bool isFullParryBullet = &bullet == fullParryBullet_;
					const float damage = bullet.GetDamage() + (isFullParryBullet ? player_.GetFullParryDamage() : 0.0f);
					boss_.OnDamage(damage);
					// ボスに当たった手応え（小）
					Vibrate(vibrationSmall_);
					if (isFullParryBullet) {
						boss_.StartShock();
						fullParryBullet_ = nullptr;
					}
					bullet.OnCollision();
				}
				continue;
			}

			// パリィ受付中に判定へ触れたらはね返す
			if (canParry && IsCollision(parryOBB, bullet.GetOBB())) {
				bullet.Reflect(boss_.GetCenter());
				// ゲージ満タンでのパリィ（フルパリィ）なら、この弾がボスに当たったときに大ダメージ
				if (player_.OnParrySuccess(false)) {
					fullParryBullet_ = &bullet;
					// フルパリィの瞬間はプレイヤーへ素早く寄る
					followCamera_.StartPunchZoom(&player_.GetPosition());
					// フルパリィ（大）
					Vibrate(vibrationLarge_);
				} else {
					// 弾のパリィ（中）
					Vibrate(vibrationMedium_);
				}
				continue;
			}

			// プレイヤーと弾の判定
			if (IsCollision(playerOBB, bullet.GetOBB())) {
				const float hpBefore = player_.GetHP();
				player_.OnDamage(bullet.GetDamage());
				// 実際にダメージが入ったときだけ（無敵中は震えない）
				if (player_.GetHP() < hpBefore) {
					Vibrate(vibrationMedium_);
				}
				bullet.OnCollision();
			}
		}

		// プレイヤーとボスの近接攻撃判定
		if (boss_.IsMeleeActive()) {
			const RyoEngine::OBB& meleeOBB = boss_.GetMeleeOBB();
			if (canParry && IsCollision(parryOBB, meleeOBB)) {
				// カウンター成功
				boss_.OnCountered();
				// ゲージ満タンでのカウンター（フルパリィ）なら、その場で大ダメージ＋感電
				if (player_.OnParrySuccess(true)) {
					boss_.OnDamage(player_.GetFullParryDamage());
					boss_.StartShock();
					// フルパリィの瞬間はプレイヤーへ素早く寄る
					followCamera_.StartPunchZoom(&player_.GetPosition());
					// フルパリィ（大）
					Vibrate(vibrationLarge_);
				}
				// 近接のパリィ（大）
				Vibrate(vibrationLarge_);
				player_.SetAttackHitDone();
			} else if (!boss_.IsMeleeHitDone() && IsCollision(playerOBB, meleeOBB)) {
				const float hpBefore = player_.GetHP();
				player_.OnDamage(boss_.GetMeleeDamage());
				// 実際にダメージが入ったときだけ（無敵中は震えない）
				if (player_.GetHP() < hpBefore) {
					Vibrate(vibrationMedium_);
				}
				boss_.SetMeleeHitDone();
			}
		}

		// プレイヤーの通常攻撃とボスの判定
		if (player_.IsAttackActive() && !player_.IsAttackHitDone() && boss_.IsCollidable() &&
			IsCollision(player_.GetAttackOBB(), boss_.GetOBB())) {
			boss_.OnDamage(player_.GetAttackDamage());
			// ボスに当たった手応え（小）
			Vibrate(vibrationSmall_);
			player_.SetAttackHitDone();
		}
	}
}