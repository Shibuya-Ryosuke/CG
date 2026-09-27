#pragma once
#include "../../Original/RyoEngine.h"

class GameImage {
public:
	enum class Image : uint32_t {
		BackGround,
		// Particle,  など

		Count
	};

	static void Initialize() {
		// --- 背景関連 ---
		// texHandles[static_cast<uint32_t>(Image::BackGround)] = RyoEngine::LoadTex("Resources/EngineResources/Images/BackGround.png"); など
	}

private:
	inline static uint32_t texHandles[static_cast<uint32_t>(Image::Count)] = {};
};