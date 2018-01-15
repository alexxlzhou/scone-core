#pragma once

#include <limits>
#include "core.h"

namespace scone
{
	template< typename T >
	class Range
	{
	public:
		Range( const PropNode& props ) { min = props.get< T >( "min", std::numeric_limits< T >::min() ); max = props.get< T >( "max", std::numeric_limits< T >::max() ); }
		Range( const T& i_min = T(), const T& i_max = T() ) : min( i_min ), max( i_max ) { };

		// test if a value is inside the range
		bool Test( const T& value ) { return ( value >= min ) && ( value <= max ); }

		// get how much a value is outside the range
		// negative == below min, positive == above max, 0 = within range
		T GetRangeViolation( const T& value ) {
			if ( value < min ) return value - min;
			else if ( value > max ) return value - max;
			else return T( 0 );
		}

		T min;
		T max;
	};

	typedef Range< Real > RealRange;
}
