#include "MetaReflexController.h"

#include "scone/model/Locality.h"
#include "scone/model/Model.h"
#include "scone/model/Dof.h"
#include "scone/model/Muscle.h"
#include "scone/core/HasName.h"
#include "scone/core/Factories.h"

#include "MetaReflexDof.h"
#include "MetaReflexMuscle.h"
#include "scone/core/propnode_tools.h"
#include <memory>

namespace scone
{
	MetaReflexController::MetaReflexController( const PropNode& props, Params& par, Model& model, const Locality& area ) :
		Controller( props, par, model, area )
	{
		bool symmetric = props.get< bool >( "symmetric", true );
		SCONE_ASSERT( symmetric == true ); // only symmetric controllers work for now

		// create Meta Reflexes
		const PropNode& reflexes = props.get_child( "Reflexes" );
		for ( const auto& item : reflexes )
		{
			if ( item.second.get< String >( "type" ) == "MetaReflex" )
			{
				// check if the target dof is sided
				// TODO: see if we can come up with something nicer here...
				const String& target_dof = item.second.get< String >( "target" );
				SCONE_ASSERT( GetSideFromName( target_dof ) == NoSide ); // must be symmetric
				if ( HasElementWithName( model.GetDofs(), target_dof ) )
				{
					// this is a dof with no sides: only create one controller
					m_ReflexDofs.push_back( MetaReflexDofUP( new MetaReflexDof( item.second, par, model, Locality( NoSide ) ) ) );
				}
				else
				{
					// this is a dof that has sides (probably), create a controller that matches the Area side
					SCONE_ASSERT( area.side != NoSide );
					m_ReflexDofs.push_back( MetaReflexDofUP( new MetaReflexDof( item.second, par, model, area ) ) );
				}
			}
			else if ( item.second.get< String >( "type" ) == "VirtualMuscleReflex" )
			{
				m_VirtualMuscles.push_back( MetaReflexVirtualMuscleUP( new MetaReflexVirtualMuscle( item.second, par, model, area ) ) );
			}
			else SCONE_THROW( "Invalid MetaReflex type: " + item.second.get< String >( "type " ) );
		}

		// backup the current state
		State org_state = model.GetState();

		// reset all dofs to ensure consistency when there are unspecified dofs
		for ( DofUP& dof : model.GetDofs() )
		{
			dof->SetPos( 0, false );
			dof->SetVel( 0 );
		}

		// now set the DOFs
		// TODO: include mirror_left variable!
		// TODO: SetPos should use Radian as input parameter
		for ( MetaReflexDofUP& mr : m_ReflexDofs )
			mr->target_dof.SetPos( Radian( Degree( mr->dof_pos.ref_pos ) ).value, false );

		// set target dof rotation axes (required for local balance)
		for ( MetaReflexDofUP& mr : m_ReflexDofs )
			mr->SetDofRotationAxis();

		// Create meta reflex muscles
		for ( MuscleUP& mus : model.GetMuscles() )
		{
			if ( GetSideFromName( mus->GetName() ) == area.side )
			{
				MetaReflexMuscleUP mrm = MetaReflexMuscleUP( new MetaReflexMuscle( *mus, model, *this, area ) );
				if ( mrm->dof_infos.size() > 0 || mrm->vm_infos.size() > 0 ) // only keep reflex if it crosses any of the relevant dofs
					m_ReflexMuscles.push_back( std::move( mrm ) );
			}
		}

		// init meta reflex control parameters
		for ( MetaReflexMuscleUP& mrm : m_ReflexMuscles )
			mrm->UpdateMuscleControlParameters( true );

		// restore original state
		model.SetState( org_state, 0 );
	}

	MetaReflexController::~MetaReflexController()
	{
	}

	MetaReflexController::UpdateResult MetaReflexController::UpdateControls( Model& model, double timestamp )
	{
		// get balance
		Vec3 global_balance = model.GetDelayedOrientation();

		for ( MetaReflexDofUP& mrdof : m_ReflexDofs )
			mrdof->UpdateLocalBalance( global_balance ); // TODO: perhaps not every time?

		for ( MetaReflexVirtualMuscleUP& vm : m_VirtualMuscles )
			vm->UpdateLocalBalance( global_balance ); // TODO: perhaps not every time?

		for ( MetaReflexMuscleUP& mrmus : m_ReflexMuscles )
		{
			mrmus->UpdateMuscleControlParameters(); // TODO: perhaps not every time?
			mrmus->UpdateControls();
		}

		return SuccessfulUpdate;
	}

	String MetaReflexController::GetClassSignature() const
	{
		// count reflex types
		int l = 0, c = 0, f = 0, s = 0;
		for ( const MetaReflexMuscleUP& r : m_ReflexMuscles )
		{
			if ( r->length_gain != 0.0 ) ++l;
			if ( r->constant != 0.0 ) ++c;
			if ( r->force_gain != 0.0 ) ++f;
			if ( r->stiffness != 0.0 ) ++s;
		}

		String str = "M";
		if ( l > 0 ) str += "L";
		if ( c > 0 ) str += "C";
		if ( f > 0 ) str += "F";
		if ( s > 0 ) str += "S";

		return str;
	}

	void MetaReflexController::StoreData( Storage< Real >::Frame& frame, const StoreDataFlags& flags ) const
	{
		for ( auto& mr : m_ReflexDofs )
			mr->StoreData( frame, flags );
	}
}
