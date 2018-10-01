/*
** Sensor.h
**
** Copyright (C) 2013-2018 Thomas Geijtenbeek. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#pragma once

#include "scone/core/PropNode.h"
#include "scone/optimization/Params.h"
#include "scone/core/Exception.h"
#include "scone/core/types.h"

namespace scone
{
	class SCONE_API Sensor
	{
	public:
		Sensor();
		Sensor( const PropNode& pn, Params& par, class Model& model, const class Location& target_area );
		virtual ~Sensor();

		virtual Real GetValue() const { SCONE_THROW_NOT_IMPLEMENTED; }
		virtual String GetName() const = 0;

		// special case for multichannel sensors
		virtual size_t GetChannelCount() { return 1; }
		virtual Real GetValue( index_t idx ) const { SCONE_ASSERT( idx == 0 ); return GetValue(); }
	};
}
