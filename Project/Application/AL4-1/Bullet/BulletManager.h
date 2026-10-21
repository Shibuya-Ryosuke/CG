#pragma once
#include <RyoEngine.h>
#include <string>
#include <vector>
#include "Bullet.h"
#include "../MapChipField/MapChipField.h"

namespace AL4_1 {
	/// <summary>
	/// ボスの弾をプールで管理する
	/// ・Initialize で最大数ぶんのインスタンスを先に確保し、使い終わった弾は非表示にして使い回す
	/// </summary>
	class BulletManager {
	public:
		void Initialize(const std::string& modelPath, uint32_t maxCount);
		// 弾が壁に当たったら消すためのマップ
		void SetMap(const MapChipField* map) { map_ = map; }

		/// <summary>
		/// 空いてる弾を発射
		/// </summary>
		/// <param name="position"></param>
		/// <param name="velocity"></param>
		/// <returns></returns>
		Bullet* Fire(const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity);

		void Update();
		void Draw(RyoEngine::Camera& camera);
		// 全部の弾を消す（リトライ時など）
		void Clear();

		std::vector<Bullet>& GetBullets() { return bullets_; }

	private:
		RyoEngine::InstancedModel model_{};
		std::vector<Bullet> bullets_{};
		const MapChipField* map_ = nullptr;
	};
}
