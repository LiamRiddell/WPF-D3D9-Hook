#pragma once

// --------------------------------------------------------#
//	Base
// --------------------------------------------------------#
#include <Windows.h>
#include <WinUser.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <dinput.h>
#include <Psapi.h>

// --------------------------------------------------------#
//    Common
// --------------------------------------------------------#
inline void**& getvtable(void* inst, size_t offset = 0)
{
    return *reinterpret_cast<void***>((size_t)inst + offset);
}

inline void** getvtable(const void* inst, size_t offset = 0)
{
    return *reinterpret_cast<void***>((size_t)inst + offset);
}

template<typename Fn>
inline Fn GetVFunc(const void* inst, size_t index, size_t offset = 0)
{
    return reinterpret_cast<Fn>(getvtable(inst, offset)[index]);
}

// --------------------------------------------------------#
//	Minhook
// --------------------------------------------------------#
#include "minhook\include\MinHook.h"

// --------------------------------------------------------#
//	DirectX 9 Includes
// --------------------------------------------------------#
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "d3d9.lib")