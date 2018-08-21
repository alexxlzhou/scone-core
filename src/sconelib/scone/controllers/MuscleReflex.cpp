#include "MuscleReflex.h"
#include "scone/model/Muscle.h"
#include "scone/model/Locality.h"
#include "scone/model/Dof.h"
#include "scone/core/propnode_tools.h"

namespace scone
{
	MuscleReflex::MuscleReflex( const PropNode& props, Params& par, Model& model, const Locality& area ) :
	Reflex( props, par, model, area ),
	m_pForceSensor( nullptr ),
	m_pLengthSensor( nullptr ),
	m_pVelocitySensor( nullptr ),
	m_pSpindleSensor( nullptr )
	{
		auto trg_name = props.get< String >( "target" );
		auto src_name = props.get< String >( "source", trg_name );
		auto muscle_name = area.ConvertName( src_name );
		Muscle& source = *FindByName( model.GetMuscles(), muscle_name );

		// init names
		String par_name = GetParName( props );
		name = GetReflexName( m_Target.GetName(), source.GetName() );
		ScopedParamSetPrefixer prefixer( par, par_name + "." );

		INIT_PAR_NAMED( props, par, length_gain, "KL", 0.0 );
		INIT_PAR_NAMED( props, par, length_ofs, "L0", 1.0 );
		INIT_PROP_NAMED( props, length_allow_negative, "allow_neg_L", true );

		INIT_PAR_NAMED( props, par, velocity_gain, "KV", 0.0 );
		INIT_PAR_NAMED( props, par, velocity_ofs, "V0", 0.0 );
		INIT_PROP_NAMED( props, velocity_allow_negative, "allow_neg_V", false );

		INIT_PAR_NAMED( props, par, force_gain, "KF", 0.0 );
		INIT_PAR_NAMED( props, par, force_ofs, "F0", 0.0 );
		INIT_PROP_NAMED( props, force_allow_negative, "allow_neg_F", true );

		INIT_PAR_NAMED( props, par, spindle_gain, "KS", 0.0 );
		INIT_PAR_NAMED( props, par, spindle_ofs, "S0", 0.0 );
		INIT_PROP_NAMED( props, spinde_allow_negative, "allow_neg_S", false );

		INIT_PAR_NAMED( props, par, u_constant, "C0", 0.0 );

		// create delayed sensors
		if ( force_gain != 0.0 )
			m_pForceSensor = &model.AcquireDelayedSensor< MuscleForceSensor >( source );

		if ( length_gain != 0.0 )
			m_pLengthSensor = &model.AcquireDelayedSensor< MuscleLengthSensor >( source );

		if ( velocity_gain != 0.0 )
			m_pVelocitySensor = &model.AcquireDelayedSensor< MuscleVelocitySensor >( source );

		if ( spindle_gain!= 0.0 )
			m_pSpindleSensor = &model.AcquireDelayedSensor< MuscleSpindleSensor >( source );

		//log::TraceF( "MuscleReflex SRC=%s TRG=%s KL=%.2f KF=%.2f C0=%.2f", source.GetName().c_str(), m_Target.GetName().c_str(), length_gain, force_gain, u_constant );
	}

	MuscleReflex::~MuscleReflex()
	{
	}

	void MuscleReflex::ComputeControls( double timestamp )
	{
		// add stretch reflex
		u_l = m_pLengthSensor ? length_gain * ( m_pLengthSensor->GetValue( delay ) - length_ofs ) : 0;
		if ( !length_allow_negative && u_l < 0.0 ) u_l = 0.0;

		// add velocity reflex
		u_v = m_pVelocitySensor ? velocity_gain * ( m_pVelocitySensor->GetValue( delay ) - velocity_ofs ) : 0;
		if ( !velocity_allow_negative && u_v < 0.0 ) u_v = 0.0;

		// add force reflex
		u_f = m_pForceSensor ? force_gain * ( m_pForceSensor->GetValue( delay ) - force_ofs ) : 0;
		if ( !force_allow_negative && u_f < 0.0 ) u_f = 0.0;

		// add spindle reflex
		u_s = m_pSpindleSensor ? spindle_gain * ( m_pSpindleSensor->GetValue( delay ) - spindle_ofs ) : 0;
		if ( !spinde_allow_negative && u_s < 0.0 ) u_s = 0.0;

		// sum it up
		u_total = u_l + u_v + u_f + u_s + u_constant;
		AddTargetControlValue( u_total );
	}

	void MuscleReflex::StoreData( Storage< Real >::Frame& frame, const StoreDataFlags& flags ) const
	{
		if ( m_pLengthSensor )
			frame[ name + ".RL" ] = u_l;

		if ( m_pVelocitySensor )
			frame[ name + ".RV" ] = u_v;

		if ( m_pForceSensor )
			frame[ name + ".RF" ] = u_f;

		if ( m_pSpindleSensor )
			frame[ name + ".RS" ] = u_s;
	}
}
