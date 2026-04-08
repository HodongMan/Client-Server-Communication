#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>


#include <stdarg.h>
#include <cstdlib>
#include <cstdint>
#include <cstdio>

#include <cassert>
#include <stdarg.h>


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX    // 있으면 좋음

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#include <process.h>
#include <mstcpip.h>


#pragma comment( lib, "Ws2_32.lib" )
#pragma comment( lib, "Mswsock.lib" )


#include "Logger.h"
#include "../MovePrediction/Assert.h"