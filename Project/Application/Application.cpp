#include "Application.h"
#include "Scene/SceneSetup.h"
#include "Effect/ParticleFx/ParticleFxManager.h"

using namespace RyoEngine;

namespace App {
	void Application::Initialize() {
		// ImGui で調整した値の保存先を設定して、読み込む
		const std::string jsonFolder = "Resources/ApplicationResources/Json/";
		ParamEditor::SetFolderPath(jsonFolder);
		LightManager::SetFolderPath(jsonFolder);
		GPUParticleManager::SetFolderPath(jsonFolder);
		ParamEditor::Load();
		LightManager::Load();
		GPUParticleManager::Load();

		// パーティクル演出 (ParticleFx)。JSONが無ければテンプレから見本を作る
		ParticleFxManager::SetFolderPath(jsonFolder);
		ParticleFxManager::Initialize();

		// シーンの登録と、起動時のシーンの指定 (Scene/SceneSetup.cpp)
		SetupScenes();
	}

	void Application::Draw() {
		// エディタ → 発生・シミュレーションの予約 → 描画の予約 の順
		// (エディタでの作り直しが、予約済みのコマンドを壊さないように先に行う)
		ParticleFxManager::DrawImGui();
		ParticleFxManager::Update();
		ParticleFxManager::Draw();
	}

	void Application::Finalize() {
		ParticleFxManager::Finalize();
	}
}
