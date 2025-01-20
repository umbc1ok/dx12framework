#include "MeshletBenchmark.h"

#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <random>

#include "Camera.h"
#include "GPUProfiler.h"
#include "Mesh.h"
#include "Renderer.h"
#include "debugGeometry/DebugDrawer.h"
#include "debugGeometry/VisualiserGeometry.h"
#include "Serialization/MeshSerializer.h"


MeshletBenchmark* MeshletBenchmark::m_instance;

void MeshletBenchmark::create()
{
    m_instance = new MeshletBenchmark();
}

void MeshletBenchmark::run(uint32_t numberOfFrames)
{
    m_framesLeft = numberOfFrames;
    m_scheduledFrames = numberOfFrames;
    m_running = true;
}

bool MeshletBenchmark::saveLogToFile()
{
    // Save the log to a file
    std::filesystem::path cwd = std::filesystem::current_path();
    std::string path = m_path + m_filename + ".log";
    std::ofstream file(path);
    if (file.is_open())
    {
        file << "Meshlet Benchmark\n";
        const char* items[] = { "MESHOPTIMIZER","DXMESH", "GREEDY", "BoundingSphere", "NVIDIA" };
        file << "Meshletizer type: " << items[m_meshletizerType] << "\n";
        file << "Max vertices: " << m_maxVertices << "\n";
        file << "Max primitives: " << m_maxPrimitives << "\n";
        file << "Meshletizing time: " << m_meshletizingTime << "s" << "\n";
        file << "Frames: " << m_scheduledFrames << "\n";
        file << "Average render time: " << m_accumulatedTime / m_scheduledFrames << (GPUProfiler::getInstance()->useMicroSeconds() ? "us" : "ms") << "\n";
        file << "Notes: " << m_notes << "\n";
        file.close();
        return true;
    }

    printf("Could not open file %s\n", path.c_str());
    return false;
}

bool MeshletBenchmark::savePositionSequenceToFile()
{
    std::ofstream file(m_sequencesPath
        + m_positionsFilename, std::ios::binary);
    if(file.is_open())
    {
        serializers::serializeVector(file, m_positions);
        serializers::serializeVector(file, m_lookAts);
        file.close();
        return true;
    }
    return false;
}

bool MeshletBenchmark::loadPositionSequenceFromFile()
{
    std::ifstream file(m_sequencesPath + m_positionsFilename, std::ios::binary);
    if (file.is_open())
    {
        serializers::deserializeVector(file, m_positions);
        serializers::deserializeVector(file, m_lookAts);
        file.close();
        return true;
    }
    return false;
}

void MeshletBenchmark::update(float time)
{
    if (m_running)
    {
        m_accumulatedTime += time;
        m_framesLeft--;
        if (m_framesLeft == 0)
        {
            m_running = false;
            m_saveNow = true;
        }
        Camera::getMainCamera()->entity->transform->set_position(m_positions[m_framesLeft]);
        Camera::getMainCamera()->setLookAt(m_lookAts[m_framesLeft]);
    }
}

void MeshletBenchmark::startMeshletizing()
{
    m_meshletizingStart = std::chrono::high_resolution_clock::now();
}

void MeshletBenchmark::endMeshletizing()
{
    auto end = std::chrono::high_resolution_clock::now();
    m_meshletizingTime = std::chrono::duration<float>(end - m_meshletizingStart).count();
}


MeshletBenchmark* MeshletBenchmark::getInstance()
{
    return m_instance;
}

