#pragma once
#include "../../Original/RyoEngine.h"

class GameSound {
public:
	// BGMの種類
	enum class BGM : uint32_t {
		Title,
		Game,
		Count
	};

	// SEの種類
	enum class SE : uint32_t {
		// System
		// Cancel,
		// Decision, など

		Count
	};

	/// <summary>
	/// ゲーム起動時やシーン移行時に1度だけ呼び出して全音声を一括ロードする
	/// </summary>
	static void Initialize() {
		// --- BGMのロード ---
		// bgmHandles[static_cast<uint32_t>(BGM::Title)] = RyoEngine::Audio::LoadBGM("resources/Sound/BGM/title.mp3");
		// bgmHandles[static_cast<uint32_t>(BGM::Game)] = RyoEngine::Audio::LoadBGM("resources/Sound/BGM/game.mp3"); など
		

		// --- SEのロード (System) ---
		// seHandles[static_cast<uint32_t>(SE::Cancel)] = RyoEngine::Audio::LoadSE("resources/Sound/SE/System/cancel.mp3");
		// seHandles[static_cast<uint32_t>(SE::Decision)] = RyoEngine::Audio::LoadSE("resources/Sound/SE/System/decision.mp3"); など
		
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
	/// BGMの音量を変更する（1.0が等倍）
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