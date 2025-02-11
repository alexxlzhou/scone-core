/*
** model_tools.cpp
**
** Copyright (C) 2013-2019 Thomas Geijtenbeek and contributors. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#include "model_tools.h"

#include "scone/core/string_tools.h"
#include "scone/core/profiler_config.h"
#include "scone/model/UserInput.h"

using std::vector;
using std::pair;

namespace scone
{
	Vec3 GetGroundCop( const Vec3& force, const Vec3& moment, Real min_force )
	{
		if ( force.y >= min_force )
			return Vec3( moment.z / force.y, 0, -moment.x / force.y );
		else return Vec3::zero();
	}

	Vec3 GetPlaneCop( const Vec3& normal, const Vec3& location, const Vec3& force, const Vec3& moment, Real min_force /*= REAL_WIDE_EPSILON */ )
	{
		if ( dot_product( normal, force ) >= min_force )
		{
			auto normal_force_scalar = xo::dot_product( force, normal );
			auto pos0 = xo::cross_product( normal, moment ) / normal_force_scalar;
			Vec3 delta_pos = pos0 - location;
			double p1 = dot_product( delta_pos, normal );
			double p2 = dot_product( force, normal );
			auto pos = pos0 - ( p1 / p2 ) * force;
			return pos;
		}
		else return Vec3::zero();
	}

	PropNode MakePropNode( const std::vector<UserInputUP>& user_inputs )
	{
		PropNode pn;
		for ( auto& ui : user_inputs )
			pn[ ui->GetName() ] = ui->GetValue();
		return pn;
	}

	size_t SetUserInputsFromPropNode( const PropNode& pn, const std::vector<UserInputUP>& user_inputs )
	{
		size_t count = 0;
		for ( auto& ui : user_inputs )
		{
			if ( auto v = pn.try_get<Real>( ui->GetName() ) )
				ui->SetValue( *v ), ++count;
		}
		return count;
	}
}
