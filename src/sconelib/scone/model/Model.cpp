/*
** Model.cpp
**
** Copyright (C) 2013-2019 Thomas Geijtenbeek and contributors. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#include "Model.h"
#include "Body.h"
#include "Dof.h"
#include "Joint.h"
#include "Muscle.h"
#include "UserInput.h"
#include "SensorDelayAdapter.h"
#include "State.h"
#include "scone/controllers/CompositeController.h"
#include "scone/core/Factories.h"
#include "scone/core/Log.h"
#include "scone/core/profiler_config.h"
#include "scone/core/Settings.h"
#include "scone/core/StorageIo.h"
#include "scone/measures/Measure.h"
#include "scone/core/version.h"

#include "xo/container/container_tools.h"
#include "xo/string/string_tools.h"
#include "xo/filesystem/filesystem.h"
#include "xo/container/storage.h"
#include "xo/container/flat_map.h"
#include "xo/shape/shape_tools.h"
#include "xo/container/container_algorithms.h"

#include <algorithm>
#include <tuple>
#include <fstream>

using std::endl;

namespace scone
{
	Model::Model( const PropNode& props, Params& par ) :
		HasSignature( props ),
		INIT_MEMBER( props, state_init_file, path() ),
		INIT_MEMBER( props, state_init_file_ignore_activations, false ),
		INIT_PAR_MEMBER( props, par, initial_load, 0.2 ),
		INIT_MEMBER( props, initial_load_dof, "pelvis_ty" ),
		INIT_PAR_MEMBER( props, par, sensor_delay_scaling_factor, 1.0 ),
		INIT_PAR_MEMBER( props, par, initial_equilibration_activation, 0.05 ),
		INIT_PAR_MEMBER( props, par, initialize_activations_from_controller, xo::optional<bool>() ),
		INIT_MEMBER( props, neural_delays, {} ),
		INIT_MEMBER( props, user_input_file, "" ),
		INIT_MEMBER( props, scone_version, GetSconeVersion() ),
		m_Profiler( GetProfilerEnabled() ),
		m_RootBody( nullptr ),
		m_GroundBody( nullptr ),
		m_Controller( nullptr ),
		m_Measure( nullptr ),
		m_ShouldTerminate( false ),
		m_PrevStoreDataTime( 0 ),
		m_PrevStoreDataStep( 0 ),
		m_SimulationTimer( false ),
		m_pModelProps( nullptr ),
		m_pCustomProps( nullptr ),
		m_StoreData( false ),
		m_StoreDataInterval( 1.0 / GetSconeSetting<double>( "data.frequency" ) ),
		m_StoreDataFlags( { StoreDataTypes::State, StoreDataTypes::ActuatorInput, StoreDataTypes::GroundReactionForce, StoreDataTypes::ContactForce } ),
		m_KeepAllFrames( GetSconeSetting<bool>( "data.keep_all_frames" ) )
	{
		SCONE_PROFILE_FUNCTION( GetProfiler() );

		if ( scone_version > GetSconeVersion() )
			log::warning( "This scenario was created for using a newer version of SCONE (", scone_version, ")" );

		// old-style initialization (for backwards compatibility)
		if ( auto sio = props.try_get_child( "state_init_optimization" ) )
		{
			initial_state_offset = sio->try_get_child( "offset" );
			initial_state_offset_symmetric = sio->get( "symmetric", false );
			initial_state_offset_include = sio->get< String >( "include_states", "*" );
			initial_state_offset_exclude = sio->get< String >( "exclude_states", "" );
		}
		else
		{
			initial_state_offset = props.try_get_child( "initial_state_offset" );
			INIT_PROP( props, initial_state_offset_symmetric, false );
			INIT_PROP( props, initial_state_offset_include, "*" );
			INIT_PROP( props, initial_state_offset_exclude, "" );
		}

		INIT_PROP( props, use_fixed_control_step_size, true );
		INIT_PROP( props, fixed_control_step_size, 0.001 );
		INIT_PROP( props, fixed_measure_step_size, fixed_control_step_size );
		INIT_PROP( props, max_step_size, scone_version >= version( 2, 0, 0 ) ? fixed_control_step_size : 0.001 );
		fixed_step_size = std::min( fixed_control_step_size, fixed_measure_step_size );
		fixed_control_step_interval = static_cast<int>( std::round( fixed_control_step_size / fixed_step_size ) );
		fixed_analysis_step_interval = static_cast<int>( std::round( fixed_measure_step_size / fixed_step_size ) );

		// set store data info from settings
		auto& flags = GetStoreDataFlags();
		flags.set( StoreDataTypes::BodyPosition, GetSconeSetting<bool>( "data.body" ) );
		flags.set( StoreDataTypes::JointReactionForce, GetSconeSetting<bool>( "data.joint" ) );
		flags.set( StoreDataTypes::ActuatorInput, GetSconeSetting<bool>( "data.actuator" ) );
		flags.set( StoreDataTypes::MuscleProperties, GetSconeSetting<bool>( "data.muscle" ) );
		flags.set( StoreDataTypes::MuscleDofMomentPower, GetSconeSetting<bool>( "data.muscle_dof" ) );
		flags.set( StoreDataTypes::GroundReactionForce, GetSconeSetting<bool>( "data.grf" ) );
		flags.set( StoreDataTypes::ContactForce, GetSconeSetting<bool>( "data.contact" ) );
		flags.set( StoreDataTypes::SystemPower, GetSconeSetting<bool>( "data.power" ) );
		flags.set( StoreDataTypes::SensorData, GetSconeSetting<bool>( "data.sensor" ) );
		flags.set( StoreDataTypes::ControllerData, GetSconeSetting<bool>( "data.controller" ) );
		flags.set( StoreDataTypes::MeasureData, GetSconeSetting<bool>( "data.measure" ) );
		flags.set( StoreDataTypes::SimulationStatistics, GetSconeSetting<bool>( "data.simulation" ) );
		flags.set( StoreDataTypes::DebugData, GetSconeSetting<bool>( "data.debug" ) );
	}

	Model::~Model() {}

	SensorDelayAdapter& Model::AcquireSensorDelayAdapter( Sensor& source )
	{
		auto it = std::find_if( m_SensorDelayAdapters.begin(), m_SensorDelayAdapters.end(),
			[&]( SensorDelayAdapterUP& a ) { return &a->GetInputSensor() == &source; } );

		if ( it == m_SensorDelayAdapters.end() )
		{
			m_SensorDelayAdapters.push_back( std::make_unique<SensorDelayAdapter>( *this, source, 0.0 ) );
			return *m_SensorDelayAdapters.back();
		}
		else return **it;
	}

	DelayedSensorValue Model::GetDelayedSensor( Sensor& sensor, TimeInSeconds delay )
	{
		return m_DelayedSensors.GetDelayedSensorValue( sensor, delay, fixed_control_step_size );
	}

	DelayedActuatorValue Model::GetDelayedActuator( Actuator& actuator, TimeInSeconds delay )
	{
		return m_DelayedActuators.GetDelayedActuatorValue( actuator, delay, fixed_control_step_size );
	}

	String Model::GetClassSignature() const
	{
		auto sig = GetName();
		if ( GetController() )
			sig += '.' + GetController()->GetSignature();
		if ( GetMeasure() )
			sig += '.' + GetMeasure()->GetSignature();
		return sig;
	}

	void Model::UpdateSensorDelayAdapters()
	{
		SCONE_PROFILE_FUNCTION( GetProfiler() );

		if ( !m_SensorDelayAdapters.empty() )
		{
			const bool first_frame = GetTime() == 0 && m_SensorDelayStorage.IsEmpty();
			const bool redo_first_frame = GetTime() == 0 && m_SensorDelayStorage.GetFrameCount() == 1;
			const bool subsequent_frame = !m_SensorDelayStorage.IsEmpty() && GetTime() > GetPreviousTime() && GetPreviousTime() == m_SensorDelayStorage.Back().GetTime();
			SCONE_ASSERT( first_frame || redo_first_frame || subsequent_frame );

			if ( !redo_first_frame )
				m_SensorDelayStorage.AddFrame( GetTime() );

			for ( auto& sda : m_SensorDelayAdapters )
				sda->UpdateStorage();

			//log::TraceF( "Updated Sensor Delays for Int=%03d time=%.6f prev_time=%.6f", GetIntegrationStep(), GetTime(), GetPreviousTime() );
		}
	}

	void Model::CreateControllers( const PropNode& pn, Params& par )
	{
		// add controller (new style, prefer define outside model)
		if ( auto* cprops = pn.try_get_child( "Controller" ) )
			SetController( scone::CreateController( *cprops, par, *this, Location() ) );

		// add measure (new style, prefer define outside model)
		if ( auto* cprops = pn.try_get_child( "Measure" ) )
			SetMeasure( scone::CreateMeasure( *cprops, par, *this, Location() ) );

		// add multiple controllers / measures (old style)
		if ( auto* cprops = pn.try_get_child( "Controllers" ) )
		{
			SetController( std::make_unique< CompositeController >( *cprops, par, *this, Location() ) );
			if ( auto* mprops = cprops->try_get_child( "Measure" ) )
				SetMeasure( scone::CreateMeasure( *mprops, par, *this, Location() ) );
		}
	}

	void Model::SetStoreData( bool store )
	{
		SCONE_ERROR_IF( store && GetTime() > 0.0, "Model SetStoreData() can only be set at before starting the simulation" );
		m_StoreData = store;
	}

	bool Model::GetStoreData() const
	{
		return m_StoreData &&
			( m_Data.IsEmpty()
				|| m_KeepAllFrames && GetTime() != m_Data.Back().GetTime()
				|| xo::greater_than_or_equal( GetTime() - m_Data.Back().GetTime(), m_StoreDataInterval, 1e-6 ) );
	}

	void Model::StoreData( Storage< Real >::Frame& frame, const StoreDataFlags& flags ) const
	{
		SCONE_PROFILE_FUNCTION( GetProfiler() );

		// store states
		if ( flags( StoreDataTypes::State ) )
		{
			for ( size_t i = 0; i < GetState().GetSize(); ++i )
				frame[ GetState().GetName( i ) ] = GetState().GetValue( i );
		}

		// store simulation statistics
		if ( flags( StoreDataTypes::SimulationStatistics ) )
		{
			auto dt = GetTime() - m_PrevStoreDataTime;
			auto step_count = GetIntegrationStep() - m_PrevStoreDataStep;
			frame[ "simulation_frequency" ] = dt > 0 ? step_count / dt : 0.0;
		}

		// store actuator data
		for ( auto& m : GetActuators() )
			m->StoreData( frame, flags );

		// store body data
		for ( auto& b : GetBodies() )
			b->StoreData( frame, flags );

		// store joint data
		for ( auto& j : GetJoints() )
			j->StoreData( frame, flags );

		// store dof moments and powers
		if ( flags( StoreDataTypes::MuscleDofMomentPower ) )
		{
			for ( auto& d : GetDofs() )
			{
				auto mom = d->GetMuscleMoment() + d->GetLimitMoment();
				frame[ d->GetName() + ".moment" ] = mom;
				frame[ d->GetName() + ".moment_norm" ] = mom / GetMass();
				frame[ d->GetName() + ".power" ] = mom * d->GetVel();
				frame[ d->GetName() + ".power_norm" ] = mom * d->GetVel() / GetMass();
				frame[ d->GetName() + ".acceleration" ] = d->GetAcc();
			}
		}

		// powers
		if ( flags( StoreDataTypes::SystemPower ) )
		{
			auto bp = xo::accumulate( GetBodies(), 0.0,
				[&]( auto val, auto& obj ) { return val + obj->GetPower(); } );
			auto mp = xo::accumulate( GetMuscles(), 0.0,
				[&]( auto val, auto& obj ) { return val + obj->GetForce() * -obj->GetVelocity(); } );
			auto jp = xo::accumulate( GetJoints(), 0.0,
				[&]( auto val, auto& obj ) { return val + obj->GetLimitPower(); } );
			auto cp = GetTotalContactPower();
			auto gp = xo::dot_product( GetComVel(), GetMass() * GetGravity() );
			auto external_power = jp + cp + mp + gp;
			frame[ "total_body.power" ] = bp;
			frame[ "total_muscle.power" ] = mp;
			frame[ "total_joint_limit.power" ] = jp;
			frame[ "total_contact.power" ] = cp;
			frame[ "total_gravity.power" ] = gp;
			frame[ "total_external.power" ] = external_power;
			frame[ "total.power" ] = bp - external_power;
		}

		// store controller / measure data
		if ( flags( StoreDataTypes::ControllerData ) && GetController() )
			GetController()->StoreData( frame, flags );
		if ( flags( StoreDataTypes::MeasureData ) && GetMeasure() )
			GetMeasure()->StoreData( frame, flags );

		// store sensor data
		if ( flags( StoreDataTypes::SensorData ) && !m_SensorDelayStorage.IsEmpty() )
		{
			const auto& sf = m_SensorDelayStorage.Back();
			for ( index_t i = 0; i < m_SensorDelayStorage.GetChannelCount(); ++i )
				frame[ m_SensorDelayStorage.GetLabels()[ i ] ] = sf[ i ];
		}

		// store COP data
		if ( flags( StoreDataTypes::BodyPosition ) )
		{
			auto com = GetComPos();
			auto com_u = GetComVel();
			frame[ "com_x" ] = com.x;
			frame[ "com_y" ] = com.y;
			frame[ "com_z" ] = com.z;
			frame[ "com_x_u" ] = com_u.x;
			frame[ "com_y_u" ] = com_u.y;
			frame[ "com_z_u" ] = com_u.z;

			const auto mom = GetLinAngMom();
			frame.SetVec3( "lin_mom", mom.first );
			frame.SetVec3( "ang_mom", mom.second );
		}

		// store GRF data (measured in BW)
		if ( flags( StoreDataTypes::GroundReactionForce ) )
		{
			for ( auto& leg : GetLegs() )
			{
				Vec3 force, moment, cop;
				leg->GetContactForceMomentCop( force, moment, cop );
				Vec3 grf = force / GetBW();

				frame[ leg->GetName() + ".grf_norm_x" ] = grf.x;
				frame[ leg->GetName() + ".grf_norm_y" ] = grf.y;
				frame[ leg->GetName() + ".grf_norm_z" ] = grf.z;
				frame[ leg->GetName() + ".grf_x" ] = force.x;
				frame[ leg->GetName() + ".grf_y" ] = force.y;
				frame[ leg->GetName() + ".grf_z" ] = force.z;
				frame[ leg->GetName() + ".grm_x" ] = moment.x;
				frame[ leg->GetName() + ".grm_y" ] = moment.y;
				frame[ leg->GetName() + ".grm_z" ] = moment.z;
				frame[ leg->GetName() + ".cop_x" ] = cop.x;
				frame[ leg->GetName() + ".cop_y" ] = cop.y;
				frame[ leg->GetName() + ".cop_z" ] = cop.z;
			}
		}

		if ( flags( StoreDataTypes::ContactForce ) )
		{
			for ( auto& force : GetContactForces() )
				force->StoreData( frame, flags );
		}
	}

	void Model::StoreCurrentFrame()
	{
		SCONE_PROFILE_FUNCTION( GetProfiler() );
		if ( m_Data.IsEmpty() || GetTime() > m_Data.Back().GetTime() )
			m_Data.AddFrame( GetTime() );
		StoreData( m_Data.Back(), m_StoreDataFlags );

		m_PrevStoreDataTime = GetTime();
		m_PrevStoreDataStep = GetIntegrationStep();
	}

	void Model::CreateController( const FactoryProps& controller_fp, Params& par )
	{
		SCONE_PROFILE_FUNCTION( GetProfiler() );
		SetController( scone::CreateController( controller_fp, par, *this, Location() ) );
	}

	void Model::CreateMeasure( const FactoryProps& measure_fp, Params& par )
	{
		SCONE_PROFILE_FUNCTION( GetProfiler() );
		SetMeasure( scone::CreateMeasure( measure_fp, par, *this, Location() ) );
	}

	void Model::UpdateControlValues()
	{
		SCONE_PROFILE_FUNCTION( GetProfiler() );

		// reset actuator values
		if ( GetController() )
			for ( Actuator* a : GetActuators() )
				a->ClearInput();

		if ( GetTime() > 0 )
		{
			m_DelayedActuators.UpdateActuatorInputs();
			m_DelayedActuators.AdvanceActuatorBuffers();
			m_DelayedActuators.ClearActuatorBufferValues();
		}
		else {
			m_DelayedSensors.UpdateSensorBufferValues();
			m_DelayedActuators.ClearActuatorBufferValues();
		}

		bool terminate = false;
		if ( auto* c = GetController() )
			terminate |= c->UpdateControls( *this, GetTime() );

		if ( GetTime() > 0 )
		{
			m_DelayedSensors.AdvanceSensorBuffers();
			m_DelayedSensors.UpdateSensorBufferValues();
		}
		else {
			m_DelayedActuators.UpdateActuatorInputs();
		}

		if ( terminate )
			RequestTermination();
	}

	void Model::UpdateAnalyses()
	{
		SCONE_PROFILE_FUNCTION( GetProfiler() );

		bool terminate = false;
		if ( auto* c = GetController() )
			terminate |= c->UpdateAnalysis( *this, GetTime() );
		if ( auto* m = GetMeasure() )
			terminate |= m->UpdateAnalysis( *this, GetTime() );

		if ( terminate )
			RequestTermination();
	}

	std::vector< ForceValue > Model::GetContactForceValues() const
	{
		std::vector< ForceValue > fvec;
		fvec.reserve( GetContactForces().size() );
		for ( auto& cf : GetContactForces() )
		{
			auto cfv = cf->GetForceValue();
			if ( xo::squared_length( cfv.force ) > REAL_WIDE_EPSILON )
				fvec.push_back( cf->GetForceValue() );
		}
		return fvec;
	}

	void Model::SetNullState()
	{
		State zero_state = GetState();
		for ( index_t i = 0; i < zero_state.GetSize(); ++i )
		{
			if ( !xo::str_ends_with( zero_state.GetName( i ), ".fiber_length" ) &&
				!xo::str_ends_with( zero_state.GetName( i ), ".activation" ) )
				zero_state.SetValue( i, 0 );
		}
		SetState( zero_state, 0 );
	}

	void Model::SetNeutralState()
	{
		for ( auto& dof : GetDofs() )
		{
			dof->SetPos( dof->GetRange().GetCenter() );
			dof->SetVel( 0 );
		}
	}

	PropNode Model::GetSimulationReport() const
	{
		PropNode pn;
		auto& perf_pn = pn[ "Simulation Performance" ];
		perf_pn[ "simulation_frequency" ] = ( GetIntegrationStep() / GetTime() );
		if ( auto sd = GetSimulationDuration(); sd > 0 )
			perf_pn[ "simulation_duration" ] = xo::stringf( "%.3fs (%.4gx real-time)", sd, GetTime() / sd );
		return pn;
	}

	std::vector<path> Model::WriteResults( const path& file ) const
	{
		std::vector<path> files;
		WriteStorageSto( m_Data, file + ".sto", ( file.parent_path().filename() / file.stem() ).str(), m_StoreDataInterval );
		files.push_back( file + ".sto" );

		if ( GetSconeSetting<bool>( "results.controller" ) )
		{
			if ( GetController() )
				xo::append( files, GetController()->WriteResults( file ) );
			if ( GetMeasure() )
				xo::append( files, GetMeasure()->WriteResults( file ) );
		}

		// extract specific channels for debugging / analysis
		if ( GetSconeSetting<bool>( "results.extract_channels" ) )
		{
			xo::storage< Real > sto;
			sto.resize( GetData().GetFrameCount(), 0 );
			xo::pattern_matcher match( GetSconeSetting<string>( "results.extract_channel_names" ) );
			for ( index_t idx = 0; idx < GetData().GetChannelCount(); ++idx )
				if ( const auto& label = GetData().GetLabels()[ idx ]; match( label ) )
					sto.add_channel( label, GetData().GetChannelData( idx ) );
			std::ofstream( file.str() + ".channels.txt" ) << sto;
		}

		return files;
	}

	Real Model::GetComHeight( const Vec3& up ) const
	{
		auto com = GetComPos();
		return com.y - GetProjectedOntoGround( com ).y;
	}

	Real Model::GetTotalContactForce() const
	{
		return xo::accumulate( GetLegs(), 0.0,
			[]( double v, const LegUP& l ) { return v + xo::length( l->GetContactForce() ); } );
	}

	Real Model::GetBW() const
	{
		return GetMass() * xo::length( GetGravity() );
	}

	const ContactGeometry* Model::GetGroundPlane() const
	{
		auto& cg = GetContactGeometries();
		if ( cg.size() > 0 && std::holds_alternative< xo::plane >( cg.front()->GetShape() ) )
			return cg.front().get();
		else return nullptr;
	}

	Vec3 Model::GetProjectedOntoGround( const Vec3& point, const Vec3& up ) const
	{
		if ( auto* ground = GetGroundPlane() )
		{
			auto r = xo::lined( point, -up );
			auto t = xo::transformd( ground->GetPos(), ground->GetOri() );
			return xo::intersection( r, std::get<xo::plane>( ground->GetShape() ), t );
		}
		else return point - xo::multiply( point, up ); // default projects to plane defined by origin and up
	}

	PropNode Model::GetInfo() const
	{
		PropNode pn;

		auto& model_pn = pn.add_child( "Model" );
		model_pn[ "name" ] = GetName();
		model_pn[ "mass" ] = GetMass();
		model_pn[ "gravity" ] = GetGravity();
		model_pn[ "leg count" ] = GetLegCount();

		for ( const auto& item : GetBodies() )
			pn[ "Bodies" ].add_child( item->GetName(), item->GetInfo() );

		for ( const auto& item : GetJoints() )
			pn[ "Joints" ].add_child( item->GetName(), item->GetInfo() );

		for ( const auto& item : GetActuators() )
			pn[ "Actuators" ].add_child( item->GetName(), item->GetInfo() );

		for ( const auto& item : GetDofs() )
			pn[ "Coordinates" ].add_child( item->GetName(), item->GetInfo() );

		if ( auto* c = GetController() )
			if ( auto cpn = c->GetInfo(); !cpn.empty() )
				pn.add_child( xo::get_clean_type_name( *c ), cpn );

		return pn;
	}

	void Model::AddVersionToPropNode( PropNode& pn ) const
	{
		pn.set( "scone_version", scone_version );
	}

	void Model::AddExternalDisplayGeometries( const path& model_path )
	{
		for ( const auto& b : GetBodies() )
		{
			auto geoms = b->GetDisplayGeometries();
			for ( auto& g : geoms )
				if ( xo::file_exists( model_path / g.filename_ ) )
					AddExternalResource( model_path / g.filename_ );
		}
	}

	void Model::Clear()
	{
		m_Muscles.clear();
		m_Bodies.clear();
		m_Joints.clear();
		m_Dofs.clear();
		m_Legs.clear();
		m_ContactGeometries.clear();
		m_ContactForces.clear();

		m_ActuatorPtrs.clear();
		m_RootBody = m_GroundBody = nullptr;

		m_Controller.reset();
		m_Measure.reset();
		m_Sensors.clear();
		m_SensorDelayAdapters.clear();

		m_ShouldTerminate = false;
		m_SensorDelayStorage.Clear();
		m_Data.Clear();
		m_UserData.clear();
		m_PrevStoreDataTime = 0;
		m_PrevStoreDataStep = 0;
	}
}
