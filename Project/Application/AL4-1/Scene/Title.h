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

	private:
		void RegisterInParamEditor();

	private:
		bool isFinished_ = false;
		bool isInputEnabled_ = true;

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
	};
}
