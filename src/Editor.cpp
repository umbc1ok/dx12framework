#include "Editor.h"

#include <algorithm>
#include <ctime>
#include <__msvc_filebuf.hpp>
#include <imgui_impl/imgui_impl_dx12.h>
#include <imgui_impl/imgui_impl_win32.h>
#include <imgui_internal.h>
#include "Entity.h"
#include "Keyboard.h"
#include "Renderer.h"
#include "Window.h"
#include "Tools/GPUProfiler.h"
#include "Tools/MeshletBenchmark.h"

Editor* Editor::m_instance;

void Editor::create()
{
    m_instance = new Editor();
    m_instance->addSceneHierarchy();
    m_instance->addInspector();
    m_instance->addDebugWindow();
    m_instance->addProfilerWindow();
    m_instance->addMeshletBenchmarkWindow();
}

Editor::Editor()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    auto renderer = Renderer::get_instance();
    auto window_instance = Window::get_instance();
    ImGui_ImplWin32_Init(window_instance->get_hwnd());
    ImGui_ImplDX12_Init(renderer->get_device(), Renderer::NUM_FRAMES_IN_FLIGHT,
        DXGI_FORMAT_R8G8B8A8_UNORM, renderer->get_srv_desc_heap(),
        renderer->get_srv_desc_heap()->GetCPUDescriptorHandleForHeapStart(),
        renderer->get_srv_desc_heap()->GetGPUDescriptorHandleForHeapStart());
}

void Editor::update()
{
    m_currentTime = ImGui::GetTime();
    m_frameCount += 1;

    if (m_currentTime - m_lastSecond >= 1.0)
    {
        m_averageMsPerFrame = 1000.0 / static_cast<double>(m_frameCount);
        m_frameCount = 0;
        m_lastSecond = ImGui::GetTime();
    }
    Window::get_instance()->update_window_name("DX12Framework by Hubert Olejnik @" + std::to_string(1000.0f / m_averageMsPerFrame) + "fps");

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    setDockingSpace();
    ImGuiIO& m_io = ImGui::GetIO();
    static bool show = true;
    if(ImGui::IsKeyPressed(ImGuiKey_F1))
    {
        show = !show;
    }
    if (show)
    {
            
        auto const windows_copy = m_editorWindows;
        for (auto& window : windows_copy)
        {
            switch (window->type)
            {
            case EditorWindowType::Debug:
                drawDebugWindow(window);
                    break;
            case EditorWindowType::Content:
                drawContentBrowser(window);
                break;
            case EditorWindowType::Hierarchy:
                drawSceneHierarchy(window);
                break;
            case EditorWindowType::Game:
                // TODO: Rendering to texture currently not supported
                //draw_game(window);
                break;
            case EditorWindowType::Inspector:
                drawInspector(window);
                break;
            case EditorWindowType::Profiler:
                drawProfiler(window);
                break;
            case EditorWindowType::MeshletBenchmark:
                drawMeshletBenchmark(window);
                break;
            case EditorWindowType::Custom:
                printf("Custom Editor windows are currently not supported.\n");
                break;
            }
        }
    }
    ImGui::Render();
    auto cmdlist = Renderer::get_instance()->g_pd3dCommandList;
    auto hp = Renderer::get_instance()->get_srv_desc_heap();
    cmdlist->SetDescriptorHeaps(1, &hp);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdlist);
    // Update and Render additional Platform Windows
    if (m_io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void*)cmdlist);
    }
    
}

