/*
** string_tools.cpp
**
** Copyright (C) 2013-2019 Thomas Geijtenbeek and contributors. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#include "string_tools.h"

#include <time.h>

#ifdef WIN32
#include <shlwapi.h>
#pragma comment( lib, "shlwapi.lib" )
#pragma warning( disable: 4996 ) // no push/pop because it's a cpp file
#else
#include <fnmatch.h>
#endif

#include <sstream>
#include <cstdarg>
#include <chrono>
#include <iostream>
#include <ctime>
#include <cmath>
#include "version.h"

using std::cout;
using std::endl;

namespace scone
{
	std::string GetDateTimeAsString()
	{
		std::time_t today = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
		auto tm = std::localtime( &today );
		return stringf( "%02d%02d%02d.%02d%02d%02d", tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );
	}

	std::string GetDateTimeExactAsString()
	{
		// #todo: deprecate
		// users that run multiple simulations in quick succession
		// should differentiate them through the .R 'random_seed' tag
		std::time_t today = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
		auto tm = std::localtime( &today );
		auto p = std::chrono::high_resolution_clock::now();
		auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>( p.time_since_epoch() );
		auto frac_secs = nsec.count() % 1000000;
		return stringf( "%02d%02d.%02d%02d%02d.%06d", tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, frac_secs );
	}

	void ReplaceStringTags( String& str )
	{
		xo::replace_str( str, "DATE_TIME_EXACT", GetDateTimeExactAsString() );
		xo::replace_str( str, "DATE_TIME", GetDateTimeAsString() );
		xo::replace_str( str, "SCONE_BUILD", to_str( GetSconeVersion().build_ ) );
		xo::replace_str( str, "SCONE_VERSION", to_str( GetSconeVersion().build_ ) );
	}

	const char* GetAxisName( index_t axis )
	{
		static const char* axis_names[] = { "X", "Y", "Z" };
		return axis < 3 ? axis_names[ axis ] : "";
	}

	const char* GetAxisName( const Vec3& dir )
	{
		return GetAxisName( GetAxisIndex( dir ) );
	}

	index_t GetAxisIndex( const Vec3& dir )
	{
		auto x = std::abs( dir.x ), y = std::abs( dir.y ), z = std::abs( dir.z );
		if ( x > y && x > z )
			return 0;
		else if ( y > z )
			return 1;
		else return 2;
	}
}
