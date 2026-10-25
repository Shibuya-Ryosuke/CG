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
			// --- タイトル ---
			TitleLogo,
			PressSpace,
			// --- クリア・ゲームオーバー ---
			TextClearTime,
			MenuRetry,
			MenuTitle,
			// --- 数字（Num0～Num9 の並びは変えないこと。Num0 + 数字 で取り出す） ---
			Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
			NumDot,
			NumS,
			// --- ゲージ ---
			GaugeFrame,
			GaugeFill,

			Count
		};

		static void Initialize() {
			// 2回目以降は読み込み直さない（どこから先に呼ばれても大丈夫なように）
			if (isInitialized_) { return; }
			isInitialized_ = true;

			// --- 背景関連 ---
			// texHandles[static_cast<uint32_t>(Image::BackGround)] = RyoEngine::LoadTex("Resources/ApplicationResources/Images/BackGround.png"); など

			// --- UI（フォルダごとに分けて置いてある） ---
			const std::string uiDirectory = "AL4-1/UI/";
			const std::string playerDirectory = uiDirectory + "Player/";
			const std::string bossDirectory = uiDirectory + "Boss/";
			const std::string textDirectory = uiDirectory + "Text/";
			const std::string numberDirectory = uiDirectory + "Number/";

			// --- HPバー ---
			Load(Image::PlayerHpFrame, playerDirectory + "player_hp_frame.png");
			Load(Image::PlayerHpFill, playerDirectory + "player_hp_fill.png");
			Load(Image::PlayerHpLabel, playerDirectory + "player_hp_label.png");
			Load(Image::BossHpFrame, bossDirectory + "boss_hp_frame.png");
			Load(Image::BossHpFill, bossDirectory + "boss_hp_fill.png");
			Load(Image::BossHpLabel, bossDirectory + "boss_hp_label.png");

			// --- ゲージ ---
			Load(Image::GaugeFrame, playerDirectory + "gauge_frame.png");
			Load(Image::GaugeFill, playerDirectory + "gauge_fill.png");

			// --- 文字 ---
			Load(Image::TextClear, textDirectory + "text_clear.png");
			Load(Image::TextFailed, textDirectory + "text_failed.png");
			Load(Image::TextClearTime, textDirectory + "text_clear_time.png");
			Load(Image::MenuRetry, textDirectory + "menu_retry.png");
			Load(Image::MenuTitle, textDirectory + "menu_title.png");
			Load(Image::TitleLogo, textDirectory + "title_logo.png");
			Load(Image::PressSpace, textDirectory + "press_space.png");

			// --- 数字（全部同じ大きさの画像） ---
			for (uint32_t i = 0; i < 10; ++i) {
				Load(static_cast<Image>(static_cast<uint32_t>(Image::Num0) + i), numberDirectory + "num_" + std::to_string(i) + ".png");
			}
			Load(Image::NumDot, numberDirectory + "num_dot.png");
			Load(Image::NumS, numberDirectory + "num_s.png");

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
