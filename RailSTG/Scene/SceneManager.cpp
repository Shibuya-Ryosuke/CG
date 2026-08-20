#include "SceneManager.h"
#include "Game/Game.h"
#include "../Particle/ParticleManager.h"

using namespace RyoEngine;

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

void SceneManager::Initialize(Scene sceneState) {
    // ゲームカメラ(レール自動移動カメラ)
    camera_ = std::make_unique<RailCameraController>();
    camera_->Initialize();
    camera_->SetActive(true);

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

    // デバッグカメラ
    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize();
    debugCamera_->SetTranslateY(153.0f);
    debugCamera_->SetTranslateZ(-25.0f);

    // 天球
    skydome_ = Model::Create("resources/RailSTG/Skydome/skydome.obj");
    //skydome_->SetTex("resources/uvChecker.png");
    skydome_->SetLambert(ShadingMode::NONE);
    skydome_->SetScale({ 10.0f,10.0f,10.0f });
    // 地面
    ground_ = Model::Create("resources/RailSTG/Ground/ground.obj");
    //ground_->SetScale({ 10.0f,10.0f,10.0f });
    // 
    // パーティクルマネージャー
    ParticleManager::GetInstance().Initialize();
    // シーン（enum）
    scene_ = sceneState;

    // シーン別
    game_ = std::make_unique<Game>();
    game_->Initialize();
    // フェーズ別制限時間のセット
    std::vector<float> timeLimits;
    for (const auto& route : phaseRoutes_) {
        timeLimits.push_back(route.timeLimit);
    }
    game_->SetPhaseTimeLimits(timeLimits);
    game_->SetChangingDuration(kChangingDuration_);
}

void SceneManager::Finalize() {
    ParticleManager::GetInstance().Finalize();
}

void SceneManager::Update() {
    // アクティブカメラの決定とその更新
    UpdateCamera();

    skydome_->Update(*activeCamera_);
    ground_->Update(*activeCamera_);

    ParticleManager::GetInstance().Update(*activeCamera_);

    switch (scene_) {
    case Scene::Title:
        break;

    case Scene::Game:
        game_->Update(*activeCamera_);
        CheckPhaseChange(); // game更新後にフェーズ変化をチェック
        break;

    case Scene::Result:
        break;

    case Scene::None:
    default:
        break;
    }
}

void SceneManager::Draw() {
    skydome_->Draw();
    ground_->Draw();

    ParticleManager::GetInstance().Draw();

    switch (scene_) {
    case Scene::Title:
        break;

    case Scene::Game:
        game_->Draw();
        break;

    case Scene::Result:
        break;

    case Scene::None:
    default:
        break;
    }
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
