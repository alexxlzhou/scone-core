/*
** Link.cpp
**
** Copyright (C) 2013-2019 Thomas Geijtenbeek and contributors. All rights reserved.
**
** This file is part of SCONE. For more information, see http://scone.software.
*/

#include "Link.h"

#include "Body.h"
#include "Joint.h"

namespace scone
{
	String Link::ToString( const String& prefix ) const
	{
		String s = prefix + GetBody().GetName() + ( HasJoint() ? ( " (" + GetJoint().GetName() + ")\n" ) : "\n" );
		for ( auto it = m_Children.begin(); it != m_Children.end(); ++it )
			s += ( *it )->ToString( prefix + "  " );

		return s;
	}

	const Link* Link::FindLink( const String& body ) const
	{
		if ( m_Body->GetName() == body )
			return this;
		else
		{
			for ( auto it = m_Children.begin(); it != m_Children.end(); ++it )
			{
				const Link* l = ( *it )->FindLink( body );
				if ( l ) return l;
			}
			return nullptr;
		}
	}

	Link* Link::FindLink( const String& body )
	{
		return const_cast<Link*>( const_cast<const Link&>( *this ).FindLink( body ) );
	}
}
