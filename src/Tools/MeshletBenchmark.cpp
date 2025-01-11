#include "MeshletBenchmark.h"

#include <filesystem>
#include <fstream>
#include <imgui.h>

#include "Mesh.h"


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
