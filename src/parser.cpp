#include "parser.h"
#include "arghandler.h"

int BookshelfParser::ReadFile(string file, PlaceDB &db)
{
    cout << "Reading AUX file: " << file << endl;

    ifstream out(file);
    if (!out)
    {
        cout << "\tCannot open aux file\n";
        exit(-1);
    }

    char tmp[10000];
    out.getline(tmp, 10000);

    char tmp1[500];
    char tmp2[500];
    char file_nodes[500];
    char file_nets[500];
    char file_wts[500];
    char file_pl[500];
    char file_scl[500];

    sscanf(tmp, "%s %s %s %s %s %s %s",
           tmp1, tmp2, file_nodes, file_nets, file_wts, file_pl, file_scl);

    printf("\t\t%s %s %s %s %s\n", file_nodes, file_nets, file_wts, file_pl, file_scl);

    // Read SCL first to know the row height.
    ReadSCLFile(file_scl, db); // read core-row information
    ReadNodesFile(file_nodes, db); // blocks & cell width/height
    printf("Create block name map...\n");
    db.CreateModuleNameMap();
    ReadNetsFile(file_nets, db); // read net file
    ReadPLFile(file_pl, db);     // initial module locations
    db.ClearModuleNameMap();
    return 0;
}

int BookshelfParser::ReadNodesFile(string file, PlaceDB &db)
{
    string path;
	gArg.GetString( "path", &path );
	path += file;
	ifstream in( path.c_str() );
	if( !in )
	{
		cerr << "\tCannot open nodes file: " << file << endl;
		exit(-1);
	}
	
	int nNodes, nTerminals;
	nNodes = nTerminals = -1;

	int lineNumber = 0;

	// check file format string
	char tmp[10000], tmp2[10000], tmp3[10000];
	in.getline( tmp, 10000 );
	lineNumber++;

#if 0
	if( strcmp( "UCLA nodes 1.0", tmp ) != 0 )
	{
		cerr << "Nodes file header error (not UCLA nodes 1.0)\n";
		return -1;
	}
#endif

	// check file header
	int checkFormat = 0;
	while( in.getline( tmp, 10000 ) )
	{
		lineNumber++;

		//cout << tmp << endl;
		if( tmp[0] == '#' )		continue;
		if( strncmp( "NumNodes", tmp, 8 ) == 0 )
		{
			char* pNumber = strrchr( tmp, ':' );
			nNodes = atoi( pNumber+1 );

			// 2005-12-18 (allocate mem) #donnie
			db.ReserveModuleMemory( nNodes );
			
			checkFormat++;
		}
		else if( strncmp( "NumTerminals", tmp, 12 ) == 0 )
		{
			char* pNumber = strrchr( tmp, ':' );
			nTerminals = atoi( pNumber+1 );
			checkFormat++;
		}

		if( checkFormat == 2 )
			break;

	}

	if( checkFormat != 2 )
	{
		cerr << "** Block file header error (miss NumNodes or NumTerminals)\n";
	}

	cout << "     NumNodes: " << nNodes;
	if( nNodes > 1000 )

	    cout << " (= " << nNodes / 1000 << "k)";
	cout << endl;
	cout << "    Terminals: " << nTerminals << endl;

	// Read modules and terminals.
	char name[10000];
	char type[10000];
	double w, h;
    //p double oldH = -1;
	int nReadModules = 0;
	int nReadTerminals = 0;
	while( in.getline( tmp, 10000 ) )
	{
		lineNumber++;

		if( tmp[0] == '\0' )
			continue;
		type[0] = '\0';
		sscanf( tmp, "%s %s %s %s",
					name, tmp2, tmp3, type );

		// 2005-12-05 (FARADAY testcases)
		if( strcmp( tmp2, "terminal" ) == 0 )
		{
		    nReadTerminals++;
		    db.AddModule( name, 1, 1, true );
		    continue;
		}

		w = atof( tmp2 );
		h = atof( tmp3 );

		/*if( h > db.m_rowHeight * 10 )
		{
		    printf( "    Large MACRO %s\n", name );
		}*/

#if 0
        if( oldH != -1 && h != oldH && strcmp( type, "terminal" ) != 0)
        {
            cerr << "The program cannot handle mixed-size benchmark currently.";
            exit(0);
        }
        oldH = h;
#endif

		if( strcmp( type, "terminal" ) == 0 )
		{
			nReadTerminals++;
            db.addModule( name, w, h, true );
		}
		else if( strcmp( type, "terminal_NI" ) == 0 ) // (frank) 2022-05-13 consider terminal_NI
		{
			nReadTerminals++;
			db.addModule( name, w, h, true, true );
		}
		else 
		{
			nReadModules++;
			db.addModule( name, w, h, false );
		}

		if( nNodes > stepNode && nReadModules % stepNode == 0 )
		    printf( "#%d...\n", nReadModules );
		
	}

	// check if modules number and terminal number match
	if( nReadModules+nReadTerminals != nNodes )
	{
		cerr << "Error: There are " << nReadModules << " modules in the file\n";
		exit(-1);
	}
	if( nReadTerminals != nTerminals )
	{
		cerr << "Error: There are " << nReadTerminals << " terminals in the file\n";
		exit(-1);
	}

	db.m_nModules = nNodes + nTerminals;

    // 2005/03/11
    db.m_modules.resize( db.m_modules.size() );


    // TEST: memory upper bound
    //int *ptr;
    //while( true )
    //{
    //    ptr = new int [5000000];

    //    for( int i=0; i<5000000; i++ )
    //        ptr[i] = i;

    //    cout << "*";
    //    flush(cout);
    //    sleep(1);
    //}

	// === debug ===
	//db.PrintModules();
	//db.PrintTerminals();
	// =============

	return 0;
}
