#include "ModelLoader.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <dxgidebug.h>

namespace RyoEngine {

	std::vector<ModelLoader::MaterialData> ModelLoader::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
		std::string filePath = directoryPath + filename;
		OutputDebugStringA((filePath + "\n").c_str());

		std::vector<MaterialData> materials; // 構築するMaterialData(複数)
		std::string line;  // ファイルから読んだ1行を格納するもの
		std::ifstream file(filePath);  // ファイルを開く
		assert(file.is_open());  // とりあえず開けなかったら止める

		MaterialData* current = nullptr; // 現在パース中のマテリアル

		while (std::getline(file, line)) {
			std::string identifier;
			std::istringstream s(line);
			s >> identifier;

			// identifierに応じた処理
			if (identifier == "newmtl") {
				// 新しいマテリアルの開始
				materials.emplace_back();
				current = &materials.back();
				s >> current->name;
			} else if (identifier == "map_Kd" && current != nullptr) {
				std::string textureFilename;
				s >> textureFilename;
				// 連続してファイルパスにする
				current->textureFilePath = directoryPath + textureFilename;
			}
		}

		return materials;
	}

	ModelLoader::ModelData ModelLoader::LoadObjFile(const std::string& filePath) {
		OutputDebugStringA((filePath + "\n").c_str());
		ModelData modelData;  // 構築するModelData
		std::vector<Vector4> positions;  // 位置
		std::vector<Vector3> normals;  // 法線
		std::vector<Vector2> texcoords;  // テクスチャ座標
		std::string line;  // ファイルから読んだ1行を格納するもの

		// filePath からディレクトリパスを抽出 (mtllibの解決に使う)
		std::string directoryPath = "";
		size_t dirPos = filePath.find_last_of('/');
		if (dirPos != std::string::npos) {
			directoryPath = filePath.substr(0, dirPos + 1);
		}

		std::ifstream file(filePath);  // ファイルを開く
		assert(file.is_open());  // とりあえず開けなかったら止める

		// 最初のメッシュを用意しておく (usemtlが1度も出てこないobjにも対応するため)
		modelData.meshes.emplace_back();

		// マテリアル名 -> materials配列のインデックス を検索する
		auto findMaterialIndex = [&](const std::string& name) -> int32_t {
			for (size_t i = 0; i < modelData.materials.size(); ++i) {
				if (modelData.materials[i].name == name) {
					return static_cast<int32_t>(i);
				}
			}
			return -1;
			};

		while (std::getline(file, line)) {
			std::string identifier;
			std::istringstream s(line);
			s >> identifier;  // 先頭の識別子を読む

			// identifierに応じた処理
			if (identifier == "v") {
				Vector4 position;
				s >> position.x >> position.y >> position.z;
				position.w = 1.0f;
				positions.push_back(position);
			} else if (identifier == "vt") {
				Vector2 texcoord;
				s >> texcoord.x >> texcoord.y;
				texcoords.push_back(texcoord);
			} else if (identifier == "vn") {
				Vector3 normal;
				s >> normal.x >> normal.y >> normal.z;
				normals.push_back(normal);
			} else if (identifier == "f") {
				VertexData triangle[3]{};
				// この面(f)がUV(vt)インデックスを持たない頂点を1つでも含んでいたか
				// (Blender等でUV展開せずに書き出すと "f v//vn" のようにvtインデックスが空になる)
				bool faceHasUV = true;

				// 面は三角形限定。その他は未対応
				for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
					std::string vertexDefinition;
					s >> vertexDefinition;
					// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
					// "v/vt/vn" だけでなく "v//vn" (vt省略) にも対応するため、
					// 各要素はまず文字列のまま取り出し、空文字列かどうかで判定してからstoiする。
					std::istringstream v(vertexDefinition);
					std::string indexStrs[3];
					for (int32_t element = 0; element < 3; ++element) {
						std::getline(v, indexStrs[element], '/');  // 「/」区切りでインデックス文字列を読んでいく
					}

					// 位置 (必須)
					Vector4 position = positions[static_cast<size_t>(std::stoi(indexStrs[0])) - 1];

					// UV (省略されている場合がある。省略時は(0,0)を入れ、faceHasUVをfalseにする)
					Vector2 texcoord = { 0.0f, 0.0f };
					if (!indexStrs[1].empty()) {
						texcoord = texcoords[static_cast<size_t>(std::stoi(indexStrs[1])) - 1];
						texcoord.y = 1.0f - texcoord.y;
					} else {
						faceHasUV = false;
					}

					// 法線 (必須)
					Vector3 normal = normals[static_cast<size_t>(std::stoi(indexStrs[2])) - 1];

					position.x *= -1.0f;
					normal.x *= -1.0f;

					triangle[faceVertex] = { position,texcoord,normal };
				}

				// 現在アクティブなメッシュ(末尾)に、頂点を逆順で登録することで、周り順を逆にする
				MeshData& currentMesh = modelData.meshes.back();
				if (!faceHasUV) {
					currentMesh.hasUV = false;
				}
				currentMesh.vertices.push_back(triangle[2]);
				currentMesh.vertices.push_back(triangle[1]);
				currentMesh.vertices.push_back(triangle[0]);
			} else if (identifier == "mtllib") {
				std::string materialFilename;
				s >> materialFilename;
				// mtlファイルに含まれる全マテリアルを読み込んで追加する
				std::vector<MaterialData> loaded = LoadMaterialTemplateFile(directoryPath, materialFilename);
				modelData.materials.insert(modelData.materials.end(), loaded.begin(), loaded.end());
			} else if (identifier == "usemtl") {
				std::string materialName;
				s >> materialName;
				int32_t materialIndex = findMaterialIndex(materialName);
				if (materialIndex < 0) {
					// 見つからない場合は先頭のマテリアルにフォールバック
					materialIndex = 0;
				}

				// 現在のメッシュに既に頂点が入っているなら、マテリアルの切り替わりとして
				// 新しいメッシュを作る。空のままなら使い回す。
				if (!modelData.meshes.back().vertices.empty()) {
					modelData.meshes.emplace_back();
				}
				modelData.meshes.back().materialIndex = static_cast<uint32_t>(materialIndex);
			}
		}

		// マテリアルが1つも定義されていなければダミーを1つ入れておく(白テクスチャ扱いになる)
		if (modelData.materials.empty()) {
			modelData.materials.push_back(MaterialData{});
		}

		// 頂点が1つも積まれなかった空メッシュは取り除く
		modelData.meshes.erase(
			std::remove_if(modelData.meshes.begin(), modelData.meshes.end(),
				[](const MeshData& mesh) { return mesh.vertices.empty(); }),
			modelData.meshes.end());

		return modelData;
	}

}
