#include "AL4_1.h"
#include "GameResources/GameTex.h"
#include "GameResources/GameSound.h"

using namespace RyoEngine;

namespace AL4_1 {
	void AL4_1::Initialize() {
		// リソース初期化
		GameImage::Initialize();
		GameSound::Initialize();

		// フォルダパスの指定
		ParamEditor::SetFolderPath("Resources/ApplicationResources/AL4-1/Json/");
		LightManager::SetFolderPath("Resources/ApplicationResources/AL4-1/Json/");
		GPUParticleManager::GetInstance()->SetFolderPath("Resources/ApplicationResources/AL4-1/Json/");

		// ゲーム初期化
		sceneManager_ = std::make_unique<SceneManager>();
		sceneManager_->Initialize(Scene::Title);

		// bloomは常時有効にする
		PostProcess::GetInstance()->SetBloomEnabled(true);

		// ロード
		ParamEditor::Load();
		LightManager::Load();
		GPUParticleManager::GetInstance()->Load();

		// デバッグカメラ初期無効化
		GetDebugCamera().SetAvailable(false);
	}
	void AL4_1::Update() {
#ifdef _DEBUG
		// デバッグ時一時停止のために使用
		if (Input::TriggerKey(DIK_Q)) {
			stop = !stop;
			if (!stop) {
				SetTimeScale(1.0f);
			}
		}
#endif
		if (stop) { SetTimeScale(0.0f); }
		sceneManager_->Update();
		if (stop) { SetTimeScale(0.0f); }
		
		PrimitiveRenderer::SetCamera(GetActiveCamera(sceneManager_->GetCamera()));
	}
	void AL4_1::Draw() {
		sceneManager_->Draw(GetActiveCamera(sceneManager_->GetCamera()));
	}
	void AL4_1::Finalize() {
		sceneManager_->Finalize();
	}
}