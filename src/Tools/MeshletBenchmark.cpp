#include "MeshletBenchmark.h"

#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <random>

#include "Camera.h"
#include "Mesh.h"
#include "Renderer.h"
#include "debugGeometry/DebugDrawer.h"
#include "debugGeometry/VisualiserGeometry.h"


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
        file << "Meshletizer type: " << m_meshletizerType << "\n";
        file << "Max vertices: " << m_maxVertices << "\n";
        file << "Max primitives: " << m_maxPrimitives << "\n";
        file << "Meshletizing time: " << m_meshletizingTime << "\n";
        file << "Frames: " << m_scheduledFrames << "\n";
        file << "Average render time: " << m_accumulatedTime / m_scheduledFrames << "\n";
        file << "Notes: " << m_notes << "\n";
        file.close();
        return true;
    }

    printf("Could not open file %s\n", path.c_str());
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
    const char* items[] = { "DXMESH", "MESHOPTIMIZER", "GREEDY", "BoundingSphere" }; // Items in the combo box
    if (ImGui::BeginCombo("Algorithm", items[current_item]))
    {
        for (int i = 0; i < IM_ARRAYSIZE(items); i++)
        {
            bool is_selected = (current_item == i);
            if (ImGui::Selectable(items[i], is_selected))
            {
                current_item = i; // Update the selected item
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus(); // Ensure the selected item is focused
            }
        }
        ImGui::EndCombo();
    }


    ImGui::Separator();
    ImGui::Text("Meshletizing time: %f", m_meshletizingTime);

    if (ImGui::Button("Randomize positions"))
    {
        generateBenchmarkPositions();
    }
    if (ImGui::Button("Start Recording"))
    {
        m_accumulatedTime = 0;
        run(10000);
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

    std::uniform_real_distribution<> positionDistribution(m_innerRadius, m_outerRadius);
    std::bernoulli_distribution randomBool(0.5);
    std::uniform_real_distribution<> lookAtDistribution(-m_innerRadius, m_innerRadius);
    for (int i = 0; i < 10000; i++)
    {
        hlsl::float3 position = hlsl::float3(
            positionDistribution(randomEngine) * randomBool(randomEngine) * (-1.0f),
            positionDistribution(randomEngine) * randomBool(randomEngine) * (-1.0f),
            positionDistribution(randomEngine) * randomBool(randomEngine) * (-1.0f));

        hlsl::float3 lookAt = hlsl::float3(lookAtDistribution(randomEngine), lookAtDistribution(randomEngine), lookAtDistribution(randomEngine));
        m_lookAts.push_back(lookAt);
        m_positions.push_back(position);
    }
}