void Editor::drawInspector(EditorWindow* const& window)
{
    bool is_still_open = true;
    bool const open = ImGui::Begin(window->get_name().c_str(), &is_still_open, window->flags);

    if (!is_still_open)
    {
        removeWindow(window);
        ImGui::End();
        return;
    }

    if (!open)
    {
        ImGui::End();
        return;
    }

    drawWindowMenuBar(window);

    if (window->is_locked() && window->get_locked_entity() == nullptr)
    {
        window->set_is_locked(false, {});
    }

    Entity* current_entity = window->get_locked_entity();

    if (current_entity == nullptr)
    {
        current_entity = m_selectedEntity;
    }

    if (current_entity == nullptr)
    {
        ImGui::End();
        return;
    }

    //auto const camera = Camera::get_main_camera();
    auto const entity = current_entity;

    ImGui::Text("Transform");
    ImGui::Spacing();

    hlsl::float3 position = entity->transform->get_local_position();
    ImGui::InputFloat3("Position", position.data);

    float const input_width = ImGui::CalcItemWidth() / 3.0f - ImGui::GetStyle().ItemSpacing.x * 0.66f;

    ImGui::SameLine();
    ImGui::Text(" | ");
    //ImGui::SameLine();

    // TODO: Implement copy position/rotation/scale to clipboard
    /*
    if (ImGui::Button("Copy##1"))
    {
        std::string const cpy = glm::to_string(position);
        ImGui::SetClipboardText(cpy.c_str());
    }
    */
    entity->transform->set_local_position(position);

    hlsl::float3 rotation = entity->transform->get_euler_angles();
    ImGui::InputFloat3("Rotation", rotation.data);
    entity->transform->set_local_euler_angles(rotation);

    ImGui::SameLine();
    ImGui::Text(" | ");
    //ImGui::SameLine();

    // TODO: Implement copy position/rotation/scale to clipboard
    /*
    if (ImGui::Button("Copy##2"))
    {
        std::string const cpy = glm::to_string(rotation);
        ImGui::SetClipboardText(cpy.c_str());
    }
    */
    hlsl::float3 scale = entity->transform->get_local_scale();
    hlsl::float3 old_scale = scale;

    ImGui::PushItemWidth(input_width);

    //ImGui::BeginDisabled(m_lock_scale && m_disabled_scale.x);
    ImGui::InputFloat("##x", &scale.x);
    //ImGui::EndDisabled();

    ImGui::SameLine();

    //ImGui::BeginDisabled(m_lock_scale && m_disabled_scale.y);
    ImGui::InputFloat("##y", &scale.y);
    //ImGui::EndDisabled();

    ImGui::SameLine();

    //ImGui::BeginDisabled(m_lock_scale && m_disabled_scale.z);
    ImGui::InputFloat("Scale##z", &scale.z);
    //ImGui::EndDisabled();

    ImGui::PopItemWidth();

    /*
    if (scale != old_scale && m_lock_scale)
    {
        scale = update_locked_value(scale, old_scale);
    }
    */
    entity->transform->set_local_scale(scale);

    ImGui::SameLine();
    ImGui::Text("    | ");
    ImGui::SameLine();

    // TODO: Implement copy position/rotation/scale to clipboard
    /*
    if (ImGui::Button("Copy##3"))
    {
        std::string const cpy = glm::to_string(scale);
        ImGui::SetClipboardText(cpy.c_str());
    }
    */
    //ImGui::SameLine();

    // TODO: Implement scale locking
    /*
    if (ImGui::Checkbox("LOCK", &m_lock_scale))
    {
        if (!m_lock_scale)
        {
            m_disabled_scale = { false, false, false };
        }
        else
        {
            m_lock_ratio = entity->transform->get_local_scale();

            m_disabled_scale.x = glm::epsilonEqual(m_lock_ratio.x, 0.0f, 0.0001f);

            m_disabled_scale.y = glm::epsilonEqual(m_lock_ratio.y, 0.0f, 0.0001f);

            m_disabled_scale.z = glm::epsilonEqual(m_lock_ratio.z, 0.0f, 0.0001f);
        }
    }
    */
    auto const components_copy = entity->components;
    for (auto const& component : components_copy)
    {
        ImGui::Spacing();
        std::string guid = "##" + component->guid;

        // NOTE: This only returns unmangled name while using the MSVC compiler
        std::string const typeid_name = typeid(*component).name();
        std::string const name = typeid_name.substr(6) + " " + component->custom_name;

        bool const component_open = ImGui::TreeNode((name + guid).c_str());

        ImGuiDragDropFlags src_flags = 0;
        src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;

        if (ImGui::BeginDragDropSource(src_flags))
        {
            ImGui::Text((entity->name + " : " + name).c_str());
            ImGui::SetDragDropPayload("guid", component->guid.data(), sizeof(i32) * 8);
            ImGui::EndDragDropSource();
        }

        ImGui::Spacing();

        if (component_open)
        {
            bool enabled = component->enabled();
            ImGui::Checkbox("Enabled", &enabled);
            component->set_enabled(enabled);

            component->drawEditor();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TreePop();
        }
    }
    // TODO: Implement search bar
    /*
    if (ImGui::BeginListBox("##empty", ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        ImGui::Text("Search bar");

        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 5));
        ImGui::InputText("##filter", m_searchFilter.data(), 100);
        ImGui::PopStyleVar();
        ImGui::PopItemWidth();

        std::ranges::transform(m_searchFilter, m_searchFilter.begin(), [](char const c) { return std::tolower(c); });

#define CONCAT_CLASS(name) class name
#define ENUMERATE_COMPONENT(name, ui_name)                                                                        \
    {                                                                                                             \
        std::string ui_name_lower(ui_name);                                                                       \
        std::ranges::transform(ui_name_lower, ui_name_lower.begin(), [](u8 const c) { return std::tolower(c); }); \
        if (m_searchFilter.empty() || ui_name_lower.find(m_searchFilter) != std::string::npos)                  \
        {                                                                                                         \
            if (ImGui::Button(ui_name, ImVec2(-FLT_MIN, 20)))                                                     \
                entity->add_component<CONCAT_CLASS(name)>(##name::create());                                      \
        }                                                                                                         \
    }
        ENUMERATE_COMPONENTS
#undef ENUMERATE_COMPONENT

            ImGui::EndListBox();
    }
    */
    ImGui::End();
}

