#pragma once
#include "imgui.h"
#include "Transform.h"
#include "utils/Types.h"
class Entity;



enum class EditorWindowType
{
    Custom,
    Debug,
    Content,
    Game,
    Inspector,
    Hierarchy,
    Profiler
};

struct LockData
{
    Entity* selected_entity;
};

class EditorWindow
{
public:
    EditorWindow(i32& last_id, i32 const flags, EditorWindowType const type) : type(type)
    {
        m_id = last_id + 1;
        last_id = m_id;

        this->flags |= flags;

        switch (type)
        {
        case EditorWindowType::Debug:
            m_name = "Debug##" + std::to_string(m_id);
            break;
        case EditorWindowType::Content:
            m_name = "Content##" + std::to_string(m_id);
            break;
        case EditorWindowType::Hierarchy:
            m_name = "Hierarchy##" + std::to_string(m_id);
            break;
        case EditorWindowType::Game:
            m_name = "Game##" + std::to_string(m_id);
            break;
        case EditorWindowType::Inspector:
            m_name = "Inspector##" + std::to_string(m_id);
            break;
        case EditorWindowType::Profiler:
            m_name = "Profiler##" + std::to_string(m_id);
            break;
        case EditorWindowType::Custom:
            m_name = "Custom##" + std::to_string(m_id);
            break;
        }
    }

    i32 flags = 0;
    EditorWindowType type = EditorWindowType::Custom;

    [[nodiscard]] i32 get_id() const
    {
        return m_id;
    }

    void set_name(std::string const& name)
    {
        m_name = name + "##" + std::to_string(m_id);
    }

    [[nodiscard]] std::string get_name() const
    {
        return m_name;
    }

    void set_is_locked(bool const locked, LockData const& data)
    {
        m_is_locked = locked;

        if (locked)
        {
            m_selected_entity = data.selected_entity;
        }
        else
        {
            m_selected_entity = nullptr;
        }
    }

    [[nodiscard]] bool is_locked() const
    {
        return m_is_locked;
    }

    [[nodiscard]] Entity* get_locked_entity() const
    {
        return m_selected_entity;
    }

private:
    i32 m_id = 0;
    std::string m_name = {};
    bool m_is_locked = false;

    Entity* m_selected_entity = {};
};

enum class AssetType
{
    Unknown,
    Model,
    Scene,
    Prefab,
    Texture,
    Audio
};

class Asset
{
public:
    Asset(std::string const& path, AssetType const type) : path(path), type(type)
    {
    }

    std::string path;
    AssetType type;
};

class Editor
{
public:
    static void create();
    static Editor* get_instance() { return m_instance; }
    Editor();
    ~Editor() = default;
    void update();
    void draw_inspector(EditorWindow* const& window);
    void draw_content_browser(EditorWindow* const& window);
    void draw_debug_window(EditorWindow* const& window);
    void cleanup();
    void remove_window(EditorWindow* const& window);
    void add_inspector();
    void add_game();
    void add_content_browser();
    void add_scene_hierarchy();
    void add_debug_window();
    void add_profiler_window();
    void set_docking_space();


    i32 m_last_window_id = 0;

private:
    void draw_scene_hierarchy(EditorWindow* const& window);
    void draw_entity_recursively(Transform* const& transform);
    void draw_window_menu_bar(EditorWindow* const& window);
    void draw_profiler(EditorWindow* const& window);

    void add_child_entity() const;
    void copy_selected_entity() const;
    void delete_selected_entity() const;
    bool draw_entity_popup(Entity* const& entity);

    std::vector<Asset> m_assets = {};
    std::vector<EditorWindow*> m_editor_windows = {};
    Entity* m_selected_entity = nullptr;
    bool m_rendering_to_editor = false;
    bool m_polygon_mode_active = false;
    int m_frame_count = 0;
    double m_current_time = 0.0;
    double m_last_second = 0.0;
    double m_average_ms_per_frame = 0.0;
    static Editor* m_instance;

    int m_current_debug_mode = 1;
    std::string m_search_filter = {};
};
