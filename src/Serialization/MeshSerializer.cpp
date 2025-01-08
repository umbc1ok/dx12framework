#include "MeshSerializer.h"

bool serializers::serializeMesh(
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<Meshlet>& meshlets,
    const std::vector<uint32_t>& meshletTriangles,
    const std::vector<uint32_t>& attributes,
    const std::vector<hlsl::float3>& positions,
    const std::vector<hlsl::float3>& normals,
    const std::vector<hlsl::float2>& UVs,
    const std::vector<CullData>& cullData,
    int32_t MeshletMaxVerts,
    int32_t MeshletMaxPrims,
    MeshletizerType type,
    const std::string& fileName)
{
    std::ofstream out(fileName, std::ios::binary);
    if (!out.is_open())
    {
        return false;
    }

    serializeVector(out, vertices);
    serializeVector(out, indices);
    serializeVector(out, meshlets);
    serializeVector(out, meshletTriangles);
    serializeVector(out, attributes);
    serializeVector(out, positions);
    serializeVector(out, normals);
    serializeVector(out, UVs);
    serializeVector(out, cullData);

    serializeObject(out, MeshletMaxVerts);
    serializeObject(out, MeshletMaxPrims);
    serializeObject(out, static_cast<int>(type));
    out.close();
    return true;
}

void serializers::deserializeMesh(
    std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices,
    std::vector<Meshlet>& meshlets,
    std::vector<uint32_t>& meshletTriangles,
    std::vector<uint32_t>& attributes,
    std::vector<hlsl::float3>& positions,
    std::vector<hlsl::float3>& normals,
    std::vector<hlsl::float2>& UVs,
    std::vector<CullData>& cullData,
    int32_t MeshletMaxVerts,
    int32_t MeshletMaxPrims,
    MeshletizerType& type,
    const std::string& fileName)
{
    std::ifstream in(fileName, std::ios::binary);
    if (!in.is_open())
    {
        return;
    }

    deserializeVector(in, vertices);
    deserializeVector(in, indices);
    deserializeVector(in, meshlets);
    deserializeVector(in, meshletTriangles);
    deserializeVector(in, attributes);
    deserializeVector(in, positions);
    deserializeVector(in, normals);
    deserializeVector(in, UVs);
    deserializeVector(in, cullData);

    deserializeObject(in, MeshletMaxVerts);
    deserializeObject(in, MeshletMaxPrims);

    int typeInt;
    deserializeObject(in, typeInt);
    type = static_cast<MeshletizerType>(typeInt);
    in.close();
}


