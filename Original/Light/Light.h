#pragma once
#include <cstdint>
#include "../Math/Vector.h"

namespace RyoEngine {

	// ライトの種類
	enum class LightType : uint32_t {
		Directional = 0,
		Point = 1,
		Spot = 2,
		Area = 3,
	};

	// 汎用ライト構造体（GPU(HLSL)側のstructと1:1でレイアウトを一致させること）
	// 全部で64byte。StructuredBufferの要素として使うので16byteアライン(cbuffer)ほど厳密である必要はないが、
	// 見通しを良くするため16byte区切りで揃えている。
	struct Light {
		Vector4   color;       // rgb=色, a=未使用                         (16byte)
		Vector3   direction;   // Directional/Spotで使用（正規化して格納）
		float     intensity;   // 共通：明るさ                             (16byte)
		Vector3   position;    // Point/Spot/Areaで使用
		float     range;       // Point/Spotの減衰距離                     (16byte)
		float     spotAngle;   // Spotの照射角（cos値で持つと軽い）
		float     spotFalloff; // Spotの縁の滑らかさ
		LightType type;        // ライト種別
		float     padding;     // 16byteアライン調整用                     (16byte)
	};
	static_assert(sizeof(Light) == 64, "Light構造体のサイズがHLSL側と食い違うと壊れるので確認すること");

	// GPUへ渡すライト数（毎フレーム変わるため、Light配列本体とは別の定数バッファで持つ）
	struct LightCountData {
		uint32_t lightCount;
		float    padding[3]; // 16byteアライン
	};

	// アンビエントライト（方向・位置を持たない、シーン全体への底上げ光。マテリアルではなくシーン側の設定値）
	struct AmbientLight {
		Vector4 color;     // rgb=色
		float   intensity; // 強度
	};

	// 後方互換用：以前の「シーンにDirectionalLightが1つだけ」設計の構造体。
	// Model/MeshのGetDirectionalLight()等、旧API向けの受け渡しにのみ使用する。
	// 新規コードでは使わず、Lightを直接使うこと。
	struct DirectionalLight {
		Vector4 color;
		Vector3 direction;
		float   intensity;
	};
}