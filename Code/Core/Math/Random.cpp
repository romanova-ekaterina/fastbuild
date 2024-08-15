// Random
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Random.h"

#include <time.h>
#include <stdio.h>
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
Random::Random()
{
    static uint32_t offset( 0 );
    m_Seed = (uint32_t)time( nullptr ) + offset;
    offset++;
}

// GetRand
//------------------------------------------------------------------------------
uint32_t Random::GetRand()
{
    m_Seed = m_Seed * 1103515245 + 12345;
    return( (uint32_t)( m_Seed / ( ( CORE_RAND_MAX + 1 ) * 2 ) ) % ( CORE_RAND_MAX + 1 ) );
}

//------------------------------------------------------------------------------
