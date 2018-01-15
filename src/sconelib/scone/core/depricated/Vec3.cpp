#include "stdafx.h"

#if 0
#include "Vec3.h"
#include "Exception.h"
#include <sstream>
#include <cmath>
#include "tools.h"

namespace scone
{
	const Vec3 Vec3::ZERO = Vec3( 0, 0, 0 );
	const Vec3 Vec3::UNIT_X = Vec3( 1, 0, 0 );
	const Vec3 Vec3::UNIT_Y = Vec3( 0, 1, 0 );
	const Vec3 Vec3::UNIT_Z = Vec3( 0, 0, 1 );
	const Vec3 Vec3::NEG_UNIT_X = Vec3( -1, 0, 0 );
	const Vec3 Vec3::NEG_UNIT_Y = Vec3( 0, -1, 0 );
	const Vec3 Vec3::NEG_UNIT_Z = Vec3( 0, 0, -1 );

	bool Vec3::IsZeroLength()
	{
		return GetSquaredLength() <= std::numeric_limits<Real>::epsilon();
	}

	Real Vec3::GetSquaredLength() const
	{
		return X() * X() + Y() * Y() + Z() * Z();
	}

	Real Vec3::GetLength() const
	{
		return sqrt( X() * X() + Y() * Y() + Z() * Z() );
	}

	Vec3 Vec3::GetNormalized() const
	{
		Real s = GetLength();
		if ( s > std::numeric_limits<Real>::epsilon() )
			return ( *this ) * ( ( Real )1.0 / s );
		else return ( *this );
	}

	bool Vec3::Normalize()
	{
		Real s = GetLength();
		if ( s <= std::numeric_limits<Real>::epsilon() )
			return false;
		( *this ) *= ( ( Real )1.0 / s );
		return true;
	}

	Vec3 operator*( Real s, const Vec3& vec )
	{
		return Vec3( vec.X() * s, vec.Y() * s, vec.Z() * s );
	}

	Vec3 Vec3::operator*( const Vec3& vec ) const
	{
		return Vec3( X() * vec.X(), Y() * vec.Y(), Z() * vec.Z() );
	}

	Vec3 Vec3::operator*( Real s ) const
	{
		return Vec3( X() * s, Y() * s, Z() * s );
	}

	Vec3& Vec3::operator*=( const Vec3& v )
	{
		X() *= v.X();
		Y() *= v.Y();
		Z() *= v.Z();
		return ( *this );
	}

	Vec3& Vec3::operator*=( Real s )
	{
		X() *= s;
		Y() *= s;
		Z() *= s;
		return ( *this );
	}

	Vec3& Vec3::operator/=( Real s )
	{
		SCONE_ASSERT( s != Real( 0 ) );
		s = Real( 1 ) / s;

		X() *= s;
		Y() *= s;
		Z() *= s;
		return ( *this );
	}

	Vec3 Vec3::operator/( Real s ) const
	{
		SCONE_ASSERT( s != Real( 0 ) );
		s = Real( 1 ) / s;
		return Vec3( X() * s, Y() * s, Z() * s );
	}

	Vec3 Vec3::operator+( const Vec3& v ) const
	{
		return Vec3( X() + v.X(), Y() + v.Y(), Z() + v.Z() );
	}

	Vec3& Vec3::operator+=( const Vec3& v )
	{
		X() += v.X();
		Y() += v.Y();
		Z() += v.Z();
		return ( *this );
	}

	Vec3 Vec3::operator-( const Vec3& v ) const
	{
		return Vec3( X() - v.X(), Y() - v.Y(), Z() - v.Z() );
	}

	Vec3& Vec3::operator-=( const Vec3& v )
	{
		X() -= v.X();
		Y() -= v.Y();
		Z() -= v.Z();
		return ( *this );
	}

	Vec3 Vec3::operator-() const
	{
		return Vec3( -X(), -Y(), -Z() );
	}

	Vec3 Vec3::GetCrossProduct( const Vec3& v ) const
	{
		return Vec3( Y() * v.Z() - Z() * v.Y(), Z() * v.X() - X() * v.Z(), X() * v.Y() - Y() * v.X() );
	}

	Real Vec3::GetDotProduct( const Vec3& v ) const
	{
		return X() * v.X() + Y() * v.Y() + Z() * v.Z();
	}

	Real GetDotProduct( const Vec3& v1, const Vec3& v2 )
	{
		return v1.GetDotProduct( v2 );
	}

	Real Vec3::GetAngle( const Vec3& v ) const
	{
		return acos( GetNormalized().GetDotProduct( v.GetNormalized() ) );
	}

	Vec3& Vec3::ProjectPlane( const Vec3& plane_normal )
	{
		Vec3 pnn = plane_normal.GetNormalized();
		( *this ) -= GetDotProduct( pnn ) * pnn;
		return ( *this );
	}

	Vec3 Vec3::GetPlaneProjection( const Vec3& plane_normal ) const
	{
		Vec3 pnn = plane_normal.GetNormalized();
		return *this - GetDotProduct( pnn ) * pnn;
	}

	std::string Vec3::ToString() const
	{
		return GetStringF( "(%4f,%4f,%4f)", X(), Y(), Z() );
	}

	Vec3& Vec3::FromString( std::string& str )
	{
		std::stringstream s( str );
		s >> *this;
		return *this;
	}

	Real Vec3::GetAngleOnPlane( const Vec3& v1, const Vec3& v2 ) const
	{
		return v1.GetPlaneProjection(*this).GetAngle(v2.GetPlaneProjection(*this));
	}

	Real Vec3::GetVolume() const
	{
		return X() * Y() * Z();
	}

	double Vec3::GetSum() const
	{
		return X() + Y() + Z();
	}

	Vec3 Vec3::GetScaled( Real sx, Real sy, Real sz ) const
	{
		return Vec3( sx * X(), sy * Y(), sz * Z() );
	}

	Vec3 Vec3::GetVectorProjection( const Vec3& vec ) const
	{
		// http://math.oregonstate.edu/home/programs/undergrad/CalculusQuestStudyGuides/vcalc/dotprod/dotprod.html
		// TODO: needs to me tested
		return ( GetDotProduct( vec ) / GetSquaredLength() ) * (*this);
	}
}
#endif