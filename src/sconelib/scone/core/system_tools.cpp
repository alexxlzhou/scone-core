#include "system_tools.h"

#include "scone/core/version.h"

#include <fstream>

#include "xo/system/system_tools.h"
#include "xo/filesystem/path.h"
#include "Log.h"

#ifdef _MSC_VER
#include <shlobj.h>
#endif
#include "xo/stream/prop_node_tools.h"
#include "string"
#include "xo/utility/types.h"
#include "xo/filesystem/filesystem.h"
#include <mutex>

namespace scone
{
	std::mutex g_SystemMutex;
	PropNode g_GlobalSettings;
	String g_Version;
	path g_RootFolder;

	const PropNode& GetSconeSettings()
	{
		std::lock_guard< std::mutex > lock( g_SystemMutex );
		// lazy initialization
		if ( g_GlobalSettings.empty() )
		{
			auto settings_file = GetSettingsFolder() / "settings.ini";
			if ( xo::file_exists( settings_file ) )
			{
				log::debug( "Loaded settings from ", settings_file );
				g_GlobalSettings = xo::load_ini( settings_file );
			}
			else
			{
				// add default settings here
				auto& fn = g_GlobalSettings.push_back( "folders" );
				fn.set( "results", GetDefaultDataFolder() / "results" );
				fn.set( "models", GetDefaultDataFolder() / "models" );
				fn.set( "scenarios", GetDefaultDataFolder() / "scenarios" );
				fn.set( "geometry", GetDefaultDataFolder() / "models" / "geometry" );
			}
		}

		return g_GlobalSettings;
	}

	void SaveSconeSettings( const PropNode& newSettings )
	{
		std::lock_guard< std::mutex > lock( g_SystemMutex );
		auto settings_file = GetSettingsFolder() / "settings.ini";

		g_GlobalSettings = newSettings;
		xo::create_directories( xo::path( settings_file.str() ).parent_path() );

		xo::save_ini( g_GlobalSettings, settings_file );
		log::debug( "Saved settings to ", settings_file );
	}

	path GetInstallFolder()
	{
		if ( g_RootFolder.empty() )
		{
			for ( g_RootFolder = xo::get_application_folder(); !g_RootFolder.empty(); g_RootFolder = g_RootFolder.parent_path() )
			{
				if ( xo::exists( g_RootFolder / ".version" ) )
					break;
			}
			SCONE_THROW_IF( g_RootFolder.empty(), "Could not detect installation root folder, please run .updateversion.bat" );
			log::debug( "SCONE root folder: ", g_RootFolder );
		}
		return g_RootFolder;
	}

	path GetSettingsFolder()
	{
		return xo::get_config_folder() / "SCONE";
	}

	path GetDefaultDataFolder()
	{
		return xo::get_documents_folder() / "SCONE";
	}

	path GetFolder( const String& folder, const path& default_path )
	{
		if ( GetSconeSettings().has_key( "folders" ) )
		{
			auto path_to_folder = xo::path( GetSconeSettings().get_child( "folders" ).get< String >( folder, "" ) );
			if ( !path_to_folder.empty() )
				return path_to_folder;
		}
		return default_path;
	}

	path GetFolder( SconeFolder folder )
	{
		switch ( folder )
		{
		case scone::SCONE_ROOT_FOLDER: return GetInstallFolder();
		case scone::SCONE_RESULTS_FOLDER: return GetFolder( "results", GetDefaultDataFolder() / "results" );
		case scone::SCONE_MODEL_FOLDER: return GetFolder( "models", GetDefaultDataFolder() / "models" );
		case scone::SCONE_SCENARIO_FOLDER: return GetFolder( "scenarios", GetDefaultDataFolder() / "scenarios" );
		case scone::SCONE_GEOMETRY_FOLDER: return GetFolder( "geometry", GetDefaultDataFolder() / "models" / "geometry" );
		case scone::SCONE_UI_RESOURCE_FOLDER: return GetFolder( "ui", GetInstallFolder()/ "resources/ui" );
		default: SCONE_THROW( "Unknown folder type" );
		}
	}

	xo::version GetSconeVersion()
	{
		auto build = GetSconeBuildNumber();
		int build_nr = 0;
		if ( build != "UNKNOWN" )
			build_nr = xo::from_str< int >( build );
		return version( SCONE_VERSION_MAJOR, SCONE_VERSION_MINOR, SCONE_VERSION_PATCH, build_nr, SCONE_VERSION_POSTFIX );

	}

	String GetSconeBuildNumber()
	{
		if ( g_Version.empty() )
		{
			xo::path versionpath( xo::get_application_folder() );

			// look for .version file, up to three levels from application folder
			for ( int level = 0; level <= 3; ++level )
			{
				if ( xo::exists( versionpath / ".version" ) )
				{
					// .version file found, read its contents
					std::ifstream ifstr( ( versionpath / ".version" ).string() );
					ifstr >> g_Version;
					break;

				}
				else versionpath = versionpath / "..";
			}

			if ( g_Version.empty() ) 
				g_Version = "UNKNOWN"; // reading has failed, version unknown
		}

		return g_Version;
	}
}
