/*
** JointOpenSim3.h
**
** Copyright (C) 2013-2018 Thomas Geijtenbeek. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#pragma once

#include "platform.h"
#include "scone/model/Joint.h"

namespace OpenSim
{
	class Joint;
}

namespace scone
{
	class SCONE_OPENSIM_3_API JointOpenSim3 : public Joint
	{
	public:
		JointOpenSim3( Body& body, Joint* parent, class ModelOpenSim3& model, OpenSim::Joint& osJoint );
		virtual ~JointOpenSim3();

		virtual const String& GetName() const;

		virtual Vec3 GetPos() const override;
		virtual size_t GetDofCount() const override;
		virtual Real GetDofValue( size_t index = 0 ) const override;
		virtual const String& GetDofName( size_t index = 0 ) const override;

		class ModelOpenSim3& m_Model;
		OpenSim::Joint& m_osJoint;

		virtual Vec3 GetReactionForce() const override;
	};
}
