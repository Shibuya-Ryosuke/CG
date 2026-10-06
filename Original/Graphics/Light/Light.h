#pragma once
#include <cstdint>
#include <json.hpp>
#include "../../Core/Math/Vector.h"

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


	// --- nlohmann/json 用の変換定義 ---
	inline void to_json(nlohmann::json& j, const Vector3& v) {
		j = nlohmann::json{ {"x", v.x}, {"y", v.y}, {"z", v.z} };
	}
	inline void from_json(const nlohmann::json& j, Vector3& v) {
		j.at("x").get_to(v.x);
		j.at("y").get_to(v.y);
		j.at("z").get_to(v.z);
	}

	inline void to_json(nlohmann::json& j, const Vector4& v) {
		j = nlohmann::json{ {"x", v.x}, {"y", v.y}, {"z", v.z}, {"w", v.w} };
	}
	inline void from_json(const nlohmann::json& j, Vector4& v) {
		j.at("x").get_to(v.x);
		j.at("y").get_to(v.y);
		j.at("z").get_to(v.z);
		j.at("w").get_to(v.w);
	}

	inline void to_json(nlohmann::json& j, const LightType& t) {
		j = static_cast<uint32_t>(t);
	}
	inline void from_json(const nlohmann::json& j, LightType& t) {
		t = static_cast<LightType>(j.get<uint32_t>());
	}

	inline void to_json(nlohmann::json& j, const Light& l) {
		j = nlohmann::json{
			{"color", l.color},
			{"direction", l.direction},
			{"intensity", l.intensity},
			{"position", l.position},
			{"range", l.range},
			{"spotAngle", l.spotAngle},
			{"spotFalloff", l.spotFalloff},
			{"type", l.type}
		};
	}
	inline void from_json(const nlohmann::json& j, Light& l) {
		j.at("color").get_to(l.color);
		j.at("direction").get_to(l.direction);
		j.at("intensity").get_to(l.intensity);
		j.at("position").get_to(l.position);
		j.at("range").get_to(l.range);
		j.at("spotAngle").get_to(l.spotAngle);
		j.at("spotFalloff").get_to(l.spotFalloff);
		j.at("type").get_to(l.type);
		l.padding = 0.0f;
	}

	inline void to_json(nlohmann::json& j, const AmbientLight& a) {
		j = nlohmann::json{
			{"color", a.color},
			{"intensity", a.intensity}
		};
	}
	inline void from_json(const nlohmann::json& j, AmbientLight& a) {
		j.at("color").get_to(a.color);
		j.at("intensity").get_to(a.intensity);
	}
}