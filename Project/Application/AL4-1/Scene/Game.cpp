#include "Game.h"
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
		// 弾の共有パラメータを登録（インスタンスが増減するので1回だけ）
		Bullet::RegisterInParamEditor();
		RegisterInParamEditor();

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

		// ボス登場演出から始める
		ChangePhase(Phase::Intro);
	}

	void Game::Update() {
		// 死んだら操作できないようにする
		if (!player_.IsDead()) {
			player_.Update();
		}

		// プレイヤーの移動後にカメラを更新
		followCamera_.Update(player_.GetPosition());

		boss_.Update();
		bulletManager_.Update();

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
		boss_.Draw(camera);

		bulletManager_.Draw(camera);

		if (boss_.IsCollidable()) {
			PrimitiveRenderer::DrawOBB(boss_.GetOBB(), { 0.0f,1.0f,0.0f,1.0f }, PrimitiveDrawMode::Wireframe);
		}
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
		ChangePhase(Phase::Intro);
		// 登場演出はボスのアップから始まるので、ボスの位置へ瞬間移動
		followCamera_.Snap();
	}

	void Game::RegisterInParamEditor() {
		ParamEditor::BeginGroup("Game");
		ParamEditor::RegisterValue("maxBulletCount", &maxBulletCount_, 1.0f, 1, 1000);
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
	}

	//==================================================
	// 進行
	//==================================================

	void Game::ChangePhase(Phase next) {
		phase_ = next;
		phaseTimer_ = 0.0f;

		// フェーズに入った瞬間の処理
		switch (phase_) {
		case Phase::Intro:
			player_.SetControllable(false);
			boss_.SetCanAct(false);
			followCamera_.SetFocus(boss_.GetCenter());
			break;

		case Phase::Battle:
			player_.SetControllable(true);
			boss_.SetCanAct(true);
			break;

		case Phase::BossDefeat:
			player_.SetControllable(false);
			bulletManager_.Clear();
			followCamera_.SetFocus(boss_.GetCenter());
			break;

		case Phase::GameOver:
		{
			boss_.SetCanAct(false);
			// 被弾・パリィの演出が途中で止まったままにならないように戻す
			SetTimeScale(1.0f);
			auto ppInstance = PostProcess::GetInstance();
			ppInstance->SetChromaticAberrationEnabled(false);
			ppInstance->SetChromaticAberrationStrength(0.0f);
			ppInstance->SetNoiseEnabled(true);
			ppInstance->SetBlurEnabled(true);
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
		if (followCamera_.IsFocusing() && phaseTimer_ >= defeatHoldTime_) {
			followCamera_.ClearFocus();
		}
		// 戻りきったらクリア（SceneManager がリザルトへ）
		if (phaseTimer_ >= defeatHoldTime_ + defeatReturnTime_) {
			ResetPostEffects();
			ChangePhase(Phase::Clear);
		}
	}

	void Game::UpdateGameOver() {
		phaseTimer_ += GetDeltaTime();

		// ノイズとブラーをだんだん強くする
		float t = std::clamp(phaseTimer_ / gameOverTime_, 0.0f, 1.0f);
		auto ppInstance = PostProcess::GetInstance();
		ppInstance->SetNoiseIntensity(EaseInCubic(t, 0.0f, gameOverMaxNoise_));
		ppInstance->SetBlurStrength(EaseInCubic(t, 0.0f, gameOverMaxBlur_));

		// 演出が終わったら R:リトライ T:タイトル
		if (t < 1.0f) { return; }
		if (Input::TriggerKey(DIK_R)) {
			Restart();
		} else if (Input::TriggerKey(DIK_T)) {
			ResetPostEffects();
			isRequestTitle_ = true;
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
		// 時間も
		SetTimeScale(1.0f);
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
			if (bullet.IsDead()) continue;

			// パリィされた弾はボスにだけ当たる
			if (bullet.IsReflected()) {
				if (boss_.IsCollidable() && IsCollision(bullet.GetOBB(), boss_.GetOBB())) {
					boss_.OnDamage(bullet.GetDamage());
					bullet.OnCollision();
				}
				continue;
			}

			// パリィ受付中に判定へ触れたらはね返す
			if (canParry && IsCollision(parryOBB, bullet.GetOBB())) {
				bullet.Reflect(boss_.GetCenter());
				player_.OnParrySuccess(false);
				continue;
			}

			// プレイヤーと弾の判定
			if (IsCollision(playerOBB, bullet.GetOBB())) {
				player_.OnDamage(bullet.GetDamage());
				bullet.OnCollision();
			}
		}

		// プレイヤーとボスの近接攻撃判定
		if (boss_.IsMeleeActive()) {
			const RyoEngine::OBB& meleeOBB = boss_.GetMeleeOBB();
			if (canParry && IsCollision(parryOBB, meleeOBB)) {
				// カウンター成功
				boss_.OnCountered();
				player_.OnParrySuccess(true);
				player_.SetAttackHitDone();
			} else if (!boss_.IsMeleeHitDone() && IsCollision(playerOBB, meleeOBB)) {
				player_.OnDamage(boss_.GetMeleeDamage());
				boss_.SetMeleeHitDone();
			}
		}

		// プレイヤーの通常攻撃とボスの判定
		if (player_.IsAttackActive() && !player_.IsAttackHitDone() && boss_.IsCollidable() &&
			IsCollision(player_.GetAttackOBB(), boss_.GetOBB())) {
			boss_.OnDamage(player_.GetAttackDamage());
			player_.SetAttackHitDone();
		}
	}
}
