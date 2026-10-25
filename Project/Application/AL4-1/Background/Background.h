#pragma once
#include <RyoEngine.h>
#include <array>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace AL4_1 {
	// 背景の装飾（横に流れて、ループ範囲の外に出たら反対側から出てくる）
	class Background {
	public:
		// 種類（奥から手前の順）
		enum class Type {
			Wall,
			Pillar,
			Pipe,
			Server,
			Monitor,
			DataCube,
			Cube,
			Count
		};

		Background();
		~Background();
		// コピー禁止（ユニークポインタを持つので）
		Background(const Background&) = delete;
		Background& operator=(const Background&) = delete;
		// ムーブは許可
		Background(Background&&) noexcept = default;
		Background& operator=(Background&&) noexcept = default;

		// ParamEditor::Load() より前に呼ぶ
		void RegisterInParamEditor();
		// ParamEditor::Load() の後に呼ぶ（ロードした個数でモデルを作る）
		// center: 並べる基準の位置
		void Initialize(const RyoEngine::Vector3& center);
		// center: ループの基準の位置（プレイヤーの位置を渡す）
		void Update(const RyoEngine::Vector3& center);
		void Draw(RyoEngine::Camera& camera);

	private:
		// 1つ分
		struct Object {
			Object() = default;
			// コピー禁止（ユニークポインタを持つので）
			Object(const Object&) = delete;
			Object& operator=(const Object&) = delete;
			// ムーブは許可
			Object(Object&&) noexcept = default;
			Object& operator=(Object&&) noexcept = default;

			std::unique_ptr<RyoEngine::Model> model = nullptr;
			RyoEngine::Transform transform{
				.scale = {1.0f,1.0f,1.0f}
			};
			float yRatio = 0.0f;
			float scaleRatio = 0.0f;
		};

		// 種類ごとの設定
		struct Group {
			std::string name;			// ParamEditorのグループ名
			std::string modelPath;
			int32_t count = 0;			// 個数（変えたら再起動で反映）
			float z = 20.0f;			// 奥行き
			float yMin = 0.0f;			// 出てくる高さの範囲
			float yMax = 0.0f;
			float scaleMin = 1.0f;		// 大きさの範囲
			float scaleMax = 1.0f;
			float speed = 0.0f;			// 横に流れる速さ（負で左へ。0で静止）
			RyoEngine::Vector3 rotateSpeed = { 0.0f,0.0f,0.0f };	// 自転の速さ（度/秒）
			bool isRandomRotate = false;	// 出てくるたびに向きをランダムにするか
			float loopHalfWidth = 40.0f;	// 基準からこの距離より外に出たら反対側へ（奥ほど広くする）
			RyoEngine::Vector4 color = { 1.0f,1.0f,1.0f,1.0f };
			RyoEngine::Vector3 emissiveColor = { 0.0f,0.0f,0.0f };
			float emissiveIntensity = 0.0f;

			std::vector<Object> objects;
		};

		// 初期値
		void SetupDefaults();
		// 出直すときに高さ・大きさ・向きを決め直す（xはそのまま）
		void Respawn(Group& group, Object& object);

	private:
		// モデルの置き場所
		std::string modelDirectory_ = "Resources/ApplicationResources/AL4-1/Stage/";
		// グループ
		std::array<Group, static_cast<size_t>(Type::Count)> groups_{};

		// 全体
		bool isEnabled_ = true;
		float speedScale_ = 1.0f;	// 全体の速さの倍率
	};
}
