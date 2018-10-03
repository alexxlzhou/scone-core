/*
** Sensors.cpp
**
** Copyright (C) 2013-2018 Thomas Geijtenbeek. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#include "Sensors.h"
#include "Model.h"
#include "Muscle.h"
#include "Body.h"
#include "Dof.h"

namespace scone
{
	String MuscleForceSensor::GetName() const { return muscle_.GetName() + ".F"; }
	Real MuscleForceSensor::GetValue() const { return muscle_.GetNormalizedForce(); }

	String MuscleLengthSensor::GetName() const { return muscle_.GetName() + ".L"; }
	Real MuscleLengthSensor::GetValue() const { return muscle_.GetNormalizedFiberLength(); }

	String MuscleVelocitySensor::GetName() const { return muscle_.GetName() + ".V"; }
	Real MuscleVelocitySensor::GetValue() const { return muscle_.GetNormalizedFiberVelocity(); }

	String MuscleSpindleSensor::GetName() const { return muscle_.GetName() + ".S"; }
	Real MuscleSpindleSensor::GetValue() const { return muscle_.GetNormalizedSpindleRate(); }

	String MuscleExcitationSensor::GetName() const { return muscle_.GetName() + ".excitation"; }
	Real MuscleExcitationSensor::GetValue() const { return muscle_.GetExcitation(); }

	String DofPositionSensor::GetName() const { return dof_.GetName() + ".DP"; }
	Real DofPositionSensor::GetValue() const { return root_dof_ ? root_dof_->GetPos() + dof_.GetPos() : dof_.GetPos(); }

	String DofVelocitySensor::GetName() const { return dof_.GetName() + ".DV"; }
	Real DofVelocitySensor::GetValue() const { return root_dof_ ? root_dof_->GetVel() + dof_.GetVel() : dof_.GetVel(); }

	String DofPosVelSensor::GetName() const { return dof_.GetName() + ".DPV"; }
	Real DofPosVelSensor::GetValue() const {
		if ( root_dof_ )
			return root_dof_->GetPos() + dof_.GetPos() + kv_ * ( root_dof_->GetVel() + dof_.GetVel() );
		else return dof_.GetPos() + kv_ * dof_.GetVel();
	}

	String LegLoadSensor::GetName() const { return leg_.GetName() + ".LD"; }
	Real LegLoadSensor::GetValue() const { return leg_.GetLoad(); }

	String BodyPointPositionSensor::GetName() const { return body_.GetName() + ".PP"; }
	Real BodyPointPositionSensor::GetValue() const {
		return xo::dot_product( direction_, body_.GetPosOfPointOnBody( offset_ ) );
	}

	String BodyPointVelocitySensor::GetName() const { return body_.GetName() + ".PV"; }
	Real BodyPointVelocitySensor::GetValue() const {
		return xo::dot_product( direction_, body_.GetLinVelOfPointOnBody( offset_ ) );
	}

	String BodyPointAccelerationSensor::GetName() const { return body_.GetName() + ".PA"; }
	Real BodyPointAccelerationSensor::GetValue() const {
		return xo::dot_product( direction_, body_.GetLinAccOfPointOnBody( offset_ ) );
	}
}