void MeshletBenchmark::drawEditor()
{
    ImGui::InputText("Log file name:", m_filename, IM_ARRAYSIZE(m_filename));
    ImGui::InputTextMultiline("Additional notes:", m_notes, IM_ARRAYSIZE(m_notes));


    ImGui::Separator();
    ImGui::Text("Meshes metadata");
    ImGui::Text("Max meshlet primitives: %i", m_maxPrimitives);
    ImGui::Text("Max meshlet vertices: %i", m_maxVertices);
    static int current_item = 0; // Index of the selected item
    const char* items[] = { "MESHOPTIMIZER","DXMESH", "GREEDY", "BoundingSphere", "NVIDIA" }; // Items in the combo box
    ImGui::Text("Meshletizer type: %s", items[static_cast<int>(m_meshletizerType)]);


    ImGui::Separator();
    ImGui::Text("Meshletizing time: %f", m_meshletizingTime);


    static std::vector<std::string> fileNames;
    static std::string inputFileName;
    static bool isLoaded = false;
    if (!isLoaded)
    {
        fileNames.clear();

        for (const auto& entry : std::filesystem::directory_iterator(m_sequencesPath))
        {
            if (entry.is_regular_file()) {
                fileNames.push_back(entry.path().filename().string());
            }
        }

        isLoaded = true;
    }


    // Input box for entering file name
    ImGui::InputText("File Name", m_positionsFilename, IM_ARRAYSIZE(m_positionsFilename));

    // Display file list
    if (ImGui::BeginListBox("File List", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing())))
    {
        for (const auto& fileName : fileNames)
        {
            if (ImGui::Selectable(fileName.c_str())) 
            {
                memset(m_positionsFilename, 0, IM_ARRAYSIZE(m_positionsFilename));
                fileName.copy(m_positionsFilename, fileName.size());
            }
        }
        ImGui::EndListBox();
    }

    if (ImGui::Button("Randomize positions"))
    {
        isLoaded = false;
        generateBenchmarkPositions();
    }
    if (ImGui::Button("Load positions"))
    {
        loadPositionSequenceFromFile();
    }

    ImGui::Separator();

    ImGui::InputInt("Number of frames to schedule:", &m_noOfFrames);
    if (ImGui::Button("Start Recording"))
    {
        m_accumulatedTime = 0;
        run(m_noOfFrames);
    }

    if(m_running)
    {
        ImGui::Text("Running... \%i frames left", m_framesLeft);
    }
    else
    {
        float averageTime = m_accumulatedTime / m_scheduledFrames;
        ImGui::Text("Average model render time: %f\n", averageTime);
        ImGui::Text("Not running...");
    }
    
    static DebugDrawing* innerSphere = nullptr;
    static DebugDrawing* outerSphere = nullptr;
    auto debugDrawer = Renderer::get_instance()->getDebugDrawer();
    if(ImGui::InputFloat("Inner sphere radius", &m_innerRadius) || !innerSphere)
    {
        if (innerSphere)
        {
            debugDrawer->eraseDebugDrawing(innerSphere);
        }
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        visualisers::generateSphereGeometry(m_innerRadius, vertices, indices);
        innerSphere = debugDrawer->registerDebugDrawing(indices, vertices, DebugDrawingType::BENCHMARK_SPHERE);
    }

    if(ImGui::InputFloat("Outer sphere radius", &m_outerRadius) || !outerSphere)
    {
        if (outerSphere)
        {
            debugDrawer->eraseDebugDrawing(outerSphere);
        }
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        visualisers::generateSphereGeometry(m_outerRadius, vertices, indices);
        outerSphere = debugDrawer->registerDebugDrawing(indices, vertices, DebugDrawingType::BENCHMARK_SPHERE);
    }

    static bool showDebugDrawings = false;
    if(ImGui::Checkbox("Show debug drawing", &showDebugDrawings))
    {
        outerSphere->active = showDebugDrawings;
        innerSphere->active = showDebugDrawings;
    }



    if(m_saveNow)
    {
        saveLogToFile();
        m_saveNow = false;
    }
}

void MeshletBenchmark::updateMeshletizerType(MeshletizerType type)
{
    m_meshletizerType = type;
}

void MeshletBenchmark::updateMeshletParameters(uint32_t maxVertices, uint32_t maxPrimitives)
{
    m_maxVertices = maxVertices;
    m_maxPrimitives = maxPrimitives;
}

void MeshletBenchmark::generateBenchmarkPositions()
{
    m_positions.clear();
    m_lookAts.clear();
    std::random_device rd;
    std::mt19937 randomEngine(rd()); // Mersenne Twister engine

    std::uniform_real_distribution<> positionDistribution(-m_outerRadius, m_outerRadius);
    std::uniform_real_distribution<> lookAtDistribution(-m_innerRadius, m_innerRadius);
    for (int i = 0; i < m_noOfFrames; i++)
    {
        hlsl::float3 position = hlsl::float3(
            positionDistribution(randomEngine),
            positionDistribution(randomEngine),
            positionDistribution(randomEngine));
        position = hlsl::normalizeSafe(position)* positionDistribution(randomEngine);


        hlsl::float3 lookAt = hlsl::float3(lookAtDistribution(randomEngine), lookAtDistribution(randomEngine), lookAtDistribution(randomEngine));
        m_lookAts.push_back(lookAt);
        m_positions.push_back(position);
    }
    savePositionSequenceToFile();
}
