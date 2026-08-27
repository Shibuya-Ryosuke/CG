#include "SceneManager.h"
#include "Title/Title.h"
#include "Game/Game.h"
#include "Result/Result.h"
#include "../Particle/ParticleManager.h"
#include "../Time/TimeManager.h"
#include "../GameSound/GameSound.h"
#include <imgui.h>
#include <cmath>

using namespace RyoEngine;

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

void SceneManager::Initialize(Scene sceneState) {
    // ゲームカメラ(レール自動移動カメラ)
    camera_ = std::make_unique<RailCameraController>();
    camera_->Initialize();
    camera_->SetTranslate({ 0.0f,153.0f,-25.0f });
    camera_->SetActive(true);

    RouteInitialize();

    // デバッグカメラ
    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize();
    debugCamera_->SetTranslateY(153.0f);
    debugCamera_->SetTranslateZ(-25.0f);

    // 天球
    skydome_ = Model::Create("resources/RailSTG/Model/Skydome/skydome.obj");
    //skydome_->SetTex("resources/uvChecker.png");
    skydome_->SetLambert(ShadingMode::NONE);
    skydome_->SetScale({ 10.0f,10.0f,10.0f });
    skydome_->SetUVScale({ 2.0f,4.0f });
    // 地面
    ground_ = Model::Create("resources/RailSTG/Model/Ground/ground4.obj");
    ground_->SetUVScale({ 4.0f,4.0f },0);
    //ground_->SetScale({ 10.0f,10.0f,10.0f });
     
    pressSpace_.Initialize("resources/RailSTG/UI/Input/pressSpace.png", { 640.0f,450.0f }, Anchor::Center);

    // シーン（enum）
    scene_ = sceneState;
    camera_->SetIdleRotate(scene_ != Scene::Game); // Game以外はその場回転

    // 起動時(Title)はフェードインから始める
    fade_.StartFadeIn(kFadeInDurationFrames_);

    // シーン別
    // タイトル
    title_ = std::make_unique<Title>();
    title_->Initialize();

    // ゲーム
    game_ = std::make_unique<Game>();
    game_->Initialize();
    // フェーズ別制限時間のセット
    std::vector<float> timeLimits;
    for (const auto& route : phaseRoutes_) {
        timeLimits.push_back(route.timeLimit);
    }
    game_->SetPhaseTimeLimits(timeLimits);
    game_->SetChangingDuration(kChangingDuration_);

    // リザルト
    result_ = std::make_unique<Result>();
    result_->Initialize();

    // 音読み込み
    GameSound::Initialize();

    // bgm
    GameSound::PlayBGM(GameSound::BGM::Title);

}

void SceneManager::Finalize() {
   
}

