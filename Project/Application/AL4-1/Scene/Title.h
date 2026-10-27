#pragma once
#include <RyoEngine.h>
#include <memory>

namespace AL4_1 {
	class Title {
	public:
		Title() = default;
		// コピー禁止（ユニークポインタを持つので）
		Title(const Title&) = delete;
		Title& operator=(const Title&) = delete;
		// ムーブは許可
		Title(Title&&) noexcept = default;
		Title& operator=(Title&&) noexcept = default;

		void Initialize();
		void Update();
		void Draw();
		void Finalize();

		// シーンに入るときに呼ぶ
		void Start() { isFinished_ = false; }
		// ゲームへ進みたいか
		bool IsFinished() const { return isFinished_; }
		// キー入力を受け付けるか（フェード中は false にする）
		void SetInputEnabled(bool enabled) { isInputEnabled_ = enabled; }
		// タイトル用のカメラ（ゲームとは別の構図）
		RyoEngine::Camera& GetCamera() { return camera_; }
		// カメラの位置（背景のループの基準に使う）
		const RyoEngine::Vector3& GetCameraTranslate() const { return cameraTranslate_; }

	private:
		void RegisterInParamEditor();

	private:
		bool isFinished_ = false;
		bool isInputEnabled_ = true;

		// --- タイトル用のカメラ（背景だけを映す） ---
		RyoEngine::Camera camera_{};
		RyoEngine::Vector3 cameraTranslate_ = { 30.0f,10.0f,-35.0f };
		RyoEngine::Vector3 cameraRotate_ = { 10.0f,0.0f,0.0f };	// 度

		// --- タイトルロゴ（パリパリパリィ） ---
		std::unique_ptr<RyoEngine::Sprite> logo_ = nullptr;
		RyoEngine::Vector2 logoPosition_ = { 640.0f,260.0f };
		RyoEngine::Vector2 logoScale_ = { 0.85f,0.85f };
		// ときどき電気が走ったようにチラつく（パリパリ）
		float flickerInterval_ = 1.6f;	// チラつく間隔（毎回0.5～1.5倍でばらける）
		float flickerTime_ = 0.12f;		// 1回のチラつきの長さ
		float flickerOffset_ = 6.0f;	// ずれる幅（ピクセル）
		float flickerMinAlpha_ = 0.5f;	// チラつき中の一番薄いアルファ
		float flickerCooldown_ = 1.0f;	// 次のチラつきまでの残り時間
		float flickerTimer_ = 0.0f;		// チラつきの残り時間

		// --- PRESS SPACE（点滅） ---
		std::unique_ptr<RyoEngine::Sprite> pressSpace_ = nullptr;
		RyoEngine::Vector2 pressSpacePosition_ = { 640.0f,540.0f };
		RyoEngine::Vector2 pressSpaceScale_ = { 0.7f,0.7f };
		float blinkSpeed_ = 1.2f;		// 1秒あたりの点滅の回数
		float blinkMinAlpha_ = 0.25f;	// 一番薄いときのアルファ
		float timer_ = 0.0f;

		// --- PRESS BUTTON（ゲームパッドのとき。点滅はPRESS SPACEと同じ） ---
		std::unique_ptr<RyoEngine::Sprite> pressButton_ = nullptr;
		RyoEngine::Vector2 pressButtonPosition_ = { 640.0f,540.0f };
		RyoEngine::Vector2 pressButtonScale_ = { 0.7f,0.7f };
		// デバッグ用（_DEBUG のときだけ効く）。0:自動 1:キーボード 2:ゲームパッド 3:両方表示
		int32_t debugDevice_ = 0;

		// --- コントローラー対応（点滅させず、いつも表示する） ---
		std::unique_ptr<RyoEngine::Sprite> controllerSupport_ = nullptr;
		RyoEngine::Vector2 controllerSupportPosition_ = { 1120.0f,670.0f };
		RyoEngine::Vector2 controllerSupportScale_ = { 0.4f,0.4f };
	};
}