void Editor::drawContentBrowser(EditorWindow* const& window)
{
    bool is_still_open = true;
    bool const open = ImGui::Begin(window->get_name().c_str(), &is_still_open, window->flags);

    if (!is_still_open)
    {
        removeWindow(window);
        ImGui::End();
        return;
    }

    if (!open)
    {
        ImGui::End();
        return;
    }

    drawWindowMenuBar(window);

    // TODO: Implement scene saving
    //draw_scene_save();

    if (ImGui::CollapsingHeader("Models"))
    {
        for (auto const& asset : m_assets)
        {
            if (asset.type != AssetType::Scene && asset.type != AssetType::Prefab && ImGui::Selectable(asset.path.c_str()))
            {
                ImGui::SetClipboardText(asset.path.c_str());
            }
        }
    }

    if (ImGui::CollapsingHeader("Textures"))
    {
        for (auto const& asset : m_assets)
        {
            if (asset.type == AssetType::Texture && ImGui::Selectable(asset.path.c_str()))
            {
                ImGui::SetClipboardText(asset.path.c_str());
            }
        }
    }

    if (ImGui::CollapsingHeader("Sounds"))
    {
        for (auto const& asset : m_assets)
        {
            if (asset.type == AssetType::Audio && ImGui::Selectable(asset.path.c_str()))
            {
                ImGui::SetClipboardText(asset.path.c_str());
            }
        }
    }

    bool const ctrl_pressed = ImGui::GetIO().KeyCtrl;

    // TODO: Implement scene saving
    /*
    if (ImGui::CollapsingHeader("Scenes"))
    {
        for (auto const& asset : m_assets)
        {
            if (asset.type == AssetType::Scene && ImGui::Selectable(asset.path.c_str()))
            {
                std::filesystem::path file_path(asset.path);
                std::string const filename = file_path.stem().string();

                if (!m_append_scene && !ctrl_pressed)
                {
                    MainScene::get_instance()->unload();
                }

                bool const loaded = load_scene_name(filename);

                if (!loaded)
                {
                    Debug::log("Could not load a scene.", DebugType::Error);
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Prefabs"))
    {
        for (auto const& asset : m_assets)
        {
            if (asset.type == AssetType::Prefab && ImGui::Selectable(asset.path.c_str()))
            {
                std::filesystem::path file_path(asset.path);
                std::string const filename = file_path.stem().string();

                if (!m_append_scene && !ctrl_pressed)
                {
                    MainScene::get_instance()->unload();
                }

                bool const loaded = load_prefab(filename);

                if (!loaded)
                {
                    Debug::log("Could not load a prefab.", DebugType::Error);
                }
            }
        }
    }
    */
    ImGui::End();
}
void Editor::drawDebugWindow(EditorWindow* const& window)
{
    bool is_still_open = true;
    bool const open = ImGui::Begin(window->get_name().c_str(), &is_still_open, window->flags);

    if (!is_still_open)
    {
        removeWindow(window);
        ImGui::End();
        return;
    }

    if (!open)
    {
        ImGui::End();
        return;
    }

    drawWindowMenuBar(window);

    ImGui::Text("Application average %.3f ms/frame", m_averageMsPerFrame);
    if(ImGui::Checkbox("Polygon mode", &m_polygonModeActive))
    {
        Renderer::get_instance()->set_wireframe(m_polygonModeActive);
    }
    const char* items[] = { "NONE", "MESHLET VIEW", "TRIANGLE VIEW" };
    {
        ImGui::Text("Wybierz opcje:");

        if (ImGui::Combo("MESHLET DEBUG MODE", &m_currentDebugMode, items, IM_ARRAYSIZE(items)))
        {
            Renderer::get_instance()->set_debug_mode(m_currentDebugMode);
        }
    }
    static bool vsync = true;
    if(ImGui::Checkbox("VSYNC:", &vsync))
    {
        Renderer::get_instance()->set_vsync(vsync);
    }

    Renderer::get_instance()->getDebugDrawer()->drawEditor();
    // TODO: Implement scene saving
    //draw_scene_save();

    // TODO: Implement logging
    /*
    std::string const log_count = "Logs " + std::to_string(Debug::debug_messages.size());
    ImGui::Text(log_count.c_str());
    if (ImGui::Button("Clear log"))
    {
        Debug::clear();
    }
    if (ImGui::BeginListBox("Logs", ImVec2(-FLT_MIN, 0.0f)))
    {
        ImGuiListClipper clipper;
        clipper.Begin(Debug::debug_messages.size());
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                switch (Debug::debug_messages[i].type)
                {
                case DebugType::Log:
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                    break;
                case DebugType::Warning:
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 250, 0, 255));
                    break;
                case DebugType::Error:
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 55, 0, 255));
                    break;
                }

                ImGui::Text(Debug::debug_messages[i].text.c_str());
                ImGui::PopStyleColor();
            }
        }

        if (m_always_newest_logs)
        {
            ImGui::SetScrollHereY(1.0f); // Scroll to bottom so the latest logs are always shown
        }

        ImGui::EndListBox();
    }
    */

    ImGui::End();

    //Renderer::get_instance()->wireframe_mode_active = m_polygonModeActive;
}