void SceneManager::Update() {
    TimeManager::Update();

    fade_.Update();

    // フェードアウトが完了した瞬間に実際のシーン切り替えを行う
    if (fade_.IsFadeOutJustFinished()) {
        scene_ = pendingScene_;
        fade_.StartFadeIn(kFadeInDurationFrames_);
        camera_->SetIdleRotate(scene_ != Scene::Game); // 追加: Game以外はその場回転
        if (pendingScene_ == Scene::Game) {
            camera_->SetTranslate({ 0.0f,153.0f,-25.0f });
            GameSound::StopBGM(GameSound::BGM::Title);
            GameSound::PlayBGM(GameSound::BGM::Game);
        } else if (pendingScene_ == Scene::Title) {
            RouteInitialize();
            GameSound::StopBGM(GameSound::BGM::Game);
            GameSound::StopBGM(GameSound::BGM::Clear);
            GameSound::StopBGM(GameSound::BGM::Failed);
            GameSound::PlayBGM(GameSound::BGM::Title);
            pressSpace_.SetTranslate({ 640.0f,450.0f });
        } else if (pendingScene_ == Scene::Result) {
            GameSound::StopBGM(GameSound::BGM::Game);
            if (game_->IsClear()) {
                GameSound::PlayBGM(GameSound::BGM::Clear);
                result_->SetInfo(
                    game_->GetTotalSpawnEnemies(),
                    game_->GetTotalDestroyEnemies(),
                    game_->GetPlayerJustEvasionNumber(),
                    game_->GetPlayerRemainingHP(),
                    game_->GetClearTime()
                );
                pressSpace_.SetTranslate({ 640.0f,610.0f });
            } else {
                GameSound::PlayBGM(GameSound::BGM::Failed);
            }
            result_->SetIsClear(game_->IsClear());
        }
    }

    // アクティブカメラの決定とその更新
    UpdateCamera();
 
    skydome_->Update(*activeCamera_);
    ground_->Update(*activeCamera_);

   

    switch (scene_) {
    case Scene::Title:
        title_->Update();
        PressSpaceFade();

        camera_->SetIdleRotate(true);
        if (camera_->GetTranslate().z >= 500.0f) {
            camera_->SetTranslateZ(-25.0f);
        }

        // フェード中でない(=遷移待ちでない)ときだけ入力を受け付ける
        if (fade_.IsIdle() && Input::TriggerKey(DIK_SPACE)) {
            pendingScene_ = Scene::Game;
            game_->Initialize();
            fade_.StartFadeOut(kFadeOutDurationFrames_);
            GameSound::PlaySE(GameSound::SE::Decision,0.22f);
        }
        break;

    case Scene::Game:
        game_->Update(*activeCamera_);
        CheckPhaseChange(); // game更新後にフェーズ変化をチェック

        // 追加: Third終了(End)またはHP0でResultへ
        if (fade_.IsIdle() && (game_->GetPhase() == Phase::End || game_->IsPlayerDead())) {
            pendingScene_ = Scene::Result;
            result_->Initialize();
            fade_.StartFadeOut(kFadeOutDurationFrames_);
        }

        // ポーズからタイトルへ戻る
        if (fade_.IsIdle() && game_->IsBackToTitle()) {
            pendingScene_ = Scene::Title;
            title_->Initialize();
            fade_.StartFadeOut(kFadeOutDurationFrames_);
            // Defaultにしないとカメラが回転しない
            TimeManager::SetTimeState(TimeState::Default);
        }
        break;

    case Scene::Result:
        result_->Update();
        PressSpaceFade();

        // フェード中でない(=遷移待ちでない)ときだけ入力を受け付ける
        if (fade_.IsIdle() && Input::TriggerKey(DIK_SPACE)) {
            pendingScene_ = Scene::Title;
            title_->Initialize();
            fade_.StartFadeOut(kFadeOutDurationFrames_);
            GameSound::PlaySE(GameSound::SE::Decision,0.22f);
        }
        break;

    case Scene::None:
    default:
        break;
    }
}

void SceneManager::Draw() {
    skydome_->Draw();
    ground_->Draw();

   

    switch (scene_) {
    case Scene::Title:
        title_->Draw();
        pressSpace_.Draw();
        break;

    case Scene::Game:
        game_->Draw();
        break;

    case Scene::Result:
        result_->Draw();
        pressSpace_.Draw();
        break;

    case Scene::None:
    default:
        break;
    }

    fade_.Draw(); // 最前面に重ねて描画
}

void SceneManager::PressSpaceFade() {
    // タイマーを進める（毎フレーム 1 ずつ増やす）
    pressSpaceTimer_ ++;

    // 60フレームで1周期（0〜2*PI）になるように計算
    // ※もし「もっとゆっくり（例: 2秒で1往復）」にしたい場合は 60.0f の部分を 120.0f に変えてください
    float alpha = (std::sin(float(pressSpaceTimer_) * (2.0f * float(M_PI)) / 60.0f) + 1.0f) * 0.5f;
    pressSpace_.SetColor({ 1.0f,1.0f,1.0f,alpha });
    pressSpace_.Update();
}

