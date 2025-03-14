#pragma once
#include <string>

class Entity;

class Component
{
public:
    Component();
    virtual ~Component() = default;

    virtual void initialize();
    virtual void uninitialize();
    virtual void reprepare();

    virtual void awake();
    virtual void start();
    virtual void update();
    virtual void on_enabled();
    virtual void on_disabled();
    virtual void on_destroyed();

    void destroy_immediate();

    virtual void drawEditor();

    Entity* entity;

    bool has_been_awaken = false;
    bool has_been_started = false;

    void set_can_tick(bool const value);
    bool get_can_tick() const;

    void set_enabled(bool const value);
    bool enabled() const;

    std::string guid = "";
    std::string custom_name = "";

private:
    bool m_enabled = true;
    bool m_can_tick = false;
};

