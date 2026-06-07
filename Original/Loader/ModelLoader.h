#pragma once
#include"../Math/Geometry.h"
#include <vector>
#include <string>

namespace RyoEngine {
	class ModelLoader {
	public:
		ModelLoader() = delete;
		~ModelLoader() = default;

		struct MaterialData {
			std::string textureFilePath;
		};

		struct ModelData {
			std::vector<VertexData> vertices;
			MaterialData material;
		};

		static MaterialData LoadMaterialTemplateFile(const std::string& filePath);

		static ModelData LoadObjFile(const std::string& filePath);

	};
}
