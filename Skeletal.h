#pragma once
#include "BoneInfo.h"
#include "Object3D.h"
#include <unordered_map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <unordered_map>
#include <algorithm>

class Skeletal
{
public:
	Skeletal(const std::string& path, bool flipTextureCoords);

	Object3D& getRoot() { return m_root; }

	auto& GetBoneInfoMap() { return m_BoneInfoMap; }
	int& GetBoneCount() { return m_BoneCounter; }

private:
	Object3D m_root;
	std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
	int m_BoneCounter = 0;

	Object3D s_assimpLoad(const std::string& path, bool flipTextureCoords);

	Object3D s_processAssimpNode(aiNode* node, const aiScene* scene,
		const std::filesystem::path& modelPath,
		std::unordered_map<std::filesystem::path, Texture>& loadedTextures);

	Mesh3D s_fromAssimpMesh(const aiMesh* mesh, const aiScene* scene, const std::filesystem::path& modelPath,
		std::unordered_map<std::filesystem::path, Texture>& loadedTextures);

	void ExtractBoneWeightForVertices(std::vector<Vertex3D>& vertices, const aiMesh* mesh, const aiScene* scene);
};
