#pragma once


#include <cstdint>
#include <atomic>
#include <chrono>


// thread safe version
class IdGenerator
{
public:
    int32_t                                         generate() noexcept
    {
        // 상위 16비트: 서버 시작 후 경과 초
        // 하위 16비트: 카운터
        std::chrono::steady_clock::time_point now   = std::chrono::steady_clock::now();
        const int64_t elapsed                       = std::chrono::duration_cast< std::chrono::seconds >( now - _startTime ).count();
        const int32_t elapsedSec                    = static_cast<int64_t>( elapsed );
        const int32_t count                         = _counter.fetch_add( 1, std::memory_order_relaxed );

        return ( elapsedSec << 16 ) | count;
    }

private:
    // 나중에 받아서 써야 합니다
    // 서버 실행 시간을
    const std::chrono::steady_clock::time_point     _startTime              = std::chrono::steady_clock::now();
    std::atomic< int32_t >                          _counter{ 0 };
};