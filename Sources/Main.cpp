/*
 *  Main.cpp
 */

#define VERSION "0.9.2 Beta"

#include "BattleTechGame.h"
#include "BattleTechServer.h"
#include "ServerConsole.h"

#ifdef WIN32
#include <ios>
#endif


// Set 64-bit DLL directory to Bin64 and make sure the working directory is correct.
// This must happen before main, or DLLs will fail to link!
bool pre_main_result = Raptor::PreMain( (sizeof(void*) * 8 != 32) ? (sizeof(void*) * 8) : 0 );


int main( int argc, char **argv )
{
	bool dedicated = false;
	if( (argc >= 2) && (strcmp( argv[ 1 ], "-dedicated" ) == 0) )
		dedicated = true;
	
	#ifdef WIN32
		bool terminal = false;
		if( (argc >= 2) && (strcmp( argv[ 1 ], "-terminal" ) == 0) )
			terminal = true;
		
		// Create a Windows command-prompt-style console window.
		if( (terminal || dedicated) && AllocConsole() )
		{
			freopen( "CONIN$", "rt", stdin );
			freopen( "CONOUT$", "wt", stderr );
			freopen( "CONOUT$", "wt", stdout );
			std::ios::sync_with_stdio();
		}
	#endif
	
	Raptor::Game = new BattleTechGame( VERSION );
	Raptor::Server = new BattleTechServer( VERSION );
	Raptor::Game->Console.OutFile = stdout;
	Raptor::Game->SetServer( Raptor::Server );
	
	if( ! dedicated )
	{
		Raptor::Game->Initialize( argc, argv );
		Raptor::Game->Run();
	}
	else
	{
		Raptor::Server->Console = new ServerConsole();
		Raptor::Server->Console->OutFile = stdout;
		Raptor::Server->Data.Properties[ "permissions" ] = "all";
		Raptor::Server->Start( "Dedicated Server" );
		((ServerConsole*)( Raptor::Server->Console ))->Run();
	}
	
	return 0;
}