void SceneManager::UpdateCamera() {
#ifdef _DEBUG
    // アクティブの反転(お試し)
    if (Input::TriggerKey(DIK_K)) {
        if (camera_->IsActive()) {
            camera_->SetActive(false);
        } else {
            camera_->SetActive(true);
        }
    }
    // デバッグカメラの時画面上かどうかの判定（ImGuiの操作中動くのを防ぐため）
    if (!camera_->IsActive()) {
        debugCamera_->SetAvailable(RyoEngine::GetOnTheGameView());
    }
    // テキストの表示
    if (camera_->IsActive()) {
        PrintText("camera", { 10,10 });
    } else {
        PrintText("debug", { 10,10 });
    }

    // アクティブカメラを決定
    activeCamera_ = camera_->IsActive() ? static_cast<RyoEngine::Camera*>(camera_.get()) : static_cast<RyoEngine::Camera*>(debugCamera_.get());
    activeCamera_->Update();
#else
    // リリース時はゲームカメラで固定
    activeCamera_ = camera_.get();
    activeCamera_->Update();
#endif
    // 即時描画のカメラ指定
    PrimitiveRenderer::SetCamera(*activeCamera_);
}

void SceneManager::CheckPhaseChange() {
    Phase currentPhase = game_->GetPhase();
    if (currentPhase == lastPhase_) {
        return; // 変化なし
    }
    lastPhase_ = currentPhase;

    if (currentPhase == Phase::Changing) {
        camera_->EnterChangingStraight(kChangingDuration_); // 追加: 直進演出開始
    } else if (currentPhase == Phase::First || currentPhase == Phase::Second || currentPhase == Phase::Third) {
        camera_->ConnectToNextPhase(phaseRoutes_[PhaseToRouteIndex(currentPhase)].wayPoints);
    }
}

void SceneManager::RouteInitialize() {
    // フェーズごとの経路データを構築（First:35s, Second:45s, Third:60s）
    phaseRoutes_ = {
     { { // First (絶対座標 / 35秒用：シンプルに長めの直線を想定)
         {   0.0f, 153.0f,   -25.0f },
         {   0.0f, 153.0f,  1000.0f }, // 時間が長いため終点を少し遠くに延長
     }, 35.0f },
     { { // Second (相対座標 / 45秒用：ポイントを少し増やしてカーブを滑らかに)
         {   0.0f,   0.0f,    0.0f },
         {  20.0f,   0.0f,  100.0f },
         {  50.0f,   0.0f,  220.0f },
         {  85.0f,   0.0f,  360.0f },
         { 103.4f,   0.0f,  500.0f },
         { 110.0f,   0.0f,  650.0f },
         { 135.0f,   0.0f,  780.0f },
         { 160.0f,   0.0f,  920.0f },
         { 180.8f,   0.0f, 1050.0f }, // 時間増加に合わせて終点も調整
     }, 45.0f },
     { { // Third (相対座標 / 60秒用：一番時間が長いため、高低差とカーブを増やした長丁場な経路)
         {   0.0f,   0.0f,    0.0f },
         {  30.0f,  10.0f,  100.0f },
         {  60.0f,  25.0f,  220.0f },
         { 101.6f,  41.8f,  350.0f },
         { 120.0f,  50.0f,  480.0f },
         { 127.6f,  52.3f,  620.0f },
         { 127.6f,  45.0f,  760.0f },
         { 127.6f,  30.0f,  900.0f },
         { 127.6f,  15.0f, 1040.0f },
         { 127.6f,   0.0f, 1200.0f }, // 60秒かけて進むロングコース
     }, 60.0f },
    };

    // 最初のフェーズ(First)の経路をセット(スタートなので今まで通りSetWayPointsでOK)
    camera_->SetWayPoints(phaseRoutes_[PhaseToRouteIndex(Phase::First)].wayPoints);
}
