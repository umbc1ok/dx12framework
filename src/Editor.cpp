#include "Editor.h"

#include <algorithm>
#include <ctime>
#include <__msvc_filebuf.hpp>
#include <imgui_impl/imgui_impl_dx12.h>
#include <imgui_impl/imgui_impl_win32.h>
#include <imgui_internal.h>
#include "Entity.h"
#include "Window.h"

Editor* Editor::m_instance;

void Editor::create()
{
    m_instance = new Editor();
    m_instance->add_scene_hierarchy();
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
    auto window_instance = Window::get_instance();
    ImGui_ImplWin32_Init(window_instance->get_hwnd());
    ImGui_ImplDX12_Init(window_instance->get_device(), Window::NUM_FRAMES_IN_FLIGHT,
        DXGI_FORMAT_R8G8B8A8_UNORM, window_instance->get_srv_desc_heap(),
        window_instance->get_srv_desc_heap()->GetCPUDescriptorHandleForHeapStart(),
        window_instance->get_srv_desc_heap()->GetGPUDescriptorHandleForHeapStart());
}

void Editor::update()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& m_io = ImGui::GetIO();

    auto const windows_copy = m_editor_windows;
    for (auto& window : windows_copy)
    {
        switch (window->type)
        {
        case EditorWindowType::Debug:
            draw_debug_window(window);
            break;
        case EditorWindowType::Content:
            draw_content_browser(window);
            break;
        case EditorWindowType::Hierarchy:
            draw_scene_hierarchy(window);
            break;
        case EditorWindowType::Game:
            // TODO: Rendering to texture currently not supported
            //draw_game(window);
            break;
        case EditorWindowType::Inspector:
            draw_inspector(window);
            break;
        case EditorWindowType::Custom:
            printf("Custom Editor windows are currently not supported.\n");
            break;
        }
    }

    // Rendering
    ImGui::Render();
    auto cmd_list = Window::get_instance()->get_cmd_list();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list);

    // Update and Render additional Platform Windows
    if (m_io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void*)cmd_list);
    }
}

void Editor::draw_inspector(EditorWindow* const& window)
{
    bool is_still_open = true;
    bool const open = ImGui::Begin(window->get_name().c_str(), &is_still_open, window->flags);

    if (!is_still_open)
    {
        remove_window(window);
        ImGui::End();
        return;
    }

    if (!open)
    {
        ImGui::End();
        return;
    }

    draw_window_menu_bar(window);

    if (window->is_locked() && window->get_locked_entity() == nullptr)
    {
        window->set_is_locked(false, {});
    }

    Entity* current_entity = window->get_locked_entity();

    if (current_entity == nullptr)
    {
        current_entity = m_selected_entity;
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

    // TODO: Implement transforms and overall math

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

            component->draw_editor();

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
        ImGui::InputText("##filter", m_search_filter.data(), 100);
        ImGui::PopStyleVar();
        ImGui::PopItemWidth();

        std::ranges::transform(m_search_filter, m_search_filter.begin(), [](char const c) { return std::tolower(c); });

#define CONCAT_CLASS(name) class name
#define ENUMERATE_COMPONENT(name, ui_name)                                                                        \
    {                                                                                                             \
        std::string ui_name_lower(ui_name);                                                                       \
        std::ranges::transform(ui_name_lower, ui_name_lower.begin(), [](u8 const c) { return std::tolower(c); }); \
        if (m_search_filter.empty() || ui_name_lower.find(m_search_filter) != std::string::npos)                  \
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

void Editor::draw_content_browser(EditorWindow* const& window)
{
    bool is_still_open = true;
    bool const open = ImGui::Begin(window->get_name().c_str(), &is_still_open, window->flags);

    if (!is_still_open)
    {
        remove_window(window);
        ImGui::End();
        return;
    }

    if (!open)
    {
        ImGui::End();
        return;
    }

    draw_window_menu_bar(window);

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
void Editor::draw_debug_window(EditorWindow* const& window)
{
    m_current_time = clock();
    m_frame_count += 1;

    if (m_current_time - m_last_second >= 1.0)
    {
        m_average_ms_per_frame = 1000.0 / static_cast<double>(m_frame_count);
        m_frame_count = 0;
        m_last_second = clock();
    }

    bool is_still_open = true;
    bool const open = ImGui::Begin(window->get_name().c_str(), &is_still_open, window->flags);

    if (!is_still_open)
    {
        remove_window(window);
        ImGui::End();
        return;
    }

    if (!open)
    {
        ImGui::End();
        return;
    }

    draw_window_menu_bar(window);

    ImGui::Checkbox("Polygon mode", &m_polygon_mode_active);
    ImGui::SameLine();
    ImGui::Text("Application average %.3f ms/frame", m_average_ms_per_frame);

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

    //Renderer::get_instance()->wireframe_mode_active = m_polygon_mode_active;
}

void Editor::cleanup()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Editor::remove_window(EditorWindow* const& window)
{
    auto const it = std::ranges::find(m_editor_windows, window);

    if (it != m_editor_windows.end())
    {
        delete window;
        m_editor_windows.erase(it);
    }
}

void Editor::add_inspector()
{
    auto inspector_window = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::Inspector);
    m_editor_windows.emplace_back(inspector_window);
}

void Editor::add_game()
{
    auto game_window = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::Game);
    m_editor_windows.emplace_back(game_window);
}

void Editor::add_content_browser()
{
    auto content_browser_window = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::Content);
    m_editor_windows.emplace_back(content_browser_window);
}

void Editor::add_scene_hierarchy()
{
    auto hierarchy_window = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::Hierarchy);
    m_editor_windows.emplace_back(hierarchy_window);
}

