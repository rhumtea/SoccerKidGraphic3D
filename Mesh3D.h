#pragma once
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "ShaderProgram.h"
#include "Texture.h"

constexpr int MAX_BONE_PER_VERTEX = 4;

struct Vertex3D {
	// position
	glm::vec3 Position;
	// normal
	glm::vec3 Normal;
	// texCoords
	glm::vec2 TexCoords;
	// tangent
	glm::vec3 Tangent;

	// bone indexes which will influence this vertex
	int m_BoneIDs[MAX_BONE_PER_VERTEX];
	//weights from each bone
	float m_Weights[MAX_BONE_PER_VERTEX];
};

/**
 * @brief Represents a mesh whose vertices have positions, normal vectors, and texture coordinates;
 * as well as a list of Textures to bind when rendering the mesh.
 */
class Mesh3D {
private:
	uint32_t m_vao;
	std::vector<Texture> m_textures;
	size_t m_vertexCount;
	size_t m_faceCount;

public:

	
	/**
	 * @brief Construcst a Mesh3D using existing vectors of vertices and faces.
	*/
	Mesh3D(std::vector<Vertex3D>&& vertices, std::vector<uint32_t>&& faces, 
		Texture texture);

	Mesh3D(std::vector<Vertex3D>&& vertices, std::vector<uint32_t>&& faces,
		std::vector<Texture>&& textures);

	void addTexture(Texture texture);

	/**
	 * @brief Constructs a 1x1 square centered at the origin in world space.
	*/
	static Mesh3D square(const std::vector<Texture>& textures);
	/**
	 * @brief Constructs a 1x1x1 cube centered at the origin in world space.
	*/
	static Mesh3D cube(Texture texture);

	/**
	 * @brief Renders the mesh to the given context.
	 */
	void render(sf::RenderWindow& window, ShaderProgram& program) const;
	
};
