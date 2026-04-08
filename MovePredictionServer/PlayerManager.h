#pragma once


#include "RWLock.h"
#include "IdGenerator.h"


class Player;
class RIOSession;


class PlayerManager
{
public:
    explicit PlayerManager( int32_t poolSize = 3000 ) noexcept;
    ~PlayerManager( void ) noexcept;

    PlayerManager( const PlayerManager& ) noexcept = delete;
    PlayerManager&                                  operator=( const PlayerManager& ) noexcept = delete;

    Player*                                         addPlayer( RIOSession* session ) noexcept;
    void                                            removePlayer( uint32_t playerId ) noexcept;
    void                                            removePlayerBySession( RIOSession* session ) noexcept;
    
    Player*                                         getPlayer( int32_t playerId ) noexcept;
    Player*                                         getPlayerBySession( RIOSession* session ) noexcept;

    // 락 상태에서 순회해서 적용
    // 복사해서 락 풀리고 해야할지는 고민
    template< typename Func >
    void                                            forEach( Func&& func ) noexcept
    {
        AutoWriteLocker locker( &_lock );
        for ( auto/*std::pair< int32_t, Player* >*/& activePlayer : _activePlayers)
        {
            func( *activePlayer.second );
        }
    }

    int32_t                                         getPlayerCount( void ) noexcept;

private:
    Player*                                         allocPlayer( void ) noexcept;
    void                                            freePlayer( Player* player ) noexcept;

private:
    RWLock                                          _lock;
    
    // 풀
    std::vector< std::unique_ptr< Player > >        _playerPool;
    std::vector< Player* >                          _freeList;

    // 활성 플레이어
    std::unordered_map< int32_t, Player* >          _activePlayers;
    std::unordered_map< RIOSession*, Player* >      _sessionToPlayer;

    IdGenerator                                     _generator;
};