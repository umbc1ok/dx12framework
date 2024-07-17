#pragma once

#include <cassert>
#include <Windows.h>

// TODO: Check if it actually stops the program
inline void AssertFailed(HRESULT hr)
{
    assert(FAILED(hr));
}