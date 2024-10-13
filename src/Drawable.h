#pragma once
#include <intsafe.h>

#include "Component.h"
class Drawable :
    public Component
{
public:
    Drawable() = default;
    ~Drawable() = default;

    virtual HRESULT set_CBV() = 0;
    virtual void draw() = 0;
};

