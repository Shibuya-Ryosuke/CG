#pragma once
#include <RyoEngine.h>
#include <array>
#include <memory>
#include <string>

namespace AL4_1 {
	// ゲームシーンのUI（HPバー・クリア/失敗の文字・クリアタイム・メニュー）
	class GameUI {
	public:
		// メニューの項目（上から順）
		enum class MenuItem : int32_t {
			Retry,	// リトライ
			Title,	// タイトルへ

			Count
		};

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
		// HPとゲージの割合（0～1）を渡す
		void Update(float playerHpRatio, float bossHpRatio, float gaugeRatio);
		void Draw();
		void Finalize();

		// --- 表示の切り替え（Gameのフェーズに合わせて呼ぶ） ---
		// HPバーとゲージを出す・消す（イージングでフェードする。途中で切り替えても今の濃さから続く）
		void SetHpVisible(bool visible);
		// クリアの文字とクリアタイムを出す（それぞれの delay だけ遅れてフェードイン）
		void ShowClearResult(float clearTime);
		void ShowFailedText();
		// メニューを出す。isClearLayout = true でクリア時の位置、false でゲームオーバー時の位置
		void ShowMenu(bool isClearLayout);
		// 選んでいる項目（MenuItem の番号）
		void SetMenuSelection(int32_t selection) { menu_.selection = selection; }
		bool IsMenuVisible() const { return menu_.isVisible; }
		// メニューが出きって操作してよいか
		bool IsMenuReady() const { return menu_.isVisible && menu_.timer >= menu_.delay + menu_.fadeTime; }
		// 全部隠して、ゲージを満タンに戻す（登場演出・リトライ用）
		void Reset();
		// クリア・失敗の文字が出始めたフレームだけ true（delay の後、フェードインが始まった瞬間）
		bool IsClearTextAppeared() const { return clearText_.isAppearTrigger; }
		bool IsFailedTextAppeared() const { return failedText_.isAppearTrigger; }

	private:
		// HPバー1本分（frame → 遅れゲージ → fill → label の順に描く）
		struct HpBar {
			HpBar() = default;
			// コピー禁止（ユニークポインタを持つので）
			HpBar(const HpBar&) = delete;
			HpBar& operator=(const HpBar&) = delete;
			// ムーブは許可
			HpBar(HpBar&&) noexcept = default;
			HpBar& operator=(HpBar&&) noexcept = default;

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

		// パリィのゲージ（プレイヤーのHPバーの下。満タンで点滅する）
		struct Gauge {
			Gauge() = default;
			// コピー禁止（ユニークポインタを持つので）
			Gauge(const Gauge&) = delete;
			Gauge& operator=(const Gauge&) = delete;
			// ムーブは許可
			Gauge(Gauge&&) noexcept = default;
			Gauge& operator=(Gauge&&) noexcept = default;

			std::unique_ptr<RyoEngine::Sprite> frame = nullptr;
			std::unique_ptr<RyoEngine::Sprite> fill = nullptr;

			RyoEngine::Vector2 position = { 40.0f,100.0f };	// frameの左端（高さは中央）
			RyoEngine::Vector2 scale = { 1.0f,1.0f };
			RyoEngine::Vector2 fillOffset = { 4.0f,0.0f };	// frameからのずれ（全体の大きさに合わせて伸び縮みする）
			RyoEngine::Vector2 fillScale = { 1.0f,1.0f };
			float fillSpeed = 1.5f;		// 表示が実際の値に追いつく速さ（割合/秒）
			RyoEngine::Vector4 maxDimColor = { 0.5f,0.8f,1.0f,1.0f };	// 満タンの点滅で暗くなるときの色
			float maxBlinkSpeed = 3.0f;	// 満タンの点滅の速さ（1秒あたりの回数）

			float ratio = 0.0f;			// 実際の値
			float displayRatio = 0.0f;	// 表示している値
			float timer = 0.0f;
		};

		// 画面に出す文字（1枚）
		struct Text {
			Text() = default;
			// コピー禁止（ユニークポインタを持つので）
			Text(const Text&) = delete;
			Text& operator=(const Text&) = delete;
			// ムーブは許可
			Text(Text&&) noexcept = default;
			Text& operator=(Text&&) noexcept = default;

			std::string name;	// ParamEditorのグループ名
			std::unique_ptr<RyoEngine::Sprite> sprite = nullptr;
			RyoEngine::Vector2 position = { 640.0f,300.0f };
			RyoEngine::Vector2 scale = { 1.0f,1.0f };
			float delay = 0.0f;			// 出すと決まってからフェードインを始めるまでの時間（秒）
			float fadeTime = 0.5f;		// 出始めてから見えきるまでの時間（秒）

			bool isVisible = false;
			float timer = 0.0f;
			bool hasAppeared = false;		// もう出始めたか
			bool isAppearTrigger = false;	// 出始めたフレームだけ true（音を鳴らす合図）
		};

		// ゲームオーバー時に、文字の後ろ・HPなどのUIの上に敷く黒（文字を見やすくする）
		struct Dim {
			Dim() = default;
			// コピー禁止（ユニークポインタを持つので）
			Dim(const Dim&) = delete;
			Dim& operator=(const Dim&) = delete;
			// ムーブは許可
			Dim(Dim&&) noexcept = default;
			Dim& operator=(Dim&&) noexcept = default;

