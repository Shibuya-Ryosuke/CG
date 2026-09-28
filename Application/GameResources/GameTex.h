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
		// texHandles[static_cast<uint32_t>(Image::BackGround)] = RyoEngine::LoadTex("Resources/ApplicationResources/Images/BackGround.png"); など
	}

	/// <summary>
	/// テクスチャの取得
	/// </summary>
	/// <param name="image">登録した画像名</param>
	/// <returns></returns>
	uint32_t GetTex(Image image) {
		return texHandles[static_cast<size_t>(image)];
	}

private:
	inline static uint32_t texHandles[static_cast<uint32_t>(Image::Count)] = {};
};