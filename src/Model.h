#pragma once

#include "Texture.h"
#include "assimp/scene.h"
#include "Component.h"
#include "PipelineState.h"
#include "utils/maths.h"
#include "../res/shaders/shared/shared_cb.h"

class Mesh;

template <typename T>
class ConstantBuffer;

class Model : public Component
{
public:
    Model() = default;
    static Model* create(std::string const& model_path);
    ~Model() = default;

    void setConstantBuffer();
    void draw();
    void update() override;
    void drawEditor() override;
    void serializeMeshes() const;
    bool deserializeMeshes();

    std::vector<Texture*> m_loaded_textures;
private:
    void loadModel(std::string const& model_path);
    void processNode(aiNode const* node, aiScene const* scene);
    Mesh* processMesh(aiMesh const* mesh, aiScene const* scene);

    void uploadGPUResources();
    std::vector<Texture*> loadMaterialTextures(aiMaterial const* material, aiTextureType type, TextureType type_name);

    std::vector<Mesh*> m_meshes;
    std::string m_directory;


    ConstantBuffer<SceneConstantBuffer>* m_sceneConstantBuffer;
    SceneConstantBuffer m_sceneConstantBufferData;

    ConstantBuffer<CameraConstants>* m_cameraConstantBuffer;
    CameraConstants m_cameraConstants;


    // STATS FOR EDITOR
    int m_vertexCount = 0;
    int m_triangleCount = 0;
    int m_meshletsCount = 0;

    int m_TypeIndex = 0;

    std::string m_path;

    bool freezeCamera = false;


    PipelineState* m_pipelineState;
};

