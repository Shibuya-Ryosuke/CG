#pragma once
#include <RyoEngine.h>
#include <memory>
#include <string>

namespace AL4_1 {
	// ゲームシーンのUI（HPバー・クリア/失敗の文字）
	class GameUI {
	public:
		GameUI();
		~GameUI();
		// コピー禁止（ユニークポインタを持つので）
		GameUI(const GameUI&) = delete;
		GameUI& operator=(const GameUI&) = delete;
		// ムーブは許可
		GameUI(GameUI&&) noexcept = default;
		GameUI& operator=(GameUI&&) noexcept = default;

		// ParamEditor::Load() より前に呼ぶ
		void RegisterInParamEditor();
		// GameImage::Initialize() の後に呼ぶ（スプライトを作る）
		void Initialize();
		// HPの割合（0～1）を渡す
		void Update(float playerHpRatio, float bossHpRatio);
		void Draw();
		void Finalize();

		// --- 表示の切り替え（Gameのフェーズに合わせて呼ぶ） ---
		void SetHpVisible(bool visible) { isHpVisible_ = visible; }
		void ShowClearText();
		void ShowFailedText();
		// 全部隠して、ゲージを満タンに戻す（登場演出・リトライ用）
		void Reset();

	private:
		// HPバー1本分（frame → 遅れゲージ → fill → label の順に描く）
		struct HpBar {
			std::string name;	// ParamEditorのグループ名
			std::unique_ptr<RyoEngine::Sprite> frame = nullptr;
			std::unique_ptr<RyoEngine::Sprite> ghost = nullptr;	// 遅れて減るゲージ（fillと同じ画像）
			std::unique_ptr<RyoEngine::Sprite> fill = nullptr;
			std::unique_ptr<RyoEngine::Sprite> label = nullptr;

			// 位置（frameの左端・高さは中央）と全体の大きさ
			RyoEngine::Vector2 position = { 0.0f,0.0f };
			RyoEngine::Vector2 scale = { 1.0f,1.0f };
			// frameからのずれ（全体の大きさに合わせて伸び縮みする）と、それぞれの大きさ
			RyoEngine::Vector2 fillOffset = { 0.0f,0.0f };
			RyoEngine::Vector2 fillScale = { 1.0f,1.0f };
			RyoEngine::Vector2 labelOffset = { 0.0f,-28.0f };
			RyoEngine::Vector2 labelScale = { 1.0f,1.0f };

			// 遅れて減るゲージ
			RyoEngine::Vector4 ghostColor = { 1.0f,1.0f,1.0f,0.45f };
			float ghostDelay = 0.4f;	// 減ってから追いかけ始めるまでの時間（秒）
			float ghostSpeed = 0.5f;	// 追いかける速さ（割合/秒）

			// 今の値
			float ratio = 1.0f;
			float ghostRatio = 1.0f;
			float ghostTimer = 0.0f;
		};

		// 画面中央に出す文字
		struct Text {
			std::string name;	// ParamEditorのグループ名
			std::unique_ptr<RyoEngine::Sprite> sprite = nullptr;
			RyoEngine::Vector2 position = { 640.0f,300.0f };
			RyoEngine::Vector2 scale = { 1.0f,1.0f };
			float fadeTime = 0.5f;		// 出始めてから見えきるまでの時間（秒）

			bool isVisible = false;
			float timer = 0.0f;
		};

		void RegisterHpBar(HpBar& bar);
		void RegisterText(Text& text);
		void UpdateHpBar(HpBar& bar, float ratio);
		void UpdateText(Text& text);
		void DrawHpBar(HpBar& bar);
		void ResetHpBar(HpBar& bar);

	private:
		HpBar playerHp_{};
		HpBar bossHp_{};
		Text clearText_{};
		Text failedText_{};

		// HPバーを出すか
		bool isHpVisible_ = false;
	};
}
