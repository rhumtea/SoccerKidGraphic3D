#pragma once
#include <glm/ext.hpp>
#include <string>
class ShaderProgram {
	uint32_t m_programId;

public:
	ShaderProgram();

	void activate();

	void setUniform(const std::string& uniformName, bool value);
	void setUniform(const std::string& uniformName, int32_t value);
	void setUniform(const std::string& uniformName, float_t value);
	void setUniform(const std::string& uniformName, const glm::vec2& value);
	void setUniform(const std::string& uniformName, const glm::vec3& value);
	void setUniform(const std::string& uniformName, const glm::vec4& value);
	void setUniform(const std::string& uniformName, const glm::mat2& value);
	void setUniform(const std::string& uniformName, const glm::mat3& value);
	void setUniform(const std::string& uniformName, const glm::mat4& value);

	void load(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);    
};