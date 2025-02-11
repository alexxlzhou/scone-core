/*
** MuscleMeasure.cpp
**
** Copyright (C) 2013-2019 Thomas Geijtenbeek and contributors. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#include "MuscleMeasure.h"
#include "scone/model/Model.h"
#include "scone/core/string_tools.h"

namespace scone
{
	MuscleMeasure::MuscleMeasure( const PropNode& props, Params& par, const Model& model, const Location& loc ) :
		Measure( props, par, model, loc ),
		muscle( *FindByLocation( model.GetMuscles(), props.get< String >( "muscle" ), loc ) )
	{
		INIT_PROP( props, input, RangePenalty<Real>() );
		INIT_PROP( props, activation, RangePenalty<Real>() );
		INIT_PROP( props, length, RangePenalty<Real>() );
		INIT_PROP( props, velocity, RangePenalty<Real>() );
		INIT_PROP( props, force, RangePenalty<Real>() );

		if ( name_.empty() )
			name_ = muscle.GetName();
	}

	double MuscleMeasure::ComputeResult( const Model& model )
	{
		double penalty = 0.0;
		auto range_count = int( !input.IsNull() ) + int( !activation.IsNull() ) + int( !velocity.IsNull() ) + int( !length.IsNull() );
		if ( !input.IsNull() )
		{
			penalty += input.GetResult();
			if ( range_count > 1 )
				GetReport().set( name_ + ".input_penalty", stringf( "%g", input.GetResult() ) );
		}
		if ( !activation.IsNull() )
		{
			penalty += activation.GetResult();
			if ( range_count > 1 )
				GetReport().set( name_ + ".activation_penalty", stringf( "%g", activation.GetResult() ) );
		}
		if ( !length.IsNull() )
		{
			penalty += length.GetResult();
			if ( range_count > 1 )
				GetReport().set( name_ + ".length_penalty", stringf( "%g", length.GetResult() ) );
		}
		if ( !velocity.IsNull() )
		{
			penalty += velocity.GetResult();
			if ( range_count > 1 )
				GetReport().set( name_ + ".velocity_penalty", stringf( "%g", velocity.GetResult() ) );
		}
		if ( !force.IsNull() )
		{
			penalty += force.GetResult();
			if ( range_count > 1 )
				GetReport().set( name_ + ".force_penalty", stringf( "%g", force.GetResult() ) );
		}

		return  penalty;
	}

	bool MuscleMeasure::UpdateMeasure( const Model& model, double timestamp )
	{
		input.AddSample( timestamp, muscle.GetInput() );
		activation.AddSample( timestamp, muscle.GetActivation() );
		length.AddSample( timestamp, muscle.GetNormalizedFiberLength() );
		velocity.AddSample( timestamp, muscle.GetNormalizedFiberVelocity() );
		force.AddSample( timestamp, muscle.GetNormalizedForce() );
		return false;
	}

	String MuscleMeasure::GetClassSignature() const
	{
		return String();
	}

	void MuscleMeasure::StoreData( Storage< Real >::Frame& frame, const StoreDataFlags& flags ) const
	{
		if ( !input.IsNull() )
			frame[ muscle.GetName() + ".input_penalty" ] = input.GetLatest();
		if ( !activation.IsNull() )
			frame[ muscle.GetName() + ".activation_penalty" ] = activation.GetLatest();
		if ( !length.IsNull() )
			frame[ muscle.GetName() + ".length_penalty" ] = length.GetLatest();
		if ( !velocity.IsNull() )
			frame[ muscle.GetName() + ".velocity_penalty" ] = velocity.GetLatest();
		if ( !force.IsNull() )
			frame[ muscle.GetName() + ".force_penalty" ] = force.GetLatest();
	}
}
