#include "MapChipField.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace RyoEngine;

namespace AL4_1 {
	namespace {
		// 床に接しているだけ・壁に接しているだけで隣のマスと重なった扱いにならないための余白
		constexpr float kEps = 0.001f;

		// block.obj の1辺の長さ（-1〜+1 の立方体なので 2.0）。tileSize_ に合わせて拡縮する
		constexpr float kBlockModelSize = 2.0f;
	}

	int MapChipField::ToIndex(float v) const {
		// マスの中心が整数座標なので +0.5 して floor
		return static_cast<int>(std::floor(v / tileSize_ + 0.5f));
	}

	bool MapChipField::IsSolid(int x, int y) const {
		if (x < 0 || x >= width_) return true;    // 左右の外は壁
		if (y < 0 || y >= height_) return false;  // 下は穴、上は空
		return grid_[y * width_ + x] == MapChipType::Block;
	}

	void MapChipField::Load(const std::string& csvPath, const std::string& blockModelPath) {
		// --- CSVを数値の表にする ---
		std::ifstream ifs(csvPath);
		assert(ifs.is_open() && "マップCSVが開けません。パスを確認してください");

		std::vector<std::vector<int>> rows;
		std::string line;
		while (std::getline(ifs, line)) {
			if (!line.empty() && line.back() == '\r') line.pop_back();
			if (line.empty()) continue;

			std::vector<int> row;
			std::stringstream ss(line);
			std::string cell;
			while (std::getline(ss, cell, ',')) {
				row.push_back(std::atoi(cell.c_str()));
			}
			rows.push_back(std::move(row));
		}

		height_ = static_cast<int>(rows.size());
		width_ = 0;
		for (const auto& row : rows) width_ = std::max(width_, static_cast<int>(row.size()));

		grid_.assign(static_cast<size_t>(width_) * height_, MapChipType::None);
		spawns_.clear();

		// --- 配置データを作る（CSVの先頭行が一番上なので、yを反転する） ---
		std::vector<Vector3> blockPositions;
		for (int r = 0; r < height_; ++r) {
			for (int x = 0; x < static_cast<int>(rows[r].size()); ++x) {
				int value = rows[r][x];
				int y = height_ - 1 - r;

				if (value == static_cast<int>(MapChipType::Block)) {
					grid_[y * width_ + x] = MapChipType::Block;
					blockPositions.push_back({ x * tileSize_, y * tileSize_, 0.0f });
				} else if (value >= 2) {
					// 足元＝そのマスの下辺中央
					MapSpawn spawn;
					spawn.id = value;
					spawn.position = { x * tileSize_, y * tileSize_ - tileSize_ * 0.5f, 0.0f };
					spawns_.push_back(spawn);
				}
			}
		}

		// ブロックをインスタンスとして登録
		blockModel_.Initialize(blockModelPath, static_cast<uint32_t>(std::max<size_t>(blockPositions.size(), 1)));
		const float blockScale = tileSize_ / kBlockModelSize;
		for (const auto& pos : blockPositions) {
			auto handle = blockModel_.AddInstance(pos);

			Transform t{ .scale = { blockScale, blockScale, blockScale } };
			t.translate = pos;
			blockModel_.SetInstanceTransform(handle, t);
		}
		blockModel_.UpdateBuffer();
	}

	// ----------------------------------------------------------------
	// 判定付き移動
	// ----------------------------------------------------------------
	Vector3 MapChipField::MoveAndCollide(Vector3 feet, const Vector3& delta, const Vector3& halfSize, MapHit& hit) const {
		hit = MapHit{};

		// 速くてもブロックをすり抜けないよう、1回の移動が半マス以内になるよう分割する
		float maxMove = std::max(std::fabs(delta.x), std::fabs(delta.y));
		int steps = std::max(1, static_cast<int>(std::ceil(maxMove / (tileSize_ * 0.5f))));
		float stepX = delta.x / steps;
		float stepY = delta.y / steps;

		// 足元 → 体の中心 で計算する
		Vector3 center = { feet.x, feet.y + halfSize.y, feet.z };

		for (int i = 0; i < steps; ++i) {
			// X軸とY軸は別々に動かして別々に押し戻す（角で引っかからないため）
			MoveAxisX(center, stepX, halfSize, hit);
			MoveAxisY(center, stepY, halfSize, hit);
		}

		return { center.x, center.y - halfSize.y, center.z };
	}

	void MapChipField::MoveAxisX(Vector3& c, float dx, const Vector3& half, MapHit& hit) const {
		if (dx == 0.0f) return;
		c.x += dx;

		// 進行方向側の「先頭の辺」がいるマス列だけを調べる
		float edge = c.x + (dx > 0.0f ? half.x : -half.x);
		int ix = ToIndex(edge);
		int minY = ToIndex(c.y - half.y + kEps);
		int maxY = ToIndex(c.y + half.y - kEps);

		for (int iy = minY; iy <= maxY; ++iy) {
			if (!IsSolid(ix, iy)) continue;

			if (dx > 0.0f) {
				c.x = ix * tileSize_ - tileSize_ * 0.5f - half.x;  // ブロックの左辺にぴったり
				hit.right = true;
			} else {
				c.x = ix * tileSize_ + tileSize_ * 0.5f + half.x;  // ブロックの右辺にぴったり
				hit.left = true;
			}
			break;
		}
	}

	void MapChipField::MoveAxisY(Vector3& c, float dy, const Vector3& half, MapHit& hit) const {
		if (dy == 0.0f) return;
		c.y += dy;

		float edge = c.y + (dy > 0.0f ? half.y : -half.y);
		int iy = ToIndex(edge);
		int minX = ToIndex(c.x - half.x + kEps);
		int maxX = ToIndex(c.x + half.x - kEps);

		for (int ix = minX; ix <= maxX; ++ix) {
			if (!IsSolid(ix, iy)) continue;

			if (dy > 0.0f) {
				c.y = iy * tileSize_ - tileSize_ * 0.5f - half.y;  // 天井の下にぴったり
				hit.top = true;
			} else {
				c.y = iy * tileSize_ + tileSize_ * 0.5f + half.y;  // 床の上にぴったり
				hit.bottom = true;
			}
			break;
		}
	}
}