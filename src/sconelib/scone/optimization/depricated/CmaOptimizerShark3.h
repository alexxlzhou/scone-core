#pragma once

#include "CmaOptimizer.h"

namespace scone
{
	class SCONE_API CmaOptimizerShark3 : public CmaOptimizer
	{
	public:
		CmaOptimizerShark3( const PropNode& props );
		virtual ~CmaOptimizerShark3() {}

		virtual void Run() override;
	};
}
