#pragma once


#define _USE_MATH_DEFINES

#include <memory>
#include <vector>
#include <algorithm>
#include <queue>
#include <iostream>
#include <unordered_map>

#include <cstdlib>  // 이거 먼저
#include <stdarg.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <cassert>

#define _USE_MATH_DEFINES
#include <cmath>


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <winsock2.h>
#include <ws2tcpip.h>

// 그 다음 Windows
#include <mmsystem.h>
#include <windows.h>
#include <process.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <Vulkan/Vulkan.h>

#pragma comment( lib, "Ws2_32.lib" )
#pragma comment( lib, "winmm.lib" )

#include "../MovePrediction/Assert.h"
