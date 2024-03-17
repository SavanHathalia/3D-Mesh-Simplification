#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
	unsigned int ID;

	Shader(const char* vertexPath, const char* fragmentPath);
	~Shader();

	void Activate();

	// Set uniform helpers
	void setFloat(const std::string& name, float value);

	void setVec2(const std::string& name, const glm::vec2& vec);
	void setVec3(const std::string& name, const glm::vec3& vec);
	void setVec4(const std::string& name, const glm::vec4& vec);

	void setMat2(const std::string& name, const glm::mat2& mat);
	void setMat3(const std::string& name, const glm::mat3& mat);
	void setMat4(const std::string& name, const glm::mat4& mat);

private:
	// utility function for checking shader compilation/linking errors.
	void checkCompileErrors(unsigned int shader, std::string type);
};

#endif