void Editor::add_debug_window()
{
    auto debug_window = new EditorWindow(m_last_window_id, ImGuiWindowFlags_MenuBar, EditorWindowType::Debug);
    m_editor_windows.emplace_back(debug_window);
}

void Editor::draw_window_menu_bar(EditorWindow* const& window)
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Menu"))
        {
            if (ImGui::BeginMenu("Add window"))
            {
                if (ImGui::MenuItem("Inspector"))
                {
                    add_inspector();
                }
                if (ImGui::MenuItem("Game"))
                {
                    add_game();
                }
                if (ImGui::MenuItem("Content"))
                {
                    add_content_browser();
                }
                if (ImGui::MenuItem("Hierarchy"))
                {
                    add_scene_hierarchy();
                }
                if (ImGui::MenuItem("Debug"))
                {
                    add_debug_window();
                }

                ImGui::EndMenu();
            }

            if (window->type == EditorWindowType::Inspector)
            {
                if (ImGui::Button("Lock"))
                {
                    window->set_is_locked(!window->is_locked(), LockData{ m_selected_entity });
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


void Editor::add_child_entity() const
{
    if (m_selected_entity == nullptr)
        return;

    auto const entity = m_selected_entity;
    auto const child_entity = Entity::create("Child");
    child_entity->transform->set_parent(entity->transform);
}

void Editor::delete_selected_entity() const
{
    if (m_selected_entity != nullptr)
    {
        m_selected_entity->destroy_immediate();
    }
}

bool Editor::draw_entity_popup(Entity* const& entity)
{
    if (m_selected_entity != nullptr && ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
    {
        if (m_selected_entity != entity)
        {
            m_selected_entity = entity;
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
            delete_selected_entity();
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
            add_child_entity();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return true;
        }


        ImGui::EndPopup();
    }

    return true;
}

void Editor::draw_scene_hierarchy(EditorWindow* const& window)
{
    bool is_still_open = true;
    bool const open = ImGui::Begin(window->get_name().c_str(), &is_still_open, window->flags);

    if (!is_still_open)
    {
        remove_window(window);
        ImGui::End();
        return;
    }

    if (!open)
    {
        ImGui::End();
        return;
    }

    draw_window_menu_bar(window);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::Button("Add Entity"))
        {
            Entity::create("Entity");
        }

        ImGui::EndMenuBar();
    }

    // Draw every entity without a parent, and draw its children recursively
    auto const entities_copy = MainScene::get_instance()->entities;
    for (auto const& entity : entities_copy)
    {
        if (entity->transform->parent != nullptr)
            continue;

        draw_entity_recursively(entity->transform);
    }

    ImGui::End();
}

void Editor::draw_entity_recursively(Transform* const& transform)
{
    if (transform == nullptr || transform->entity == nullptr)
        return;

    auto const entity = transform->entity;
    ImGuiTreeNodeFlags const node_flags =
        (m_selected_entity != nullptr && m_selected_entity->hashed_guid == entity->hashed_guid ? ImGuiTreeNodeFlags_Selected : 0)
        | (transform->children.empty() ? ImGuiTreeNodeFlags_Leaf : 0) | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_OpenOnArrow;

    if (!ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(entity->hashed_guid)), node_flags, "%s",
        entity->name.c_str()))
    {
        if (ImGui::IsItemClicked() || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_selected_entity = entity;
        }

        if (!draw_entity_popup(entity))
        {
            return;
        }

        return;
    }


    if (ImGui::IsItemClicked() || ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        m_selected_entity = entity;
    }

    if (!draw_entity_popup(entity))
    {
        ImGui::TreePop();
        return;
    }

    for (auto const& child : transform->children)
    {
        draw_entity_recursively(child);
    }

    ImGui::TreePop();
}