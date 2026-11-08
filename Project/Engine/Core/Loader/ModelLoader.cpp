#include "ModelLoader.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <dxgidebug.h>
#include "../../Core/Base/Logger.h"
#pragma warning(push)
#pragma warning(disable: 4865 4267 5267 4619 4351)
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma warning(pop)

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
				// 残り全部（オプション + ファイル名）を読み取る
				std::vector<std::string> tokens;
				std::string token;
				while (s >> token) {
					tokens.push_back(token);
				}

				// 既知のmapオプションフラグとその引数の数（Wavefront MTL仕様）
				static const std::unordered_map<std::string, int> kOptionArgCounts = {
					{"-blendu", 1}, {"-blendv", 1}, {"-cc", 1}, {"-clamp", 1},
					{"-mm", 2}, {"-o", 3}, {"-s", 3}, {"-t", 3},
					{"-texres", 1}, {"-bm", 1}, {"-imfchan", 1}, {"-type", 1},
				};

				size_t i = 0;
				while (i < tokens.size()) {
					auto it = kOptionArgCounts.find(tokens[i]);
					if (it != kOptionArgCounts.end()) {
						i += 1 + it->second; // オプション名 + 引数の分だけスキップ
					} else {
						break; // オプションでなければ、そこがファイル名
					}
				}

				std::string textureFilename = (i < tokens.size()) ? tokens[i] : "";
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
				// 面を構成する全頂点を先に読み込む(3つとは限らない)
				std::vector<std::array<std::string, 3>> faceVertexDefs; // 各頂点の "v/vt/vn" 分解結果
				std::string vertexDefinition;
				while (s >> vertexDefinition) {
					std::istringstream v(vertexDefinition);
					std::array<std::string, 3> indexStrs{};
					for (int32_t element = 0; element < 3; ++element) {
						std::getline(v, indexStrs[element], '/');
					}
					faceVertexDefs.push_back(indexStrs);
				}

				if (faceVertexDefs.size() < 3) continue; // 不正な面はスキップ

				// 各頂点インデックスからVertexDataを構築するヘルパー
				bool faceHasUV = true;
				auto buildVertex = [&](const std::array<std::string, 3>& indexStrs) -> VertexData {
					Vector4 position = positions[static_cast<size_t>(std::stoi(indexStrs[0])) - 1];

					Vector2 texcoord = { 0.0f, 0.0f };
					if (!indexStrs[1].empty()) {
						texcoord = texcoords[static_cast<size_t>(std::stoi(indexStrs[1])) - 1];
						texcoord.y = 1.0f - texcoord.y;
					} else {
						faceHasUV = false;
					}

					Vector3 normal = normals[static_cast<size_t>(std::stoi(indexStrs[2])) - 1];

					position.x *= -1.0f;
					normal.x *= -1.0f;

					return { position, texcoord, normal };
					};

				std::vector<VertexData> faceVertices;
				faceVertices.reserve(faceVertexDefs.size());
				for (auto& indexStrs : faceVertexDefs) {
					faceVertices.push_back(buildVertex(indexStrs));
				}

				MeshData& currentMesh = modelData.meshes.back();
				if (!faceHasUV) {
					currentMesh.hasUV = false;
				}

				// 三角形ファン分割: (0,1,2), (0,2,3), (0,3,4), ...
				for (size_t i = 1; i + 1 < faceVertices.size(); ++i) {
					// 元の実装同様、頂点を逆順で積んで周り順を反転させる
					currentMesh.vertices.push_back(faceVertices[i + 1]);
					currentMesh.vertices.push_back(faceVertices[i]);
					currentMesh.vertices.push_back(faceVertices[0]);
				}
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
					Logger::LogWarning("[ModelLoader] L185\nMaterial not found.\n");
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

	ModelLoader::ModelData ModelLoader::LoadModelFile(const std::string& filePath) {
		size_t dot = filePath.find_last_of('.');
		std::string ext = (dot != std::string::npos) ? filePath.substr(dot) : "";
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

		if (ext == ".obj") {
			return LoadObjFile(filePath);  // objは既存の自作ローダーをそのまま使う
		}
		return LoadWithAssimp(filePath);
	}

	ModelLoader::ModelData ModelLoader::LoadWithAssimp(const std::string& filePath) {
		OutputDebugStringA((filePath + "\n").c_str());
		ModelData modelData;

		// テクスチャパスの解決に使うディレクトリ (末尾に区切り文字を含む)
		std::string directoryPath;
		size_t dirPos = filePath.find_last_of("/\\");
		if (dirPos != std::string::npos) {
			directoryPath = filePath.substr(0, dirPos + 1);
		}

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filePath,
			aiProcess_Triangulate |           // 全ポリゴンを三角形化
			aiProcess_GenSmoothNormals |      // 法線が無ければ生成
			aiProcess_PreTransformVertices);  // ノード階層のTransformを頂点に焼き込む (静的モデル用)

		if (!scene || !scene->mRootNode) {
			Logger::LogWarning("[ModelLoader] Assimp load failed.\n");
			OutputDebugStringA((std::string(importer.GetErrorString()) + "\n").c_str());  // 失敗理由を出力
			assert(false && "Assimp load failed");
			return modelData;
		}

		// ---- マテリアル ----
		for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
			const aiMaterial* src = scene->mMaterials[i];
			MaterialData material;
			material.name = src->GetName().C_Str();

			aiString texPath;
			// glTFのbaseColorTextureは BASE_COLOR に入る (環境によっては DIFFUSE にも入る)
			if (src->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS ||
				src->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
				// "*0" のような形式は glb の埋め込みテクスチャ (現状は未対応なのでパスは空のまま)
				if (texPath.length > 0 && texPath.C_Str()[0] != '*') {
					material.textureFilePath = directoryPath + texPath.C_Str();
				} else if (texPath.length > 0) {
					Logger::LogWarning("[ModelLoader] Embedded texture is not supported yet.\n");
				}
			}
			modelData.materials.push_back(material);
		}
		// マテリアルが1つも無ければダミーを1つ入れておく (白テクスチャ扱い)
		if (modelData.materials.empty()) {
			modelData.materials.push_back(MaterialData{});
		}

		// ---- メッシュ ----
		for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
			const aiMesh* src = scene->mMeshes[m];

			MeshData mesh;
			mesh.materialIndex = (src->mMaterialIndex < modelData.materials.size()) ? src->mMaterialIndex : 0;
			mesh.hasUV = src->HasTextureCoords(0);

			// 頂点1つぶんをVertexDataに変換する (objローダーと同じ座標系変換を行う)
			auto buildVertex = [&](unsigned int idx) -> VertexData {
				Vector4 position = { src->mVertices[idx].x, src->mVertices[idx].y, src->mVertices[idx].z, 1.0f };

				Vector3 normal = { 0.0f, 1.0f, 0.0f };
				if (src->HasNormals()) {
					normal = { src->mNormals[idx].x, src->mNormals[idx].y, src->mNormals[idx].z };
				}

				Vector2 texcoord = { 0.0f, 0.0f };
				if (mesh.hasUV) {
					texcoord = { src->mTextureCoords[0][idx].x, 1.0f - src->mTextureCoords[0][idx].y };
				}

				position.x *= -1.0f;
				normal.x *= -1.0f;

				return { position, texcoord, normal };
				};

			mesh.vertices.reserve(static_cast<size_t>(src->mNumFaces) * 3);
			for (unsigned int f = 0; f < src->mNumFaces; ++f) {
				const aiFace& face = src->mFaces[f];
				if (face.mNumIndices != 3) continue;  // 点・線は無視

				// objローダーと同じく、頂点を逆順で積んで周り順を反転させる
				mesh.vertices.push_back(buildVertex(face.mIndices[2]));
				mesh.vertices.push_back(buildVertex(face.mIndices[1]));
				mesh.vertices.push_back(buildVertex(face.mIndices[0]));
			}

			if (!mesh.vertices.empty()) {
				modelData.meshes.push_back(std::move(mesh));
			}
		}

		return modelData;
	}

}