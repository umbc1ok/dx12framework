#pragma once
#include <chrono>
#include <string>

#include "MeshletStructs.h"
#include "utils/maths.h"

class MeshletBenchmark
{
public:
    MeshletBenchmark() = default;
    ~MeshletBenchmark() = default;

    static void create();

    void update(float time);

    void startMeshletizing();
    void endMeshletizing();

    static MeshletBenchmark* getInstance();

    void drawEditor();

    void updateMeshletizerType(MeshletizerType type);
    void updateMeshletParameters(uint32_t maxVertices, uint32_t maxPrimitives);

    void generateBenchmarkPositions();

private:
    void run(uint32_t numberOfFrames);
    bool saveLogToFile();
    static MeshletBenchmark* m_instance;

    float m_outerRadius = 40.0f;
    float m_innerRadius = 12.0f;

    char m_filename[128] = "";
    char m_notes[1024] = "";

    // This should stay at 0,0,0. But I added it if there are some special cases.
    hlsl::float3 m_center = hlsl::float3(0.0f, 0.0f, 0.0f);

    bool m_running = false;
    bool m_saveNow = false;
    uint32_t m_framesLeft = 0;
    uint32_t m_scheduledFrames = 0;
    float m_accumulatedTime = 0;


    MeshletizerType m_meshletizerType = DXMESH;
    uint32_t m_maxVertices = 64;
    uint32_t m_maxPrimitives = 126;
    float m_meshletizingTime = 0.0f;
    std::chrono::high_resolution_clock::time_point m_meshletizingStart;

    std::vector<hlsl::float3> m_positions;
    std::vector<hlsl::float3> m_lookAts;


    std::string m_path = "../../cache/logs/";
};


