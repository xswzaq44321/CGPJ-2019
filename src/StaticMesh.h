#ifndef _STATIC_MESH_H_
#define _STATIC_MESH_H_

#include <glad/glad.h>
#include <string>
#include <glm/glm.hpp>
#include <vector>
class StaticMesh {
public:
    StaticMesh(const StaticMesh &rhs)=default;
    void release();

    static StaticMesh LoadMesh(const std::string &filename);
	void LoadInstancedArrays(const std::vector<glm::vec3> &);
    void draw();
	void drawInstanced(int count);

	bool hasNormal() const;
	bool hasUV() const;

    bool operator!=(const StaticMesh &rhs) const
    { return vao != rhs.vao; }
private:
    StaticMesh();
    GLuint vao;
    GLuint vbo[3];
	GLuint instanceVBO = -1;
    GLuint ibo;
    GLuint numIndices;

	bool m_uv, m_normal;
};

#endif