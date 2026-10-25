#pragma once
#include <RyoEngine.h>
#include <string>

namespace AL4_1 {
	class GameImage {
	public:
		enum class Image : uint32_t {
			BackGround,
			// Particle,  など

			// --- UI（HPバー） ---
			PlayerHpFrame,
			PlayerHpFill,
			PlayerHpLabel,
			BossHpFrame,
			BossHpFill,
			BossHpLabel,
			// --- UI（文字） ---
			TextClear,
			TextFailed,
			// --- フェード ---
			FadeBlack,

			Count
		};

		static void Initialize() {
			// 2回目以降は読み込み直さない（どこから先に呼ばれても大丈夫なように）
			if (isInitialized_) { return; }
			isInitialized_ = true;

			// --- 背景関連 ---
			// texHandles[static_cast<uint32_t>(Image::BackGround)] = RyoEngine::LoadTex("Resources/ApplicationResources/Images/BackGround.png"); など

			// --- UI ---
			const std::string uiDirectory = "Resources/ApplicationResources/AL4-1/UI/";
			Load(Image::PlayerHpFrame, uiDirectory + "player_hp_frame.png");
			Load(Image::PlayerHpFill, uiDirectory + "player_hp_fill.png");
			Load(Image::PlayerHpLabel, uiDirectory + "player_hp_label.png");
			Load(Image::BossHpFrame, uiDirectory + "boss_hp_frame.png");
			Load(Image::BossHpFill, uiDirectory + "boss_hp_fill.png");
			Load(Image::BossHpLabel, uiDirectory + "boss_hp_label.png");
			Load(Image::TextClear, uiDirectory + "text_clear.png");
			Load(Image::TextFailed, uiDirectory + "text_failed.png");

			// --- フェード（1280x720の黒） ---
			Load(Image::FadeBlack, uiDirectory + "black.png");
		}

		/// <summary>
		/// テクスチャの取得
		/// </summary>
		/// <param name="image">登録した画像名</param>
		/// <returns></returns>
		static uint32_t GetImage(Image image) {
			return texHandles[static_cast<size_t>(image)];
		}

	private:
		static void Load(Image image, const std::string& path) {
			texHandles[static_cast<size_t>(image)] = RyoEngine::LoadTex(path.c_str());
		}

	private:
		inline static uint32_t texHandles[static_cast<uint32_t>(Image::Count)] = {};
		inline static bool isInitialized_ = false;
	};
}
