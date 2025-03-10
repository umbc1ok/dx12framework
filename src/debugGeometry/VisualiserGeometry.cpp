#include "VisualiserGeometry.h"

#include <cmath>


namespace visualisers
{
    void generateFrustumGeometry(float fov, float nearPlane, float farPlane, float aspectRatio, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
    {

        float nearHeight = 2.0f * tan(fov / 2.0f) * nearPlane;
        float nearWidth = nearHeight * aspectRatio;

        // Calculate dimensions of the far plane
        float farHeight = 2.0f * tan(fov / 2.0f) * farPlane;
        float farWidth = farHeight * aspectRatio;

        // Near plane vertices
        float nearLeft = -nearWidth / 2.0f;
        float nearRight = nearWidth / 2.0f;
        float nearTop = nearHeight / 2.0f;
        float nearBottom = -nearHeight / 2.0f;

        // Far plane vertices
        float farLeft = -farWidth / 2.0f;
        float farRight = farWidth / 2.0f;
        float farTop = farHeight / 2.0f;
        float farBottom = -farHeight / 2.0f;

        vertices.clear();
        vertices.push_back(Vertex(hlsl::float3(nearLeft, nearTop, -nearPlane), hlsl::float3(), hlsl::float2()));   // 0 - Near Top-Left
        vertices.push_back(Vertex(hlsl::float3(nearRight, nearTop, -nearPlane), hlsl::float3(), hlsl::float2()));  // 1 - Near Top-Right
        vertices.push_back(Vertex(hlsl::float3(nearLeft, nearBottom, -nearPlane), hlsl::float3(), hlsl::float2())); // 2 - Near Bottom-Left
        vertices.push_back(Vertex(hlsl::float3(nearRight, nearBottom, -nearPlane), hlsl::float3(), hlsl::float2())); // 3 - Near Bottom-Right

        vertices.push_back(Vertex(hlsl::float3(farLeft, farTop, -farPlane), hlsl::float3(), hlsl::float2()));      // 4 - Far Top-Left
        vertices.push_back(Vertex(hlsl::float3(farRight, farTop, -farPlane), hlsl::float3(), hlsl::float2()));     // 5 - Far Top-Right
        vertices.push_back(Vertex(hlsl::float3(farLeft, farBottom, -farPlane), hlsl::float3(), hlsl::float2()));   // 6 - Far Bottom-Left
        vertices.push_back(Vertex(hlsl::float3(farRight, farBottom, -farPlane), hlsl::float3(), hlsl::float2()));  // 7 - Far Bottom-Right

        //vertices.push_back(Vertex(hlsl::float3(-1.0f, -1.0f, -1.0f), hlsl::float3(0, 0, 0), hlsl::float2(0.0f, 0.0f)));
        //vertices.push_back(Vertex(hlsl::float3(-1.0f, 1.0f, -1.0f), hlsl::float3(0, 0, 0), hlsl::float2(0.0f, 0.0f)));
        //vertices.push_back(Vertex(hlsl::float3(1.0f, 1.0f, -1.0f), hlsl::float3(0, 0, 0), hlsl::float2(0.0f, 0.0f)));
        //vertices.push_back(Vertex(hlsl::float3(1.0f, -1.0f, -1.0f), hlsl::float3(0, 0, 0), hlsl::float2(0.0f, 0.0f)));

        //vertices.push_back(Vertex(hlsl::float3(-1.0f, -1.0f, 1.0f), hlsl::float3(0, 0, 0), hlsl::float2(0.0f, 0.0f)));
        //vertices.push_back(Vertex(hlsl::float3(-1.0f, 1.0f, 1.0f), hlsl::float3(0, 0, 0), hlsl::float2(0.0f, 0.0f)));
        //vertices.push_back(Vertex(hlsl::float3(1.0f, 1.0f, 1.0f), hlsl::float3(0, 0, 0), hlsl::float2(0.0f, 0.0f)));
        //vertices.push_back(Vertex(hlsl::float3(1.0f, -1.0f, 1.0f), hlsl::float3(0, 0, 0), hlsl::float2(0.0f, 0.0f)));

        indices.clear();
        indices = {
            // Near Face (Vertices: 0, 1, 2, 3)
            0, 1, 3, 0, 3, 2,

            // Far Face (Vertices: 4, 5, 6, 7)
            4, 5, 7, 4, 7, 6,

            // Left Face (Vertices: 0, 2, 6, 4)
            0, 2, 6, 0, 6, 4,

            // Right Face (Vertices: 1, 5, 7, 3)
            1, 3, 7, 1, 7, 5,

            // Top Face (Vertices: 0, 1, 5, 4)
            0, 1, 5, 0, 5, 4,

            // Bottom Face (Vertices: 2, 3, 7, 6)
            2, 3, 7, 2, 7, 6
        };
    }


    void generateSphereGeometry(float radius, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
    {
        vertices.clear();
        indices.clear();

        const int X_SEGMENTS = 64;
        const int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;

        // Generate vertices
        for (int y = 0; y <= Y_SEGMENTS; ++y)
        {
            for (int x = 0; x <= X_SEGMENTS; ++x)
            {
                float xSegment = static_cast<float>(x) / X_SEGMENTS;
                float ySegment = static_cast<float>(y) / Y_SEGMENTS;
                float theta = xSegment * 2.0f * PI; // Longitude
                float phi = ySegment * PI;         // Latitude

                // Spherical to Cartesian conversion
                float xPos = radius * std::sin(phi) * std::cos(theta);
                float yPos = radius * std::cos(phi);
                float zPos = radius * std::sin(phi) * std::sin(theta);

                vertices.push_back(Vertex(
                    hlsl::float3(xPos, yPos, zPos),
                    hlsl::float3(xPos / radius, yPos / radius, zPos / radius), // Normalized normal
                    hlsl::float2(xSegment, ySegment)));
            }
        }

        // Generate indices for the sphere
        for (int y = 0; y < Y_SEGMENTS; ++y)
        {
            for (int x = 0; x < X_SEGMENTS; ++x)
            {
                // Calculate the indices of the four corners of a quad
                int topLeft = y * (X_SEGMENTS + 1) + x;
                int topRight = topLeft + 1;
                int bottomLeft = (y + 1) * (X_SEGMENTS + 1) + x;
                int bottomRight = bottomLeft + 1;

                // Triangle 1
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                // Triangle 2
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
    }



}
