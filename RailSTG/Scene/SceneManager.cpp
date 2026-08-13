#include "SceneManager.h"
#include "Game.h"

using namespace RyoEngine;

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

void SceneManager::Initialize(Scene sceneState) {
    // ゲームカメラ
    camera_ = std::make_unique<Camera>();
    camera_->Initialize();
    camera_->SetActive(false);
    camera_->SetTranslateY(153.0f);
    // デバッグカメラ
    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize();
    debugCamera_->SetTranslateY(153.0f);

    // 天球
    skydome_ = Model::Create("resources/RailSTG/Skydome/skydome.obj");
    skydome_->SetLambert(ShadingMode::NONE);
    // 地面
    ground_ = Model::Create("resources/RailSTG/Ground/ground.obj");

    // シーン（enum）
    scene_ = sceneState;

    // シーン別
    game_ = std::make_unique<Game>();
    game_->Initialize();
}

void SceneManager::Finalize() {

}

void SceneManager::Update() {
    // アクティブカメラの決定とその更新
    UpdateCamera();
    skydome_->Update(*activeCamera_);
    ground_->Update(*activeCamera_);

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
    activeCamera_ = camera_->IsActive() ? camera_.get() : debugCamera_.get();
    activeCamera_->Update();

#else
    // リリース時はゲームカメラで固定
    activeCamera_ = camera_.get();
    activeCamera_->Update();
#endif

}
