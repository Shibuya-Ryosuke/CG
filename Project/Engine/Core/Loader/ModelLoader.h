#pragma once
#include"../../Core/Math/Geometry.h"
#include "../Animation/AnimationData.h"
#include <cstdint>
#include <vector>
#include <string>

namespace RyoEngine {
	class ModelLoader {
	public:
		ModelLoader() = delete;
		~ModelLoader() = default;

		// 1つのマテリアル (mtlファイルの newmtl 1ブロック分)
		struct MaterialData {
			std::string name;             // newmtl の後ろの識別名 (usemtlとの対応付けに使う)
			std::string textureFilePath;  // map_Kd で指定されたテクスチャの完全パス

			// --- ファイルに埋め込まれたテクスチャ (glb など。あるときは textureFilePath より優先) ---
			std::vector<uint8_t> embeddedTextureData; // 画像のデータ (png などのファイルの中身、または BGRA の画素)
			uint32_t embeddedTextureWidth = 0;        // BGRA の画素のときの幅
			uint32_t embeddedTextureHeight = 0;       // BGRA の画素のときの高さ。0 なら png などの圧縮された画像
			std::string embeddedTextureKey;           // 同じ画像を2回読まないためのキー ("モデルのパス*番号")
		};

		// 1つのメッシュ (同一マテリアルで構成される頂点のかたまり)
		struct MeshData {
			std::vector<VertexData> vertices;
			uint32_t materialIndex = 0; // materials 配列内のインデックス

			// このメッシュの面(f)が1つでもUV(vt)を持たない頂点を含んでいた場合 false になる。
			// (Blenderなどでobjを出力する際、UV展開していないモデルは vt が存在せず "f v//vn" 形式になる)
			// false の場合、頂点のtexcoordには (0,0) が入る。
			bool hasUV = true;

			// --- スキニング (骨格を持つモデルのときだけ) ---
			// 頂点ごとの、ついていくボーンと割合 (vertices と同じ数。空ならスキニングしないメッシュ)
			std::vector<VertexInfluence> influences;
			// このメッシュがついていくボーン (VertexInfluence::indices はこの配列の番号)
			std::vector<SkinBone> bones;
		};

		// モデル全体のデータ (複数メッシュ・複数マテリアルを保持)
		struct ModelData {
			std::vector<MeshData> meshes;
			std::vector<MaterialData> materials;

			// --- 骨格・アニメーション (ボーンかアニメーションを持つファイルのときだけ) ---
			bool hasSkeleton = false;
			SkeletonData skeleton;
			std::vector<AnimationData> animations;
		};

		/// <summary>
		/// mtlファイルを読み込み、含まれる全マテリアルを返す
		/// </summary>
		/// <param name="directoryPath">objファイルのあるディレクトリ (末尾に"/"を含む)</param>
		/// <param name="filename">mtlファイル名 (mtllibで指定された値)</param>
		static std::vector<MaterialData> LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

		/// <summary>
		/// objファイルを読み込む。usemtlの切り替わりごとにメッシュを分割する。
		/// </summary>
		static ModelData LoadObjFile(const std::string& filePath);

		/// <summary>
		/// 拡張子を見て読み込み方法を振り分ける。
		/// .obj は自作ローダー (LoadObjFile)、それ以外 (.gltf / .glb など) は Assimp (LoadWithAssimp) で読み込む。
		/// </summary>
		static ModelData LoadModelFile(const std::string& filePath);

		/// <summary>
		/// Assimpでモデルを読み込む (gltf / glb など)。
		/// ノード階層のTransformは頂点に焼き込むため、静的モデル専用 (ボーン・スキンアニメーションは非対応)。
		/// (ボーンかアニメーションを持つファイルは、LoadSkeletalScene で骨格付きとして読む)
		/// </summary>
		static ModelData LoadWithAssimp(const std::string& filePath);

		/// <summary>
		/// アニメーションだけを読み込む (メッシュは読まない)。ほかのモデルに、別ファイルのアニメーションを足すとき用。
		/// outSkeleton にはそのファイルの骨格が入る (体格の違いを合わせるのに使う。無ければ空)
		/// </summary>
		static std::vector<AnimationData> LoadAnimationFile(const std::string& filePath, SkeletonData& outSkeleton);

	};
}