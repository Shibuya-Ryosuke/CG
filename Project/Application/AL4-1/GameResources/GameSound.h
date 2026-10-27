#pragma once
#include <RyoEngine.h>

namespace AL4_1 {
	class GameSound {
	public:
		// BGMの種類
		enum class BGM : uint32_t {
			Title,
			Game,
			Clear,

			Count
		};

		// SEの種類
		enum class SE : uint32_t {
			Cursor,
			Decide,
			Clear,
			Failed,
			Player_Attack,
			Player_GaugeMax,
			Player_Parry,
			Player_FullParry,
			Player_Jump,
			Player_Land,
			Player_TakeDamage,
			Boss_Shot,
			Boss_MeleeCharge,
			Boss_MeleeBurst,
			Boss_Stun,
			Boss_TakeDamage,

			Count
		};

		/// <summary>
		/// ゲーム起動時やシーン移行時に1度だけ呼び出して全音声を一括ロードする
		/// </summary>
		static void Initialize() {
			// 共通パス
			const std::string commonPath = "AL4-1/Sound/";
			const std::string bgmPath = commonPath + "BGM/";
			const std::string sePath = commonPath + "SE/";

			// BGM
			bgmHandles[static_cast<uint32_t>(BGM::Title)] = RyoEngine::LoadBGM(bgmPath + "title.mp3");
			bgmHandles[static_cast<uint32_t>(BGM::Game)] = RyoEngine::LoadBGM(bgmPath + "game.mp3");
			bgmHandles[static_cast<uint32_t>(BGM::Clear)] = RyoEngine::LoadBGM(bgmPath + "clear.mp3");

			// SE
			const std::string playerSePath = sePath + "Player/";
			const std::string bossSePath = sePath + "Boss/";
			// --- UI・進行 ---
			seHandles[static_cast<uint32_t>(SE::Cursor)] = RyoEngine::LoadSE(sePath + "cursor.wav");
			seHandles[static_cast<uint32_t>(SE::Decide)] = RyoEngine::LoadSE(sePath + "decide.wav");
			seHandles[static_cast<uint32_t>(SE::Clear)] = RyoEngine::LoadSE(sePath + "clear.wav");
			seHandles[static_cast<uint32_t>(SE::Failed)] = RyoEngine::LoadSE(sePath + "failed.wav");
			// --- プレイヤー ---
			seHandles[static_cast<uint32_t>(SE::Player_Attack)] = RyoEngine::LoadSE(playerSePath + "player_attack.wav");
			seHandles[static_cast<uint32_t>(SE::Player_GaugeMax)] = RyoEngine::LoadSE(playerSePath + "player_gaugeMax.wav");
			seHandles[static_cast<uint32_t>(SE::Player_Parry)] = RyoEngine::LoadSE(playerSePath + "player_parry.wav");
			seHandles[static_cast<uint32_t>(SE::Player_FullParry)] = RyoEngine::LoadSE(playerSePath + "player_fullParry.wav");
			seHandles[static_cast<uint32_t>(SE::Player_Jump)] = RyoEngine::LoadSE(playerSePath + "player_jump.wav");
			seHandles[static_cast<uint32_t>(SE::Player_Land)] = RyoEngine::LoadSE(playerSePath + "player_land.wav");
			seHandles[static_cast<uint32_t>(SE::Player_TakeDamage)] = RyoEngine::LoadSE(playerSePath + "player_takeDamage.wav");
			// --- ボス ---
			seHandles[static_cast<uint32_t>(SE::Boss_Shot)] = RyoEngine::LoadSE(bossSePath + "boss_shot2.wav");
			seHandles[static_cast<uint32_t>(SE::Boss_MeleeCharge)] =RyoEngine:: LoadSE(bossSePath + "boss_meleeCharge.wav");
			seHandles[static_cast<uint32_t>(SE::Boss_MeleeBurst)] = RyoEngine::LoadSE(bossSePath + "boss_meleeBurst.wav");
			seHandles[static_cast<uint32_t>(SE::Boss_Stun)] = RyoEngine::LoadSE(bossSePath + "boss_stun.wav");
			seHandles[static_cast<uint32_t>(SE::Boss_TakeDamage)] = RyoEngine::LoadSE(bossSePath + "boss_takeDamage.wav");
		}

		/// <summary>
		/// BGMを再生する
		/// </summary>
		static void PlayBGM(BGM bgm, float volume = 0.1f, bool loop = true) {
			uint32_t handle = bgmHandles[static_cast<uint32_t>(bgm)];
			RyoEngine::Audio::PlayBGM(handle, volume, loop);
		}

		/// <summary>
		/// BGMを停止する（再再生時は初めからになる）
		/// </summary>
		static void StopBGM(BGM bgm) {
			uint32_t handle = bgmHandles[static_cast<uint32_t>(bgm)];
			RyoEngine::Audio::StopBGM(handle);
		}

		/// <summary>
		/// BGMの音量を変更する
		/// </summary>
		static void SetBGMVolume(BGM bgm, float volume) {
			uint32_t handle = bgmHandles[static_cast<uint32_t>(bgm)];
			RyoEngine::Audio::SetBGMVolume(handle, volume);
		}

		/// <summary>
		/// SEを再生する
		/// </summary>
		static void PlaySE(SE se, float volume = 0.15f) {
			uint32_t handle = seHandles[static_cast<uint32_t>(se)];
			RyoEngine::Audio::PlaySE(handle, volume);
		}

	private:

		inline static uint32_t bgmHandles[static_cast<uint32_t>(BGM::Count)] = {};
		inline static uint32_t seHandles[static_cast<uint32_t>(SE::Count)] = {};
	};
}