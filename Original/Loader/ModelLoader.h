#pragma once
#include"../Math/Geometry.h"
#include <vector>
#include <string>

class ModelLoader {
public:
	ModelLoader();
	~ModelLoader();

	struct MaterialData {
		std::string textureFilePath;
	};

	struct ModelData {
		std::vector<VertexData> vertices;
		MaterialData material;
	};

	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);





private:
};