void Editor::cleanup()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Editor::removeWindow(EditorWindow* const& window)
{
    auto const it = std::ranges::find(m_editorWindows, window);

    if (it != m_editorWindows.end())
    {
        delete window;
        m_editorWindows.erase(it);
    }
}

void Editor::addInspector()
{
    auto inspector_window = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::Inspector);
    m_editorWindows.emplace_back(inspector_window);
}

void Editor::addGame()
{
    auto game_window = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::Game);
    m_editorWindows.emplace_back(game_window);
}

void Editor::addContentBrowser()
{
    auto content_browser_window = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::Content);
    m_editorWindows.emplace_back(content_browser_window);
}

void Editor::addSceneHierarchy()
{
    auto hierarchy_window = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::Hierarchy);
    m_editorWindows.emplace_back(hierarchy_window);
}

void Editor::addDebugWindow()
{
    auto debug_window = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::Debug);
    m_editorWindows.emplace_back(debug_window);
}

void Editor::addProfilerWindow()
{
    auto profiler_window = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::Profiler);
    m_editorWindows.emplace_back(profiler_window);
}

void Editor::addMeshletBenchmarkWindow()
{
    auto benchmarkWindow = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::MeshletBenchmark);
    m_editorWindows.emplace_back(benchmarkWindow);
}

void Editor::setDockingSpace()
{
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}

void Editor::drawWindowMenuBar(EditorWindow* const& window)
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Menu"))
        {
            if (ImGui::BeginMenu("Add window"))
            {
                if (ImGui::MenuItem("Inspector"))
                {
                    addInspector();
                }
                if (ImGui::MenuItem("Game"))
                {
                    addGame();
                }
                if (ImGui::MenuItem("Content"))
                {
                    addContentBrowser();
                }
                if (ImGui::MenuItem("Hierarchy"))
                {
                    addSceneHierarchy();
                }
                if (ImGui::MenuItem("Debug"))
                {
                    addDebugWindow();
                }
                if (ImGui::MenuItem("Profiler"))
                {
                    addProfilerWindow();
                }
                ImGui::EndMenu();
            }

            if (window->type == EditorWindowType::Inspector)
            {
                if (ImGui::Button("Lock"))
                {
                    window->set_is_locked(!window->is_locked(), LockData{ m_selectedEntity });
                }
            }

            ImGui::EndMenu();
        }

        if (window->is_locked())
        {
            ImGui::Text("LOCKED");
        }

        ImGui::EndMenuBar();
    }
}

