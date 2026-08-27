#pragma once
#include "../../Original/RyoEngine.h"
#include <cstdint>

class GameSound {
public:
	// BGMの種類
	enum class BGM {
		Title,
		Game,
		Clear,
		Failed,
		Max
	};

	// SEの種類（ファイル数が増えてもここに追加していく、あるいはカテゴリごとに分ける）
	enum class SE {
		// System
		Cancel,
		Decision,
		Pause,
		PushWS,
		// Player
		PlayerEvasion,
		PlayerCollectJustEvasion,
		Hit,
		LockOnMode,
		LockOn,
		HomingMissile,
		MainShot,
		PlayerDestroy,
		// Enemy
		EnemySpawn,
		EnemyDestroy,
		EnemyShot,
		ReticleLock,
		ReticleReady,
		ReticleShot,
		Max
	};

	/// <summary>
	/// ゲーム起動時やシーン移行時に1度だけ呼び出して全音声を一括ロードする
	/// </summary>
	static void Initialize() {
		// --- BGMのロード ---
		bgmHandles[static_cast<int>(BGM::Title)] = RyoEngine::Audio::LoadBGM("resources/RailSTG/Sound/BGM/title.mp3");
		bgmHandles[static_cast<int>(BGM::Game)] = RyoEngine::Audio::LoadBGM("resources/RailSTG/Sound/BGM/game.mp3");
		bgmHandles[static_cast<int>(BGM::Clear)] = RyoEngine::Audio::LoadBGM("resources/RailSTG/Sound/BGM/clear.mp3");
		bgmHandles[static_cast<int>(BGM::Failed)] = RyoEngine::Audio::LoadBGM("resources/RailSTG/Sound/BGM/failed.mp3");

		// --- SEのロード (System) ---
		seHandles[static_cast<int>(SE::Cancel)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/System/cancel.mp3");
		seHandles[static_cast<int>(SE::Decision)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/System/decision.mp3");
		seHandles[static_cast<int>(SE::Pause)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/System/pause.mp3");
		seHandles[static_cast<int>(SE::PushWS)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/System/pushWS.mp3");

		// --- SEのロード (Player) ---
		seHandles[static_cast<int>(SE::PlayerEvasion)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Player/evasion.mp3");
		seHandles[static_cast<int>(SE::PlayerCollectJustEvasion)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Player/collectJustEvasion.mp3");
		seHandles[static_cast<int>(SE::Hit)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Player/hit.mp3");
		seHandles[static_cast<int>(SE::HomingMissile)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Player/homingMissile.mp3");
		seHandles[static_cast<int>(SE::LockOnMode)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Player/lockOnMode.mp3");
		seHandles[static_cast<int>(SE::LockOn)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Player/lockOn.mp3");
		seHandles[static_cast<int>(SE::MainShot)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Player/mainShot.mp3");
		seHandles[static_cast<int>(SE::PlayerDestroy)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Player/playerDestory.mp3");

		// --- SEのロード (Enemy) ---
		seHandles[static_cast<int>(SE::EnemySpawn)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Enemy/enemySpawn.mp3");
		seHandles[static_cast<int>(SE::EnemyDestroy)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Enemy/enemyDestroy.mp3");
		seHandles[static_cast<int>(SE::EnemyShot)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Enemy/enemyShot.mp3");
		seHandles[static_cast<int>(SE::ReticleLock)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Enemy/reticleLock.mp3");
		seHandles[static_cast<int>(SE::ReticleReady)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Enemy/reticleReady.mp3");
		seHandles[static_cast<int>(SE::ReticleShot)] = RyoEngine::Audio::LoadSE("resources/RailSTG/Sound/SE/Enemy/reticleShot.mp3");
	}

	/// <summary>
	/// BGMを再生する
	/// </summary>
	static void PlayBGM(BGM bgm, float volume = 0.1f, bool loop = true) {
		uint32_t handle = bgmHandles[static_cast<int>(bgm)];
		RyoEngine::Audio::PlayBGM(handle, volume, loop);
	}

	/// <summary>
	/// BGMの音量を変更する
	/// </summary>
	static void SetBGMVolume(BGM bgm, float volume) {
		uint32_t handle = bgmHandles[static_cast<int>(bgm)];
		RyoEngine::Audio::SetBGMVolume(handle, volume);
	}

	/// <summary>
	/// SEを再生する（どこからでもこの関数を呼ぶだけで鳴らせます）
	/// </summary>
	static void PlaySE(SE se, float volume = 0.15f) {
		uint32_t handle = seHandles[static_cast<int>(se)];
		RyoEngine::Audio::PlaySE(handle, volume);
	}

	static void StopBGM(BGM bgm) {
		uint32_t handle = bgmHandles[static_cast<int>(bgm)];
		RyoEngine::Audio::StopBGM(handle);
	}

private:
	// inline static を使うことで、ソースファイル(.cpp)を用意せず
	// ヘッダー単体でどこからでも安全にデータを共有・保持できます
	inline static uint32_t bgmHandles[static_cast<int>(BGM::Max)] = {};
	inline static uint32_t seHandles[static_cast<int>(SE::Max)] = {};
};