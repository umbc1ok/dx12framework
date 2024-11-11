#pragma once
#include <vector>
#include <wrl/client.h>

#include "Component.h"
#include "utils/maths.h"
#include "../res/shaders/shared/shared_cb.h"
#include <d3d12.h>

#include "PipelineState.h"
#include "DX12Wrappers/ConstantBuffer.h"


class Grass :
    public Component
{
public:
    Grass();
    ~Grass() = default;
    void update() override;
    void start() override;
    void draw_editor() override;
    void generate_blades();
    //void create_CBV();
    void upload_GPU_resources();
private:
    void set_constant_buffer();
    void dispatch();

    float m_base_force = 0.0f;

    bool m_oscilateWind = false;
    float m_oscilationsStrength = 1.0f;
    float m_oscillationsSpeed = 1.0f;
    float m_currentOscillation = 0.0f;
    bool m_oscilationRising = true;

    bool m_pulsateWind = false;
    float m_pulsationStrength = 1.0f;
    float m_pulsationSpeed = 1.0f;
    float m_currentPulsation = 0.0f;
    bool m_pulsationRising = true;


    std::vector<Blade> m_blades;

    SceneConstantBuffer m_constant_buffer_data;
    ConstantBuffer<SceneConstantBuffer>* m_constant_buffer;

    Wind* wind;
    ConstantBuffer<Wind>* m_WindConstantBuffer;

    Microsoft::WRL::ComPtr<ID3D12Resource>  m_bladesResource;

    PipelineState* m_pipeline_state;
};

