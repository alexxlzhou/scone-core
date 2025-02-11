/*
** string_tools.h
**
** Copyright (C) 2013-2019 Thomas Geijtenbeek and contributors. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#pragma once

#include "platform.h"
#include "types.h"

#include "xo/string/string_tools.h"
#include "xo/string/string_cast.h"
#include "Vec3.h"

namespace scone
{
	// import string tools from xo
	using xo::stringf;
	using xo::to_str;
	using xo::from_str;
	using xo::quoted;

	template< typename T > char GetSignChar( const T& v ) { return v < 0 ? '-' : '+'; }

	/// Get formatted date/time string
	SCONE_API String GetDateTimeAsString();

    /// Get formatted date/time with exact fractional seconds as string
	SCONE_API String GetDateTimeExactAsString();

	/// Replace DATE_TIME, SCONE_VERSION, etc. with actual values
	SCONE_API void ReplaceStringTags( String& str );

	/// Get axis name (X, Y or Z)
	const char* GetAxisName( index_t axis );
	index_t GetAxisIndex( const Vec3& dir );
	const char* GetAxisName( const Vec3& dir );
}
