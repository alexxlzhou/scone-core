#include "FeedForwardController.h"

#include "scone/model/Controller.h"
#include "scone/model/Muscle.h"
#include "scone/model/Locality.h"

#include "scone/core/Factories.h"
#include "scone/core/Profiler.h"

namespace scone
{
	FeedForwardController::FeedForwardController( const PropNode& props, Params& par, Model& model, const Locality& target_area ) :
		Controller( props, par, model, target_area )
	{
		INIT_PROP( props, symmetric, true );
		INIT_PROP( props, number_of_modes, size_t( 0 ) );

		// setup actuator info
		for ( size_t idx = 0; idx < model.GetMuscles().size(); ++idx )
		{
			ActInfo ai;
			ai.full_name = model.GetMuscles()[ idx ]->GetName();
			ai.name = GetNameNoSide( ai.full_name );
			ai.side = GetSideFromName( ai.full_name );
			ai.muscle_idx = idx;

			// see if this muscle is on the right side
			if ( target_area.side == NoSide || target_area.side == ai.side )
				m_ActInfos.push_back( ai );
		}

		// create functions
		if ( UseModes() )
		{
			// create mode functions
			for ( size_t idx = 0; idx < number_of_modes; ++idx )
			{
				ScopedParamSetPrefixer prefixer( par, stringf( "Mode%d.", idx ) );
				m_Functions.push_back( FunctionUP( scone::CreateFunction( props.get_child( "Function" ), par ) ) );
			}
		}

		for ( ActInfo& ai : m_ActInfos )
		{
			if ( symmetric )
			{
				// check if we've already processed a mirrored version of this ActInfo
				auto it = std::find_if( m_ActInfos.begin(), m_ActInfos.end(), [&]( ActInfo& oai ) { return ai.name == oai.name; } );
				if ( it->function_idx != NoIndex || !it->mode_weights.empty() )
				{
					ai.function_idx = it->function_idx;
					ai.mode_weights = it->mode_weights;
					continue;
				}
			}

			if ( UseModes() )
			{
				// set mode weights
				ai.mode_weights.resize( number_of_modes );
				String prefix = symmetric ? ai.name : ai.full_name;
				for ( size_t mode = 0; mode < number_of_modes; ++mode )
					ai.mode_weights[ mode ] = par.get( prefix + stringf( ".Mode%d", mode ), props[ "mode_weight" ] );
			}
			else
			{
				// create a new function
				String prefix = symmetric ? ai.name : ai.full_name;
				ScopedParamSetPrefixer prefixer( par, prefix + "." );
				m_Functions.push_back( FunctionUP( scone::CreateFunction( props.get_child( "Function" ), par ) ) );
				ai.function_idx = m_Functions.size() - 1;
			}
		}
	}

	bool FeedForwardController::ComputeControls( Model& model, double time )
	{
		SCONE_PROFILE_FUNCTION;

		// evaluate functions
		std::vector< double > funcresults( m_Functions.size() );
		for ( size_t idx = 0; idx < m_Functions.size(); ++idx )
			funcresults[ idx ] = m_Functions[ idx ]->GetValue( time );

		// apply results to all actuators
		for ( ActInfo& ai : m_ActInfos )
		{
			if ( UseModes() )
			{
				Real val = 0.0;
				for ( size_t mode = 0; mode < number_of_modes; ++mode )
					val += ai.mode_weights[ mode ] * funcresults[ mode ];

				// add control value
				model.GetMuscles()[ ai.muscle_idx ]->AddInput( val );
			}
			else
			{
				// apply results directly to control value
				model.GetMuscles()[ ai.muscle_idx ]->AddInput( funcresults[ ai.function_idx ] );
			}
		}

		return false;
	}

	scone::String FeedForwardController::GetClassSignature() const
	{
		String s = "F" + m_Functions.front()->GetSignature();
		if ( number_of_modes > 0 )
			s += stringf( "M%d", number_of_modes );

		return s;
	}
}