			std::unique_ptr<RyoEngine::Sprite> sprite = nullptr;
			RyoEngine::Vector2 position = { 640.0f,360.0f };	// 画面の中央（画像は1280x720）
			RyoEngine::Vector2 scale = { 1.0f,1.0f };
			float maxAlpha = 0.75f;		// 一番濃いときのアルファ（1.0で後ろのUIを完全に隠す）
			float fadeTime = 0.5f;

			bool isVisible = false;
			float timer = 0.0f;
		};

		// クリアタイム（「クリアタイム」の文字 + 数字を1文字ずつ）
		static constexpr size_t kMaxTimeChars = 8;	// "999.99s" が入る長さ
		struct TimeDisplay {
			TimeDisplay() = default;
			// コピー禁止（ユニークポインタを持つので）
			TimeDisplay(const TimeDisplay&) = delete;
			TimeDisplay& operator=(const TimeDisplay&) = delete;
			// ムーブは許可
			TimeDisplay(TimeDisplay&&) noexcept = default;
			TimeDisplay& operator=(TimeDisplay&&) noexcept = default;

			std::unique_ptr<RyoEngine::Sprite> label = nullptr;
			std::array<std::unique_ptr<RyoEngine::Sprite>, kMaxTimeChars> chars{};

			RyoEngine::Vector2 position = { 640.0f,360.0f };	// 文字の左端（高さは中央）
			RyoEngine::Vector2 scale = { 0.8f,0.8f };
			RyoEngine::Vector2 numberOffset = { 430.0f,0.0f };	// 文字の左端から1文字目までのずれ（scale で伸び縮み）
			RyoEngine::Vector2 numberScale = { 1.0f,1.0f };
			float charAdvance = 44.0f;	// 1文字ごとに進む幅
			float dotAdvance = 24.0f;	// 小数点だけ詰める
			float delay = 0.4f;
			float fadeTime = 0.4f;

			bool isVisible = false;
			float timer = 0.0f;
			std::string text;			// 表示する文字列（例 "12.34s"）
		};

		// メニュー（リトライ / タイトルへ）
		struct Menu {
			Menu() = default;
			// コピー禁止（ユニークポインタを持つので）
			Menu(const Menu&) = delete;
			Menu& operator=(const Menu&) = delete;
			// ムーブは許可
			Menu(Menu&&) noexcept = default;
			Menu& operator=(Menu&&) noexcept = default;

			std::array<std::unique_ptr<RyoEngine::Sprite>, static_cast<size_t>(MenuItem::Count)> items{};

			RyoEngine::Vector2 clearPosition = { 700.0f,470.0f };		// クリア時の1つ目の項目の左端（2つ目以降は itemSpacing だけ下）
			RyoEngine::Vector2 failedPosition = { 470.0f,470.0f };	// ゲームオーバー時の1つ目の項目の左端
			float itemSpacing = 80.0f;
			RyoEngine::Vector2 scale = { 0.6f,0.6f };
			float selectedScale = 1.15f;	// 選んでいる項目の大きさの倍率
			float pulseSpeed = 4.0f;		// 選んでいる項目が脈打つ速さ
			float pulseAmount = 0.04f;		// 脈打つ大きさ
			RyoEngine::Vector4 selectedColor = { 1.0f,1.0f,1.0f,1.0f };
			RyoEngine::Vector4 normalColor = { 0.55f,0.6f,0.75f,0.8f };
			float clearDelay = 0.8f;		// クリア時はクリアタイムの後に出す
			float fadeTime = 0.3f;

			bool isVisible = false;
			bool isClearLayout = true;
			float delay = 0.0f;
			float timer = 0.0f;
			int32_t selection = 0;
		};

		void RegisterHpBar(HpBar& bar);
		void RegisterText(Text& text);
		void UpdateHpFade();
		void UpdateHpBar(HpBar& bar, float ratio);
		void UpdateGauge(float ratio);
		void UpdateText(Text& text);
		void UpdateTime();
		void UpdateDim();
		void UpdateMenu();
		void DrawHpBar(HpBar& bar);
		void ResetHpBar(HpBar& bar);
		// delay 後に fadeTime かけて 0 → 1
		static float FadeAlpha(float timer, float delay, float fadeTime);

	private:
		HpBar playerHp_{};
		HpBar bossHp_{};
		Gauge gauge_{};
		Text clearText_{};
		Text failedText_{};
		Dim failedDim_{};
		TimeDisplay time_{};
		Menu menu_{};

		// HPバーを出すか
		bool isHpVisible_ = false;
		// HPバーとゲージのフェード（出すときは EaseOutCubic、消すときは EaseInCubic）
		float hpFadeInTime_ = 0.5f;
		float hpFadeOutTime_ = 0.6f;
		float hpAlpha_ = 0.0f;		// 今の濃さ
		float hpFadeFrom_ = 0.0f;	// フェードを始めたときの濃さ
		float hpFadeTimer_ = 0.0f;
	};
}