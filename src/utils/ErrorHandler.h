#pragma once

#include <cassert>
#include <Windows.h>

// TODO: Check if it actually stops the program
inline void AssertFailed(HRESULT hr)
{
    if (FAILED(hr) == true)
    {
        printf("Error: %d\n", hr);
    }
}