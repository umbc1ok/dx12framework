#pragma once
#include "DX12Wrappers/Vertex.h"

namespace visualisers
{
    void generateFrustumGeometry(float fov, float nearPlane, float farPlane, float aspectRatio, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    void generateSphereGeometry(float radius, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

}
