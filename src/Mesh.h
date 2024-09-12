#pragma once
#include <vector>


#include "DX12Wrappers/IndexBuffer.h"
#include "DX12Wrappers/Vertex.h"
#include "DX12Wrappers/VertexBuffer.h"
#include "Texture.h"

class Mesh
{
public:
    Mesh(std::vector<Vertex> const& m_vertices, std::vector<u16> const& m_indices, std::vector<Texture*> const& m_textures);
    ~Mesh() = default;

    void draw();
    void bind_textures();

private:
    VertexBuffer* m_vertex_buffer;
    IndexBuffer* m_index_buffer;

    std::vector<Vertex> m_vertices;
    std::vector<u16> m_indices;
    std::vector<Texture*> m_textures;

};