// This should be implemented inside the profiler, and this function should just call the profiler's function
void Editor::drawProfiler(EditorWindow* const& window)  
{
    GPUProfiler::getInstance()->drawEditor(window);
}

void Editor::drawMeshletBenchmark(EditorWindow* const& window)
{
    bool is_still_open = true;
    ImGui::Begin(window->get_name().c_str(), &is_still_open, window->flags);
    //MeshletBenchmark::getInstance()->drawEditor();
    ImGui::End();
}


void Editor::addChildEntity() const
{
    if (m_selectedEntity == nullptr)
        return;

    auto const entity = m_selectedEntity;
    auto const child_entity = Entity::create("Child");
    child_entity->transform->set_parent(entity->transform);
}

void Editor::deleteSelectedEntity() const
{
    if (m_selectedEntity != nullptr)
    {
        m_selectedEntity->destroy_immediate();
    }
}

bool Editor::drawEntityPopup(Entity* const& entity)
{
    if (m_selectedEntity != nullptr && ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
    {
        if (m_selectedEntity != entity)
        {
            m_selectedEntity = entity;
        }

        if (ImGui::Button("Rename"))
        {
            ImGui::OpenPopup("RenamePopup");
        }

        if (ImGui::BeginPopup("RenamePopup"))
        {
            if (ImGui::InputText("##empty", entity->name.data(), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (ImGui::Button("Delete"))
        {
            deleteSelectedEntity();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return false;
        }

        // TODO: Implement serializing
        /*
        if (ImGui::Button("Copy"))
        {
            copy_selected_entity();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return true;
        }

        if (ImGui::Button("Duplicate"))
        {
            copy_selected_entity();
            paste_entity();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return true;
        }

        if (ImGui::Button("Save as prefab"))
        {
            save_entity_as_prefab();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return true;
        }
        */
        if (ImGui::Button("Add child"))
        {
            addChildEntity();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return true;
        }


        ImGui::EndPopup();
    }

    return true;
}

void Editor::drawSceneHierarchy(EditorWindow* const& window)
{
    bool is_still_open = true;
    bool const open = ImGui::Begin(window->get_name().c_str(), &is_still_open, window->flags);

    if (!is_still_open)
    {
        removeWindow(window);
        ImGui::End();
        return;
    }

    if (!open)
    {
        ImGui::End();
        return;
    }

    drawWindowMenuBar(window);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::Button("Add Entity"))
        {
            Entity::create("Entity");
        }

        ImGui::EndMenuBar();
    }

    // Draw every entity without a parent, and draw its children recursively
    auto const entities_copy = MainScene::get_instance()->m_entities;
    for (auto const& entity : entities_copy)
    {
        if (entity->transform->parent != nullptr)
            continue;

        drawEntityRecursively(entity->transform);
    }

    ImGui::End();
}

void Editor::drawEntityRecursively(Transform* const& transform)
{
    if (transform == nullptr || transform->entity == nullptr)
        return;

    auto const entity = transform->entity;
    ImGuiTreeNodeFlags const node_flags =
        (m_selectedEntity != nullptr && m_selectedEntity->hashed_guid == entity->hashed_guid ? ImGuiTreeNodeFlags_Selected : 0)
        | (transform->children.empty() ? ImGuiTreeNodeFlags_Leaf : 0) | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_OpenOnArrow;

    if (!ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(entity->hashed_guid)), node_flags, "%s",
        entity->name.c_str()))
    {
        if (ImGui::IsItemClicked() || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_selectedEntity = entity;
        }

        if (!drawEntityPopup(entity))
        {
            return;
        }

        return;
    }


    if (ImGui::IsItemClicked() || ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        m_selectedEntity = entity;
    }

    if (!drawEntityPopup(entity))
    {
        ImGui::TreePop();
        return;
    }

    for (auto const& child : transform->children)
    {
        drawEntityRecursively(child);
    }

    ImGui::TreePop();
}