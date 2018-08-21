#include "MetaReflexVirtualMuscle.h"
#include "scone/model/Model.h"
#include "scone/core/propnode_tools.h"
#include "scone/model/Muscle.h"
#include "scone/core/Exception.h"
#include "scone/model/Joint.h"

namespace scone
{
	MetaReflexVirtualMuscle::MetaReflexVirtualMuscle( const PropNode& props, Params& par, Model& model, const Locality& area ) :
		local_balance( 0 ),
		body_angvel_sensor_gain( model.balance_sensor_ori_vel_gain ),
		body_sensor_delay( model.balance_sensor_delay ),
		body_ori_sensor( nullptr ),
		body_angvel_sensor( nullptr ),
		average_moment_axis( Vec3::zero() )
	{
		// TODO: remove once a proper factory is used
		SCONE_ASSERT( props.get< String >( "type" ) == "VirtualMuscleReflex" );

		// parse target
		std::stringstream str( props.get< String >( "target" ) );
		while ( str.good() )
		{
			String dofname;
			Real weight;
			str >> dofname >> weight;
			if ( !str.fail() )
			{
				if ( HasElementWithName( model.GetDofs(), dofname ) ) // see if the dof is unsided
					dof_infos.push_back( DofInfo{ *FindByName( model.GetDofs(), dofname ), weight } );
				else // try to add a sided version of the dof
					dof_infos.push_back( DofInfo{ *FindByName( model.GetDofs(), dofname + GetSideName( area.side ) ), weight } );

				name += dofname + ( weight > 0 ? '+' : '-' ); // update name
				average_moment_axis += dof_infos.back().dof.GetRotationAxis(); // update average moment axis
			}
			else break;
		}

		// normalize average moment axis
		normalize( average_moment_axis );

		// make sure the first body has a parent
		SCONE_ASSERT( dof_infos.front().dof.GetJoint().GetParent() );

		ScopedParamSetPrefixer prefixer( par, name + "." );

		// init reflex parameters
		mrp.Init( props, par, model );

		// init balance parameters
		if ( model.GetCustomProps() && model.GetCustomProps()->get( "meta_reflex_control.use_balance", true ) && props.has_key( "Balance" ) )
		{
			ScopedParamSetPrefixer pre2( par, "B." );
			auto& balprops = props.get_child( "Balance" );
			auto& body = FindByName( model.GetBodies(), balprops.get< String >( "body" ) );

			// create sensors
			body_ori_sensor = &model.AcquireDelayedSensor< BodyOriSensor >( *body );
			body_angvel_sensor = &model.AcquireDelayedSensor< BodyAngVelSensor >( *body );

			bal_mrp.Init( balprops, par, model );

			INIT_PAR_NAMED( balprops, par, body_angvel_sensor_gain, "KAV", model.balance_sensor_ori_vel_gain );
		}

		// TODO: move to muscle
		INIT_PROP_REQUIRED( props, delay );
	}

	void MetaReflexVirtualMuscle::UpdateLocalBalance( const Vec3& global_balance )
	{
		if ( body_ori_sensor && body_angvel_sensor )
		{
			Vec3 glob_ori = Vec3( body_ori_sensor->GetValue( 0u, body_sensor_delay ), body_ori_sensor->GetValue( 1u, body_sensor_delay ), body_ori_sensor->GetValue( 2u, body_sensor_delay ) );
			Vec3 glob_angvel = Vec3( body_angvel_sensor->GetValue( 0u, body_sensor_delay ), body_angvel_sensor->GetValue( 1u, body_sensor_delay ), body_angvel_sensor->GetValue( 2u, body_sensor_delay ) );

			local_balance = dot_product( glob_ori, average_moment_axis ) + body_angvel_sensor_gain * dot_product( glob_angvel, average_moment_axis );
			Real org_lb = dot_product( global_balance, average_moment_axis );
		}
	}

	scone::Real MetaReflexVirtualMuscle::GetSimilarity( const Muscle& mus, Real tot_abs_moment_arm )
	{
		//// make sure origin and insertion are the same
		//auto& mus_ob = mus.GetOriginLink().GetBody();
		//auto& mus_ib = mus.GetInsertionLink().GetBody();
		//auto& vm_ob = dof_infos.front().dof.GetJoint().GetParent()->GetBody();
		//auto& vm_ib = dof_infos.front().dof.GetJoint().GetBody();

		//if ( vm_ob.GetName() != mus_ob.GetName() || vm_ib.GetName() != mus_ib.GetName() )
		//{
		//	log::Trace( "Mismatch: " + mus.GetName() + " and " + name + " -> " + vm_ob.GetName() + "-" + vm_ib.GetName() );
		//	return 0;
		//}

		// check if this muscle has a moment arm for all dofs
		for ( auto& di : dof_infos )
		{
			if ( !mus.HasMomentArm( di.dof ) )
				return 0;
		}

		// calculate similarity
		Real similarity = 0;
		for ( auto& di : dof_infos )
			similarity += di.w * ( mus.GetMomentArm( di.dof ) / tot_abs_moment_arm );

		return similarity;
	}
}
