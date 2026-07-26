#pragma once
#include"../Math/Geometry.h"
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
		};

		// 1つのメッシュ (同一マテリアルで構成される頂点のかたまり)
		struct MeshData {
			std::vector<VertexData> vertices;
			uint32_t materialIndex = 0; // materials 配列内のインデックス
		};

		// モデル全体のデータ (複数メッシュ・複数マテリアルを保持)
		struct ModelData {
			std::vector<MeshData> meshes;
			std::vector<MaterialData> materials;
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

	};
}
