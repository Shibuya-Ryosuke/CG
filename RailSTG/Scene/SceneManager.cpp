#include "SceneManager.h"
#include "Game.h"
#include "../Particle/ParticleManager.h"

using namespace RyoEngine;

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

void SceneManager::Initialize(Scene sceneState) {
    // ゲームカメラ(レール自動移動カメラ)
    camera_ = std::make_unique<RailCameraController>();
    camera_->Initialize();
    camera_->SetActive(true);

    // 軌道のウェイポイント(仮の値。旧来のSetTranslateY(153.0f)/SetTranslateZ(-25.0f)相当の
    // 開始位置を先頭に置いてある。実際のステージレイアウトに合わせて後で調整する)
    camera_->SetWayPoints({
        {   0.0f, 153.0f,   -25.0f }, // 0: スタート
        {   0.0f, 153.0f,   100.0f }, // 1: まっすぐ進む
        {  40.0f, 160.0f,   300.0f }, // 2: 大きく右へ曲がりながら少し上昇
        { -40.0f, 150.0f,   500.0f }, // 3: 今度は大きく左へカーブして下降
        {   0.0f, 153.0f,   700.0f }, // 4: 中央に戻ってくる
        {   0.0f, 180.0f,  1000.0f }, // 5: 一気に上空高くへ駆け上がる
        {   0.0f, 153.0f,  1300.0f }, // 6: ゴール・着地
        });
    // 1フレームあたりに進むワールド距離(仮の値)

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
