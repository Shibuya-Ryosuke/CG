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

	namespace {
		// assimp を使う読み込みの部品 (定義はファイルの後ろ)
		void LoadAssimpMaterials(const aiScene* scene, const std::string& filePath, const std::string& directoryPath,
			std::vector<ModelLoader::MaterialData>& outMaterials);
		bool IsSkeletalScene(const aiScene* scene);
		ModelLoader::ModelData LoadSkeletalScene(const aiScene* scene, const std::string& filePath, const std::string& directoryPath);
		void BuildSkeleton(const aiNode* node, int32_t parent, SkeletonData& skeleton);
		std::vector<AnimationData> ReadAnimations(const aiScene* scene);
	}

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
			aiProcess_LimitBoneWeights);      // 1頂点のボーンを4本までにする (スキニング用。ボーンが無ければ何もしない)

		if (!scene || !scene->mRootNode) {
			Logger::LogWarning("[ModelLoader] Assimp load failed.\n");
			OutputDebugStringA((std::string(importer.GetErrorString()) + "\n").c_str());  // 失敗理由を出力
			assert(false && "Assimp load failed");
			return modelData;
		}

		// ボーンかアニメーションを持つファイルは、ノード階層を残したまま骨格付きで読む
		if (IsSkeletalScene(scene)) {
			return LoadSkeletalScene(scene, filePath, directoryPath);
		}

		// ノード階層のTransformを頂点に焼き込む (静的モデル用)
		scene = importer.ApplyPostProcessing(aiProcess_PreTransformVertices);
		if (!scene) {
			Logger::LogWarning("[ModelLoader] Assimp load failed.\n");
			OutputDebugStringA((std::string(importer.GetErrorString()) + "\n").c_str());  // 失敗理由を出力
			assert(false && "Assimp load failed");
			return modelData;
		}

		// ---- マテリアル ----
		LoadAssimpMaterials(scene, filePath, directoryPath, modelData.materials);

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


	namespace {
	// =====================================================================
	// マテリアル (静的モデル・骨格付きモデル共通)
	// =====================================================================
	void LoadAssimpMaterials(const aiScene* scene, const std::string& filePath, const std::string& directoryPath,
		std::vector<ModelLoader::MaterialData>& outMaterials) {
		for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
			const aiMaterial* src = scene->mMaterials[i];
			ModelLoader::MaterialData material;
			material.name = src->GetName().C_Str();

			aiString texPath;
			// glTFのbaseColorTextureは BASE_COLOR に入る (環境によっては DIFFUSE にも入る)
			if (src->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS ||
				src->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
				if (texPath.length > 0) {
					// "*0" のような形式や、glb の中に入っている画像は、ファイルではなくデータとして受け取る
					const auto [embedded, embeddedIndex] = scene->GetEmbeddedTextureAndIndex(texPath.C_Str());
					if (embedded) {
						material.embeddedTextureKey = filePath + "*" + std::to_string(embeddedIndex);
						if (embedded->mHeight == 0) {
							// png などの圧縮された画像: mWidth がバイト数
							const uint8_t* bytes = reinterpret_cast<const uint8_t*>(embedded->pcData);
							material.embeddedTextureData.assign(bytes, bytes + embedded->mWidth);
						} else {
							// 圧縮されていない画素 (aiTexel = b, g, r, a の順の4バイト)
							material.embeddedTextureWidth = embedded->mWidth;
							material.embeddedTextureHeight = embedded->mHeight;
							const uint8_t* bytes = reinterpret_cast<const uint8_t*>(embedded->pcData);
							material.embeddedTextureData.assign(bytes,
								bytes + static_cast<size_t>(embedded->mWidth) * embedded->mHeight * sizeof(aiTexel));
						}
					} else if (texPath.C_Str()[0] != '*') {
						material.textureFilePath = directoryPath + texPath.C_Str();
					} else {
						Logger::LogWarning("[ModelLoader] 埋め込みテクスチャが見つかりませんでした: {} ({})\n", texPath.C_Str(), filePath);
					}
				}
			}
			outMaterials.push_back(material);
		}
		// マテリアルが1つも無ければダミーを1つ入れておく (白テクスチャ扱い)
		if (outMaterials.empty()) {
			outMaterials.push_back(ModelLoader::MaterialData{});
		}
	}

	// =====================================================================
	// 骨格・アニメーション
	// =====================================================================
	// 座標系の変換について:
	//   メッシュは「X を反転して、面の向き(頂点の順番)を逆にする」ことでエンジンの座標系にしている。
	//   骨格・アニメーションも同じ変換をかけないと、メッシュとずれる。
	//   ・位置      : x を反転            (x, y, z) → (-x, y, z)
	//   ・回転      : y と z を反転        (x, y, z, w) → (x, -y, -z, w)   (鏡に映した回転)
	//   ・スケール  : そのまま
	//   ・行列      : assimp は列ベクトル規約なので転置してから、X を反転 (S * M * S、S = X だけ -1 の行列)

	bool IsSkeletalScene(const aiScene* scene) {
		if (scene->HasAnimations()) {
			return true;
		}
		for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
			if (scene->mMeshes[m]->HasBones()) {
				return true;
			}
		}
		return false;
	}

	Vector3 ConvertPosition(const aiVector3D& v) {
		return { -v.x, v.y, v.z };
	}

	Quaternion ConvertRotation(const aiQuaternion& q) {
		return Normalize(Quaternion{ q.x, -q.y, -q.z, q.w });
	}

	Matrix4x4 ConvertMatrix(const aiMatrix4x4& source) {
		const float values[4][4] = {
			{ source.a1, source.a2, source.a3, source.a4 },
			{ source.b1, source.b2, source.b3, source.b4 },
			{ source.c1, source.c2, source.c3, source.c4 },
			{ source.d1, source.d2, source.d3, source.d4 },
		};
		Matrix4x4 result{};
		for (int row = 0; row < 4; ++row) {
			for (int column = 0; column < 4; ++column) {
				// 転置 (列ベクトル規約 → 行ベクトル規約)
				float value = values[column][row];
				// X の反転: 行と列のどちらか一方だけが X(0番) のとき、符号が変わる
				if ((row == 0) != (column == 0)) {
					value = -value;
				}
				result.m[row][column] = value;
			}
		}
		return result;
	}

	void BuildSkeleton(const aiNode* node, int32_t parent, SkeletonData& skeleton) {
		JointData joint;
		joint.name = node->mName.C_Str();
		joint.parent = parent;

		aiVector3D scaling;
		aiQuaternion rotation;
		aiVector3D position;
		node->mTransformation.Decompose(scaling, rotation, position);
		joint.translate = ConvertPosition(position);
		joint.rotate = ConvertRotation(rotation);
		joint.scale = { scaling.x, scaling.y, scaling.z };

		const int32_t index = static_cast<int32_t>(skeleton.joints.size());
		skeleton.joints.push_back(joint);
		// 同じ名前のノードが複数あるときは、最初のものを名前で引けるようにする
		if (!skeleton.jointMap.emplace(joint.name, index).second) {
			Logger::LogWarning("[ModelLoader] 同じ名前のノードがあります (最初のものを使います): {}\n", joint.name);
		}

		for (unsigned int i = 0; i < node->mNumChildren; ++i) {
			BuildSkeleton(node->mChildren[i], index, skeleton);
		}
	}

	std::vector<AnimationData> ReadAnimations(const aiScene* scene) {
		std::vector<AnimationData> animations;
		for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
			const aiAnimation* src = scene->mAnimations[a];
			AnimationData animation;
			animation.name = src->mName.C_Str();
			// assimp の時間は「tick」単位。1秒あたりの tick 数で割って秒にする (0 のときは assimp の既定の 25)
			const double ticksPerSecond = src->mTicksPerSecond != 0.0 ? src->mTicksPerSecond : 25.0;
			animation.duration = static_cast<float>(src->mDuration / ticksPerSecond);

			for (unsigned int c = 0; c < src->mNumChannels; ++c) {
				const aiNodeAnim* channel = src->mChannels[c];
				NodeAnimation& nodeAnimation = animation.nodeAnimations[channel->mNodeName.C_Str()];

				nodeAnimation.translate.keyframes.reserve(channel->mNumPositionKeys);
				for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k) {
					const aiVectorKey& key = channel->mPositionKeys[k];
					nodeAnimation.translate.keyframes.push_back({ static_cast<float>(key.mTime / ticksPerSecond), ConvertPosition(key.mValue) });
				}
				nodeAnimation.rotate.keyframes.reserve(channel->mNumRotationKeys);
				for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k) {
					const aiQuatKey& key = channel->mRotationKeys[k];
					nodeAnimation.rotate.keyframes.push_back({ static_cast<float>(key.mTime / ticksPerSecond), ConvertRotation(key.mValue) });
				}
				nodeAnimation.scale.keyframes.reserve(channel->mNumScalingKeys);
				for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k) {
					const aiVectorKey& key = channel->mScalingKeys[k];
					nodeAnimation.scale.keyframes.push_back({ static_cast<float>(key.mTime / ticksPerSecond), { key.mValue.x, key.mValue.y, key.mValue.z } });
				}
			}
			animations.push_back(std::move(animation));
		}
		return animations;
	}

	ModelLoader::ModelData LoadSkeletalScene(const aiScene* scene, const std::string& filePath, const std::string& directoryPath) {
		ModelLoader::ModelData modelData;
		modelData.hasSkeleton = true;

		// ---- マテリアル ----
		LoadAssimpMaterials(scene, filePath, directoryPath, modelData.materials);

		// ---- 骨格 (ノード階層をそのままボーンにする) ----
		BuildSkeleton(scene->mRootNode, -1, modelData.skeleton);
		SkeletonData& skeleton = modelData.skeleton;

		// ---- メッシュ ----
		// ノードを骨格と同じ順番でたどり、ノードが持つメッシュを読む。
		// ・ボーンを持つメッシュ        : ボーンのウェイトでスキニングする
		// ・ボーンを持たないメッシュ    : そのノード1本に100%でついていく (ノードだけが動くアニメーション用)
		struct NodeEntry {
			const aiNode* node;
			int32_t jointIndex;
		};
		std::vector<NodeEntry> nodes;
		{
			// BuildSkeleton と同じ「深さ優先・子は前から順」の順番で番号を振る
			int32_t nextIndex = 0;
			auto visit = [&](auto&& self, const aiNode* node) -> void {
				nodes.push_back({ node, nextIndex++ });
				for (unsigned int i = 0; i < node->mNumChildren; ++i) {
					self(self, node->mChildren[i]);
				}
			};
			visit(visit, scene->mRootNode);
		}

		size_t verticesWithoutWeight = 0;
		for (const NodeEntry& entry : nodes) {
			for (unsigned int n = 0; n < entry.node->mNumMeshes; ++n) {
				const aiMesh* src = scene->mMeshes[entry.node->mMeshes[n]];

				ModelLoader::MeshData mesh;
				mesh.materialIndex = (src->mMaterialIndex < modelData.materials.size()) ? src->mMaterialIndex : 0;
				mesh.hasUV = src->HasTextureCoords(0);

				// 元の頂点番号ごとの影響 (ボーンが無いメッシュは、このノード1本に100%)
				std::vector<VertexInfluence> sourceInfluences(src->mNumVertices);
				if (src->HasBones()) {
					for (unsigned int b = 0; b < src->mNumBones; ++b) {
						const aiBone* bone = src->mBones[b];
						SkinBone skinBone;
						skinBone.jointIndex = skeleton.FindJoint(bone->mName.C_Str());
						if (skinBone.jointIndex < 0) {
							Logger::LogWarning("[ModelLoader] ボーンに対応するノードがありません (メッシュのノードで代用します): {}\n", bone->mName.C_Str());
							skinBone.jointIndex = entry.jointIndex;
						}
						skinBone.inverseBindMatrix = ConvertMatrix(bone->mOffsetMatrix);
						const uint32_t boneIndex = static_cast<uint32_t>(mesh.bones.size());
						mesh.bones.push_back(skinBone);

						for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
							const aiVertexWeight& weight = bone->mWeights[w];
							if (weight.mVertexId >= sourceInfluences.size() || weight.mWeight <= 0.0f) continue;
							VertexInfluence& influence = sourceInfluences[weight.mVertexId];
							// 空いている所、無ければ一番小さい所と入れ替える (4本より多いときは大きい4本を残す)
							int slot = 0;
							for (int s = 1; s < 4; ++s) {
								if (influence.weights[s] < influence.weights[slot]) slot = s;
							}
							if (weight.mWeight > influence.weights[slot]) {
								influence.weights[slot] = weight.mWeight;
								influence.indices[slot] = boneIndex;
							}
						}
					}
					// 合計が1になるようにする
					for (VertexInfluence& influence : sourceInfluences) {
						const float total = influence.weights[0] + influence.weights[1] + influence.weights[2] + influence.weights[3];
						if (total > 0.0f) {
							for (float& w : influence.weights) w /= total;
						} else {
							++verticesWithoutWeight;
						}
					}
				} else {
					SkinBone skinBone;
					skinBone.jointIndex = entry.jointIndex;
					skinBone.inverseBindMatrix = MakeIdentity4x4(); // 頂点はノードから見た座標なので、そのままでよい
					mesh.bones.push_back(skinBone);
					for (VertexInfluence& influence : sourceInfluences) {
						influence.weights[0] = 1.0f;
						influence.indices[0] = 0;
					}
				}

				// 頂点1つぶんをVertexDataに変換する (静的モデルと同じ座標系変換を行う)
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
				mesh.influences.reserve(static_cast<size_t>(src->mNumFaces) * 3);
				for (unsigned int f = 0; f < src->mNumFaces; ++f) {
					const aiFace& face = src->mFaces[f];
					if (face.mNumIndices != 3) continue;  // 点・線は無視
					// 静的モデルと同じく、頂点を逆順で積んで周り順を反転させる
					for (int corner = 2; corner >= 0; --corner) {
						const unsigned int idx = face.mIndices[corner];
						mesh.vertices.push_back(buildVertex(idx));
						mesh.influences.push_back(sourceInfluences[idx]);
					}
				}

				if (!mesh.vertices.empty()) {
					modelData.meshes.push_back(std::move(mesh));
				}
			}
		}
		if (verticesWithoutWeight > 0) {
			Logger::LogWarning("[ModelLoader] ボーンのウェイトが無い頂点が {} 個あります (動きません): {}\n", verticesWithoutWeight, filePath);
		}

		// ---- アニメーション ----
		modelData.animations = ReadAnimations(scene);
		return modelData;
	}
	}

	std::vector<AnimationData> ModelLoader::LoadAnimationFile(const std::string& filePath, SkeletonData& outSkeleton) {
		outSkeleton = SkeletonData{};
		Assimp::Importer importer;
		// アニメーションだけ使うので、メッシュの加工はしない
		const aiScene* scene = importer.ReadFile(filePath, 0);
		if (!scene || !scene->mRootNode) {
			Logger::LogWarning("[ModelLoader] アニメーションのファイルを読めませんでした: {}\n", filePath);
			OutputDebugStringA((std::string(importer.GetErrorString()) + "\n").c_str());  // 失敗理由を出力
			return {};
		}
		BuildSkeleton(scene->mRootNode, -1, outSkeleton);
		return ReadAnimations(scene);
	}
}