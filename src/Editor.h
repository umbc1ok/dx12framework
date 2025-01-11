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
    Profiler,
    MeshletBenchmark
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
        case EditorWindowType::MeshletBenchmark:
            m_name = "MeshletBenchmark##" + std::to_string(m_id);
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
    void drawInspector(EditorWindow* const& window);
    void drawContentBrowser(EditorWindow* const& window);
    void drawDebugWindow(EditorWindow* const& window);
    void cleanup();
    void removeWindow(EditorWindow* const& window);
    void addInspector();
    void addGame();
    void addContentBrowser();
    void addSceneHierarchy();
    void addDebugWindow();
    void addProfilerWindow();
    void addMeshletBenchmarkWindow();
    void setDockingSpace();


    i32 m_last_window_id = 0;

private:
    void drawSceneHierarchy(EditorWindow* const& window);
    void drawEntityRecursively(Transform* const& transform);
    void drawWindowMenuBar(EditorWindow* const& window);
    void drawProfiler(EditorWindow* const& window);
    void drawMeshletBenchmark(EditorWindow* const& window);

    void addChildEntity() const;
    // todo: implement
    void copySelectedEntity() const;
    void deleteSelectedEntity() const;
    bool drawEntityPopup(Entity* const& entity);

    std::vector<Asset> m_assets = {};
    std::vector<EditorWindow*> m_editorWindows = {};
    Entity* m_selectedEntity = nullptr;
    bool m_renderingToEditor = false;
    bool m_polygonModeActive = false;
    int m_frameCount = 0;
    double m_currentTime = 0.0;
    double m_lastSecond = 0.0;
    double m_averageMsPerFrame = 0.0;
    static Editor* m_instance;

    int m_currentDebugMode = 1;
    std::string m_searchFilter = {};
};
