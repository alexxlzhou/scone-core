#include "xo/filesystem/path.h"
#include "scone/core/system_tools.h"

void simulation_test()
{
	xo::path testpath = scone::GetFolder( scone::SCONE_ROOT_FOLDER ) / "unittestdata/simulation_test";

	XO_NOT_IMPLEMENTED;

#if 0
	for ( directory_iterator dir_it( testpath ); dir_it != directory_iterator(); ++dir_it )
	{
		if ( is_directory( dir_it->path() ) )
		{
			for ( directory_iterator fileit( dir_it->path() ); fileit != directory_iterator(); ++fileit )
			{
				//cout << "Checking file " << fileit->path().string() << endl;
				if ( fileit->path().extension() == ".par" )
				{
					path fp = fileit->path();
					xo::prop_node result = scone::RunSimulation( xo::path( fp.string() ) );

					path reportpath = fp.parent_path() / ( "result_" + make_platform_id() + "_" + fp.stem().string() + ".prop" );
					if ( !exists( reportpath ) )
					{
						BOOST_ERROR( "Could not find simulation report: " + reportpath.string() );
						save_prop( result, xo::path( reportpath.string() ) );
					}
					else
					{
						auto verify = xo::load_prop( xo::path( reportpath.string() ) );
						auto rep1 = result.get_child( "result" );
						auto rep2 = verify.get_child( "result" );
						BOOST_CHECK( rep1 == rep2 );
						if ( rep1 != rep2 )
						{
							BOOST_TEST_MESSAGE( "rep1:" );
							BOOST_TEST_MESSAGE( rep1 );
							BOOST_TEST_MESSAGE( "rep2:" );
							BOOST_TEST_MESSAGE( rep2 );
						}
					}
				}
			}
		}
	}
#endif
}
