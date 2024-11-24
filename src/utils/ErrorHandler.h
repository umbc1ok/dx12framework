#pragma once

#include <cassert>
#include <Windows.h>

#include "Renderer.h"

inline void AssertFailed(HRESULT hr)
{
    if (FAILED(hr) == true)
    {
        printf("Error: %ld\n", hr);
        throw std::exception("some shit, check in debugger");
    }
}