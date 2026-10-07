#pragma once
#include <RyoEngine.h>
#include <cstdint>
#include <string>
#include <vector>

namespace AL4_1 {
	// CSVの値   0:空  1:ブロック  2以上:スポーン指定(中身は空として扱う)
	enum class MapChipType : uint8_t {
		None = 0,
		Block = 1,
	};

	// 移動中にどの方向でブロックに当たったか
	struct MapHit {
		bool left = false;
		bool right = false;
		bool top = false;     // 頭をぶつけた
		bool bottom = false;  // 着地した（接地）
	};

	// CSVに置かれたスポーン指定(2以上の値)
	struct MapSpawn {
		int id = 0;
		RyoEngine::Vector3 position{};  // そのマスの「下辺中央」＝足元の位置
	};

	/// <summary>
	/// CSVから読み込んだマップチップの配置・描画・当たり判定を担当する
	/// 
	/// ・CSVの1行目が一番上。読み込み時に上下を反転し、Y軸は上方向がプラス
	/// ・マス(x,y)の中心ワールド座標は (x * tileSize, y * tileSize, 0)
	/// ・1マスの大きさ(tileSize)は SetTileSize で変更できる（Loadより前に呼ぶ）
	/// ・ブロック用モデルは「-1〜+1 の立方体で原点が中心」のobjを想定。tileSizeに合わせて自動で拡縮する
	/// </summary>
	class MapChipField {
	public:
		/// <summary>
		/// 1マスの大きさを設定する。Loadより前に呼ぶこと。
		/// モデルが2x2x2(-1〜+1)なら 2.0（デフォルト）で、モデルの見た目と判定がぴったり合う
		/// </summary>
		void SetTileSize(float size) { tileSize_ = size; }
		float GetTileSize() const { return tileSize_; }

		/// <summary>CSVとブロックモデルを読み込み、ブロックを配置する</summary>
		void Load(const std::string& csvPath, const std::string& blockModelPath);

		void Draw(RyoEngine::Camera& camera) { blockModel_.Draw(camera); }

		/// <summary>
		/// 判定付き移動。足元中心(feet)から delta だけ動かし、ブロックにめり込まない位置を返す
		/// </summary>
		/// <param name="feet">体の足元中心の位置</param>
		/// <param name="delta">今フレームの移動量</param>
		/// <param name="halfSize">体のAABBの半分のサイズ（x,yを使用）</param>
		/// <param name="hit">どの面で当たったか</param>
		RyoEngine::Vector3 MoveAndCollide(RyoEngine::Vector3 feet, const RyoEngine::Vector3& delta,
			const RyoEngine::Vector3& halfSize, MapHit& hit) const;

		/// <summary>ワールド座標がブロックの中かどうか（崖チェックなどに使う）</summary>
		bool IsSolidAt(const RyoEngine::Vector3& worldPos) const {
			return IsSolid(ToIndex(worldPos.x), ToIndex(worldPos.y));
		}

		const std::vector<MapSpawn>& GetSpawns() const { return spawns_; }
		int GetWidth() const { return width_; }
		int GetHeight() const { return height_; }

	private:
		// マス番号 → 判定。範囲外：左右は壁、下は穴、上は空
		bool IsSolid(int x, int y) const;
		int ToIndex(float v) const;

		void MoveAxisX(RyoEngine::Vector3& center, float dx, const RyoEngine::Vector3& half, MapHit& hit) const;
		void MoveAxisY(RyoEngine::Vector3& center, float dy, const RyoEngine::Vector3& half, MapHit& hit) const;

	private:
		// 1マスの大きさ。ブロックのモデル(2x2x2)に合わせて2.0
		float tileSize_ = 2.0f;

		int width_ = 0;
		int height_ = 0;
		std::vector<MapChipType> grid_{};  // [y * width_ + x]、yは上がプラス
		std::vector<MapSpawn> spawns_{};

		// 全ブロックを1回のDrawで描くためのインスタンスモデル
		RyoEngine::InstancedModel blockModel_{};
	};
}