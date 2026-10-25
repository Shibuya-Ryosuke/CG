#pragma once
#include <RyoEngine.h>
#include <memory>

namespace AL4_1 {

	/// <summary>
	/// 画面全体を黒い画像で覆うフェードだけを担当する
	/// </summary>
	class Fade {
	public:
		// フェードの状態
		enum class State {
			None,     // 何もしていない(透明)
			FadeOut,  // 透明 → 真っ黒
			Black,    // 真っ黒で止まっている(StartFadeIn待ち)
			FadeIn,   // 真っ黒 → 透明
		};

		Fade() = default;
		~Fade() = default;

		Fade(const Fade&) = delete;
		Fade& operator=(const Fade&) = delete;

		/// <summary>
		/// 初期化(画面を覆うスプライトを作る)
		/// </summary>
		void Initialize();

		/// <summary>
		/// 更新(アルファを進める。時間は GetDeltaTime なのでスローの影響を受けない)
		/// </summary>
		void Update();

		/// <summary>
		/// 描画(他のスプライトより後に呼ぶこと)
		/// </summary>
		void Draw();

		/// <summary>
		/// 終了処理
		/// </summary>
		void Finalize();

		/// <summary>
		/// フェードアウト開始(透明 → 真っ黒)。終わったら真っ黒のまま止まる
		/// </summary>
		/// <param name="duration">かける秒数</param>
		void StartFadeOut(float duration);

		/// <summary>
		/// フェードイン開始(真っ黒 → 透明)
		/// </summary>
		/// <param name="duration">かける秒数</param>
		void StartFadeIn(float duration);

		/// <summary>
		/// フェード中か(フェードアウト中・フェードイン中)
		/// </summary>
		bool IsFading() const { return state_ == State::FadeOut || state_ == State::FadeIn; }

		/// <summary>
		/// 真っ黒になりきったか(シーン切り替えのタイミング用)
		/// </summary>
		bool IsFadeOutFinished() const { return state_ == State::Black; }

		/// <summary>
		/// フェードが終わっているか(何もしていない・真っ黒で止まっている)
		/// </summary>
		bool IsFinished() const { return !IsFading(); }

		/// <summary>
		/// 状態の取得
		/// </summary>
		State GetState() const { return state_; }

		/// <summary>
		/// 今の不透明度(0 = 透明 / 1 = 真っ黒)
		/// </summary>
		float GetAlpha() const { return alpha_; }

	private:
		/// <summary>
		/// 不透明度をスプライトへ反映
		/// </summary>
		void ApplyAlpha();

		/// <summary>
		/// 0～1 をなめらかに補間する(端でゆっくり)
		/// </summary>
		static float SmoothStep01(float t);

	private:
		// 画面サイズ(1280x720想定)
		RyoEngine::Vector2 screenSize_{ 1280.0f, 720.0f };

		// 画面を覆うスプライト
		std::unique_ptr<RyoEngine::Sprite> sprite_;

		State state_ = State::None;
		// 今の不透明度
		float alpha_ = 0.0f;
		// フェードの進み具合(0～1)
		float t_ = 0.0f;
		// フェードにかける秒数
		float duration_ = 0.5f;
	};

}
