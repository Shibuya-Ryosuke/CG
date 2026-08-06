#include "SceneManager.h"

using namespace RyoEngine;

void SceneManager::Initialize(SceneState sceneState) {
    // ゲームカメラ
    camera_ = std::make_unique<Camera>();
    camera_->SetActive(true);
    // デバッグカメラ
    debugCamera_ = std::make_unique<DebugCamera>();

    // シーン
    sceneState_ = sceneState;
}

void SceneManager::Finalize() {

}

void SceneManager::Update() {
    // アクティブカメラの決定とその更新
    UpdateCamera();
}

void SceneManager::Draw() {

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
