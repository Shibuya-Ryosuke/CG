#pragma once
#include <RyoEngine.h>

namespace App {
	/// <summary>
	/// アプリ本体。起動時に保存データの読み込みと最初のシーンの予約を行う。
	/// (シーンの更新・描画は SceneManager が行う)
	/// </summary>
	class Application : public RyoEngine::Framework {
	public:
		Application() : RyoEngine::Framework(L"Application") {}
		~Application() override = default;

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

	protected:
		void Initialize() override;
		// パーティクル演出 (ParticleFx) のエディタ・更新・描画 (全シーン共通)
		void Draw() override;
		void Finalize() override;
	};
}
