#pragma once

#include <cassert>
#include <Windows.h>

#include "Renderer.h"

// TODO: Check if it actually stops the program
inline void AssertFailed(HRESULT hr)
{
    if (FAILED(hr) == true)
    {
        printf("Error: %ld\n", hr);
        FAILED(Renderer::get_instance()->get_device()->GetDeviceRemovedReason()) == true;
    }
}