/// Ready plugin for Houdini by Dan Wills.
/// Based initially on the SOP CPP Wave example from the HDK samples, and the 'rdy' commandline example from the ready codebase.

#include <UT/UT_DSOVersion.h>
#include <SYS/SYS_Math.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimVolume.h>
#include <GEO/GEO_AttributeHandle.h>
#include <PRM/PRM_Include.h>
#include <PI/PI_SpareProperty.h>
#include <PI/PI_EditScriptedParms.h>
#include <OP/OP_Director.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include "readyHoudini.h"

void readyHoudini::addSpareParms(const PRM_Template *spareParmTemplateList, const char *folderName )
{
        UT_String errors;
        OP_Director *director = OPgetDirector();
        PI_EditScriptedParms nodeParms(this, 1, 0);
        const PI_EditScriptedParms spareParms(this, spareParmTemplateList, true, true, false);
        nodeParms.mergeParms( spareParms );
        if (folderName) {
                int folderIdx = nodeParms.getParmIndexWithName( folderName );
                if (folderIdx) 
				{
                        int folderEndIdx = nodeParms.getMatchingGroupParm( folderIdx );
                        int myParmIdx = nodeParms.getNParms() - 1;
                        nodeParms.moveParms( myParmIdx, myParmIdx, folderEndIdx - myParmIdx );
                }
        }
		director->changeNodeSpareParms(this, nodeParms, errors);
}

void readyHoudini::initRenderProperties(Properties &render_settings)
{
    // TODO: code duplication here from frame.cpp, not sure how best to merge
	// Possible solution, some kind of getDefaultRenderSettings method that both call.
    render_settings.DeleteAllProperties();
    render_settings.AddProperty( Property("surface_color","color",1.0f,1.0f,1.0f) ); // RGB [0,1]
    render_settings.AddProperty( Property("color_low","color",0.0f,0.0f,1.0f) );
    render_settings.AddProperty( Property("color_high","color",1.0f,0.0f,0.0f) );
    render_settings.AddProperty( Property("show_color_scale",true) );
    render_settings.AddProperty( Property("show_multiple_chemicals",true) );
    render_settings.AddProperty( Property("active_chemical","chemical","a") );
    render_settings.AddProperty( Property("low",0.0f) );
    render_settings.AddProperty( Property("high",1.0f) );
    render_settings.AddProperty( Property("vertical_scale_1D",30.0f) );
    render_settings.AddProperty( Property("vertical_scale_2D",15.0f) );
    render_settings.AddProperty( Property("contour_level",0.25f) );
    render_settings.AddProperty( Property("use_wireframe",false) );
    render_settings.AddProperty( Property("show_cell_edges",false) );
    render_settings.AddProperty( Property("show_bounding_box",true) );
    render_settings.AddProperty( Property("slice_3D",true) );
    render_settings.AddProperty( Property("slice_3D_axis","axis","z") );
    render_settings.AddProperty( Property("slice_3D_position",0.5f) ); // [0,1]
    render_settings.AddProperty( Property("show_displacement_mapped_surface",true) );
    render_settings.AddProperty( Property("color_displacement_mapped_surface",true) );
    render_settings.AddProperty( Property("use_image_interpolation",true) );
    render_settings.AddProperty( Property("timesteps_per_render",100) );
	
	cout << "initRenderProperties called.\n";
}

void newSopOperator(OP_OperatorTable *table)
{
	 cout << "Defining ready_rd compiled operator type . . .\n";
     table->addOperator(new OP_Operator("ready_rd",
					"Ready RD",
					 readyHoudini::myConstructor,
					 readyHoudini::myTemplateList,
					 1,
					 1,
					 0));
	 cout << "Done.\n";
}

// SOP Parameters.
static PRM_Name parm_names[] =
{
    PRM_Name("vtiFile","Vti Filename"),
	PRM_Name("writeAttribute","Write Attribute"),
    PRM_Name("startFrame","Start Frame"),
    PRM_Name("stepsPerFrame","Steps Per Frame"),
    PRM_Name("reagentR","Reagent R"),
    PRM_Name("reagentG","Reagent G"),
	PRM_Name("reagentB","Reagent B"),
	PRM_Name("xRes","X Res"),
	PRM_Name("yRes","Y Res"),
	PRM_Name("zRes","Z Res"),
	PRM_Name("regenerateInitialState","Regenerate Initial State"),
};

int * readyHoudini::indexOffsets = 0;

//static PRM_Default  sopThresholdDefault(1e-03f);
static PRM_Default parm_defaults[] =
{
	//PRM_Default(0, "/home/dan/bin/ready/Patterns/grayscott-djw/grayscott_demo_worms_moreDiffuse_256.vti"), //file
	PRM_Default(0, "/home/dan/bin/ready/Patterns/grayscott-sharpenTweak/grayscott-evolvingMask-extra_flowyDots-liveBubbles.vti"), //file
	PRM_Default(0, "Cd"), //attribname
	PRM_Default(1.0f), // startFrame
	PRM_Default(100), //steps
	PRM_Default(2), //reagentZero
	PRM_Default(3), //reagentOne
	PRM_Default(1), //reagentMinusOne
	PRM_Default( 256 ), //xres
    PRM_Default( 256 ), //yres
    PRM_Default( 1 ), //zres
    PRM_Default( 0 ), //regenerateInitialState
};

static PRM_Type parm_types[] =
{
	PRM_FILE, // vtiFile
	PRM_STRING,//writeAttribute
	PRM_INT, //startFrame
	PRM_INT, //stepsPerFrame
	PRM_INT, //reagentR
	PRM_INT, //reagentG
    PRM_INT, //reagentB
    PRM_INT, //xRes
    PRM_INT, //yRes
    PRM_INT, //zRes
    PRM_TOGGLE, //regenerateInitialState
};            

PRM_Template readyHoudini::myTemplateList[] =
{
    PRM_Template(parm_types[0], 1, &parm_names[0], &parm_defaults[0]),
    PRM_Template(parm_types[1], 1, &parm_names[1], &parm_defaults[1]),
    PRM_Template(parm_types[2], 1, &parm_names[2], &parm_defaults[2]),
    PRM_Template(parm_types[3], 1, &parm_names[3], &parm_defaults[3]),
    PRM_Template(parm_types[4], 1, &parm_names[4], &parm_defaults[4]),
    PRM_Template(parm_types[5], 1, &parm_names[5], &parm_defaults[5]),
    PRM_Template(parm_types[6], 1, &parm_names[6], &parm_defaults[6]),
    PRM_Template(parm_types[7], 1, &parm_names[7], &parm_defaults[7]),
    PRM_Template(parm_types[8], 1, &parm_names[8], &parm_defaults[8]),
    PRM_Template(parm_types[9], 1, &parm_names[9], &parm_defaults[9]),
    PRM_Template(parm_types[10], 1, &parm_names[10], &parm_defaults[10]),
    PRM_Template() // sentinel
};

OP_Node * readyHoudini::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
	cout << "RD Constructor.\n";
    return new readyHoudini(net, name, op);
}

readyHoudini::readyHoudini(OP_Network *net, const char *name, OP_Operator *op)
	: SOP_Node(net, name, op)
{
	cout << "RD self-constructor.\n";
	this->render_settings = new Properties("render_settings");
	cout << "Init settings object..\n";
    initRenderProperties( *this->render_settings );
	cout << "Make test RD sim object from file..\n";
	// Houdini CE_Context - this will initialize Houdini OpenCL
	this->is_opencl_available = true;
    this->opencl_platform = 0; // TODO: get from houdini context
    this->opencl_device = 0; // TODO: get from houdini context
    this->warn_to_update = false; // TODO: get from houdini context
	this->loadedVtiName = new UT_String( "" );
	this->writeAttributeName = new UT_String( "" );
	this->num_parameters = 0;
	this->reagentCopyMap = NULL;
	this->rd_data = NULL;
	this->system = NULL;
	this->oldParmNames = NULL;
	this->old_num_parameters = 0;
	this->parmNames = NULL;
	this->parmValues = NULL;
    this->system_resx = -1;
    this->system_resy = -1;
    this->system_resz = -1;
	CE_Context *cecontext = CE_Context::getContext();
	// cl::Context object from the OpenCL C++ wrapper library
	cl::Context clcontext = cecontext->getCLContext();
	// raw clContext handle
	this->raw_context = clcontext();
	cout << "Got cl_context from houdini.\n";
	//const char *test_file = "/home/dan/bin/ready/Patterns/grayscott-djw/grayscott_demo_worms_moreDiffuse_256.vti";
	
    //this->updateVtiFile( test_file );
	//this->loadedVtiName = new UT_String( test_file, true );
	cout << "Constructor finished.\n";
}

//static void capitalize(char *sPtr)
//{
//	if (*sPtr != '\0') // zero length string in that case
//	{
//		//toupper the first char
//		*sPtr = toupper( (unsigned char) *sPtr );
//	}
//}

bool readyHoudini::strInPrmNameList( std::string parmName, PRM_Name* prmNameList, int listLength )
{
	if ( prmNameList != NULL )
	{
		for (int i = 0; i < listLength; i++)
		{
			//cout << "Check number " << i << " of " << listLength << " comparing: " << parmName.c_str() << " and " << prmNameList[i].getToken() << "\n";
			if ( strcmp( parmName.c_str(), prmNameList[i].getToken() ) == 0 )
			{
				//cout << "Match\n";
				return true;
			}
		}
	}// else {
	//	cout << "NULL prmNameList!\n";
	//}
	return false;
}

bool readyHoudini::parmNameInPrmNameList( PRM_Name parmName, PRM_Name* prmNameList, int listLength )
{
	if ( prmNameList != NULL )
	{
		for (int i = 0; i < listLength; i++)
		{
			//cout << "Pnipnl: Check number " << i << " of " << listLength << " comparing: " << parmName.getToken() << " and " << prmNameList[i].getToken() << "\n";
			if ( strcmp( parmName.getToken(), prmNameList[i].getToken() ) == 0 )
			{
				//cout << "Pnipnl: Match\n";
				return true;
			}
		}
	}// else {
	//	cout << "Pnipnl: NULL prmNameList!\n";
	//}
	return false;
}

bool readyHoudini::parmNameInTemplateList( PRM_Name parm, PRM_Template* prmList, int listLength )
{
	if ( prmList != NULL )
	{
		for (int i = 0; i < listLength; i++)
		{
			if ( strcmp( parm.getToken(), prmList[i].getNamePtr()->getToken() ) )
			{
				return true;
			}
		}
	}
	return false;
}

bool readyHoudini::parmTemplateInNameList( PRM_Template parm, PRM_Name* prmNameList, int listLength )
{
	if ( prmNameList != NULL )
	{
		for (int i = 0; i < listLength; i++)
		{
			if ( strcmp( parm.getNamePtr()->getToken(), prmNameList[i].getToken() ) )
			{
				return true;
			}
		}
	}
	return false;
}

void readyHoudini::clearSpareParmsNotInList( PRM_Name *oldParmNames, int numOldParms, PRM_Name* newParmNames, int numNewParms )
{
	OP_Director *director = OPgetDirector();
	
	if ( (this->num_parameters > 0) && (oldParmNames != NULL) )
	{
		for (int parm=0;parm<numOldParms;parm++)
		{
			//UT_StringArray *errs; //= new UT_StringArray();
			//UT_StringArray *warn; // = new UT_StringArray();
			
			//try not to remove and remake parms that are already there
			PRM_Name old_name = oldParmNames[parm];
			if ( !this->parmNameInPrmNameList( old_name, newParmNames, numNewParms ) )
			{
				director->removeNodeSpareParm( this, oldParmNames[parm].getToken() );//, errs, warn );
				//cout << "Parm not in new parm list, removing: " << oldParmNames[parm].getToken() << ".\n";
			} //else {
				//cout << "Parm in new parm list, skipping removal: " << oldParmNames[parm].getToken() << ".\n";
			//}
		}
	}  
	//director->deleteAllNodeSpareParms( this );
}

void readyHoudini::clearSpareParms()
{
	OP_Director *director = OPgetDirector();
	if ( (this->num_parameters > 0) && (this->parmNames != NULL) )
	{
		for (int parm=0;parm<this->num_parameters;parm++)
		{
			//UT_StringArray *errs; //= new UT_StringArray();
			//UT_StringArray *warn; // = new UT_StringArray();
			director->removeNodeSpareParm( this, this->parmNames[parm].getToken() );//, errs, warn );

		}
	}  
	//director->deleteAllNodeSpareParms( this );
}

void readyHoudini::updateCopyBuffersIfNeeded( bool force )
{
	//cout << "Starting updateCopyBuffersIfNeeded.\n";
	int rReagent = REAGENT_R();
	int gReagent = REAGENT_G();
	int bReagent = REAGENT_B();
    int xres = this->system->GetX();
	int yres = this->system->GetY();
	int zres = this->system->GetZ(); 
    
    cout << "Stored dims: (" << this->system_resx << "," << this->system_resy << "," << this->system_resz << ")\n";
    cout << "System dims: (" << xres << "," << yres << "," << zres << ")\n";
    
	if ( force || (this->reagentR != rReagent) || (this->reagentG != gReagent) || (this->reagentB != bReagent) || (this->rd_data == NULL) || (this->system_resx != xres) || (this->system_resy != yres) || (this->system_resz != zres))
	{
		cout << "Need to update copyBuffersAndMap.\n";
		this->updateCopyBuffersAndMap( rReagent, gReagent, bReagent, xres, yres, zres );
	}
}

void readyHoudini::updateCopyBuffersAndMap( int rReagent, int gReagent, int bReagent, int xres, int yres, int zres )
{
	cout << "Starting updateCopyBuffersAndMap.\n";
	int copyCount = 0;
	for ( int k=0; k<this->num_reagents; k++ )
	{
		//cout << "Examining reagent: " << k << "\n";

		//eventually turn this into a way to grab an arbirary number of reagents, just three for now.
		if ( (k==rReagent) || (k==gReagent) || (k==bReagent) )
		{
			//cout << "Found a match k: " << k << "\n";
			this->reagentCopyMap[copyCount] = k;
			copyCount += 1;
		}
	}
	
	this->reagentR = rReagent;
	this->reagentG = gReagent;
	this->reagentB = bReagent;
    
	this->system_resx = xres;
    this->system_resy = yres;
    this->system_resz = zres;
    
    int old_block_size = this->reagent_block_size;
    this->reagent_block_size = sizeof(float) * xres * yres * zres;
    
    cout << "Old/New block size: " << old_block_size << " / " << this->reagent_block_size << "\n";
    
	this->numCopyReagents = copyCount;
	// cout << "Copying: " << this->numCopyReagents << " reagents.\n";
	
	//for (int k=0; k<this->numCopyReagents; k++)
	//{
	//	cout << "Reagent map[" << k << "] = " << reagentCopyMap[k] << ".\n";
	//}
	
	if (this->numCopyReagents > 0)
	{
		delete this->rd_data;
		this->rd_data = new float[this->reagent_block_size * this->numCopyReagents];
	} else {
		//no reagents to copy, no ram to allocate!
		if (this->rd_data != NULL)
		{
			delete this->rd_data;
		}
	}
}

void readyHoudini::updateVtiFile( const char *update_file )
{
    
    cout << "Loading vti: " << update_file << "\n";
	if ( this->system != NULL )
	{
		delete this->system;
	}
	cout << "Deleted system.\n";
    this->system = SystemFactory::CreateFromFile(update_file,this->is_opencl_available,this->opencl_platform,this->opencl_device,this->raw_context,*this->render_settings,this->warn_to_update);
	cout << "Loaded system.\n";
	
	this->last_cooked_frame = FLT_MIN;
	this->system->Update( 0 );
	this->step_count = 0;
	
    
	if ( strcmp( this->loadedVtiName->buffer(), update_file ) == 0 )
    {
        cout << "Vti same.\n";//, setting dims: (" << this->system_resx << "," << this->system_resy << "," << this->system_resz << ")\n";
        //this->system->SetDimensions( this->system_resx, this->system_resy, this->system_resz ); 
    } else {
        cout << "Vti different, getting res.\n";
        this->system_resx = system->GetX();
        this->system_resy = system->GetY();
        this->system_resz = system->GetZ();
        this->setParameterOrProperty( "xRes", 0, 0, this->system_resx );
        this->setParameterOrProperty( "yRes", 0, 0, this->system_resy );
        this->setParameterOrProperty( "zRes", 0, 0, this->system_resz );
    }
    
    int oldBlockSize = this->reagent_block_size;
	this->reagent_block_size = sizeof(float) * this->system->GetX() * this->system->GetY() * this->system->GetZ();
	int nreagents = this->system->GetNumberOfChemicals();
	
	cout << "Num reagents:" << nreagents << "\n";
	this->old_num_parameters = this->num_parameters;
	//needs to get deleted
	this->oldParmNames = new PRM_Name[ this->old_num_parameters ];
	for ( int j=0; j < this->old_num_parameters; j++ )
	{
		cout << "saving old parm name: " << parmNames[j].getToken() << "\n";
		this->oldParmNames[j] = PRM_Name( strdup( parmNames[j].getToken() ), strdup( parmNames[j].getToken() ) );
	}
		
	if ( ( this->reagentCopyMap == NULL ) || (this->num_reagents != nreagents) || (this->reagent_block_size != oldBlockSize) )
	{
		cout << "(Re)creating reagentCopyMap.\n";
		delete this->reagentCopyMap;
		this->num_reagents = nreagents;
		this->reagentCopyMap = new int[this->num_reagents]; //allocate num_reagents, but only copyCount of these will be used
		cout << "Updating copy buffers (force)..\n";
		this->updateCopyBuffersIfNeeded( true );

	} else {
	
		cout << "Updating copy buffers (if needed)..\n";
		this->updateCopyBuffersIfNeeded( false );
	}
	
	//cout << "Looping parms\n";
	//this->num_parameters = this->system->GetNumberOfParameters();
	
	//need to delete these guys some more
	//if (this->parmValues != NULL ) delete this->parmValues;
	//if (this->parmNames  != NULL ) delete this->parmNames;
	
	this->num_parameters = this->system->GetNumberOfParameters();
	this->parmValues = new float[this->num_parameters];
	this->parmNames = new PRM_Name[ this->num_parameters ];
	
	//alloc room for max that we may need, may end up using less.
	PRM_Default parmDefaults[ this->num_parameters ];
	PRM_Template templateList[ this->num_parameters+1 ];
	int addedParmCount = 0;
	
	//cout << "...\n";
	for (int i=0;i<this->num_parameters;i++)
	{
		std::string parmName = this->system->GetParameterName( i );
		
		float parmVal = this->system->GetParameterValue( i );
		//record parm value so that we can know if any have changed
		this->parmValues[i] = parmVal;
		
		//cout << "RD Parameter: " << parmName << " with value: " << parmVal << "\n";
		//cout.flush();
		//char *parmNameCap = strdup( parmName.c_str() );
		//capitalize( parmNameCap );
		//const char* parmNameCapConst = parmNameCap;
		if ( !strInPrmNameList( parmName, this->oldParmNames, this->old_num_parameters ) )
		{
			this->parmNames[i] = PRM_Name( strdup(parmName.c_str()), strdup(parmName.c_str()) );
			parmDefaults[i] = PRM_Default( parmVal );
			templateList[addedParmCount] = PRM_Template( PRM_FLT, 1, &this->parmNames[i], &parmDefaults[i] );
			addedParmCount += 1;
			//cout << "Adding new parameter: " << parmName.c_str() << "\n";
		} else {
			//still add the name because the parameter is still there and that's how things are tracked
			this->parmNames[i] = PRM_Name( strdup(parmName.c_str()), strdup(parmName.c_str()) );
			//cout << "Skipping the addition of parameter: " << parmName.c_str() << "\n";
			//only reset parm values when a new file is loaded
			if ( strcmp( update_file, loadedVtiName->buffer() ) != 0 )
			{
				//cout << "Resetting parm value because vtiFileName was different:\n -->" << update_file << "\n <--" << loadedVtiName->buffer() << "\n";
				this->setParameterOrProperty( parmName.c_str(), 0, 0, parmVal );
			}
			//__"set the parameter value? yes.. or will auto-change read-in take care of that?! maybe so.
		}
	}
	//cout << "loopin' done, addedParmCount is" << addedParmCount << "\n";
	templateList[addedParmCount] = PRM_Template(); //add sentinel terminator
	
	//int numNewParms = this->countParmsToAdd( templateList, this->num_parameters-1, this->parmNames, this->num_parameters );
	
	//PRM_Template newParmList[ numNewParms ];
	//this->getParmsToAdd( newParmList, templateList, nTemplates, parmNames, nParmNames );
	
	//remove just the ones we're not about to add:
	cout << "clearning spare parms not in list...\n";
	this->clearSpareParmsNotInList( this->oldParmNames, this->old_num_parameters, this->parmNames, this->num_parameters );
	//this->clearSpareParms();
	
	if (addedParmCount > 0)
	{
		//add the (new) parms
		cout << "adding new parms...\n";
		this->addSpareParms( templateList, NULL );
	}
	delete this->loadedVtiName;
	cout << "Deleted loadedVtiName.\n";
	//cout << ".. new update file .. :" << this->loadedVtiName->buffer() << "\n";
	this->loadedVtiName = new UT_String( update_file, true );
	cout << "loaded VTI.." << this->loadedVtiName->buffer() << "\n";
	cout << "VTI update finished.\n";
}


readyHoudini::~readyHoudini()
{
	delete this->render_settings; // a Properties object
	delete this->system; // an AbstractRD-derived object
	delete this->loadedVtiName;
	delete this->parmValues;
	delete this->parmNames;
	delete this->reagentCopyMap;
	if ( this->num_reagents > 0 )
	{
		delete this->rd_data;
	}
	cout << "RD Destructor.\n";
}

float readyHoudini::float_parm( const char *name, int idx, int vidx, fpreal t )
{
	    return evalFloat( name, &indexOffsets[idx], vidx, t );
}

bool readyHoudini::parmChanged( OP_Context &context )
{
	if (this->parmNames != NULL)
	{
		for (int i=0;i<this->num_parameters;i++)
		{
			//cout << "Checking RD Parm:" << i << "\n";
			UT_String parmName = UT_String( this->parmNames[i].getToken() );
			//cout << " -- RD Parm Name:" << parmName.buffer() << "\n";
			float parmVal = float_parm( parmName.buffer(), 0, 0, context.getTime() );
			if ( parmVal != this->parmValues[i] )
			{
				cout << "RD Parm changed.\n";
				return true;
			}
		}
	}
    
	int rReagent = REAGENT_R();
	int gReagent = REAGENT_G();
	int bReagent = REAGENT_B();
	if ( (this->reagentR != rReagent) || (this->reagentG != gReagent) || (this->reagentB != bReagent) )
	{
		cout << "Reagent mapping changed.\n";
		return true;
	}
    
    int xres = X_RES();
	int yres = Y_RES();
	int zres = Z_RES();
	if ( (this->system_resx != xres) || (this->system_resy != yres) || (this->system_resz != zres) )
	{
		cout << "Sim res changed.\n";
		return true;
	}
    
	return false;
}

void readyHoudini::updateSystemFromParms( OP_Context &context )
{
	//cout << "Starting updateSystemFromParms.\n";
	if ( readyHoudini::parmChanged( context ) )
	{
		//cout << "Parm changed!\n";
		for ( int i=0; i < this->num_parameters; i++ )
		{ 
			string name( this->parmNames[i].getToken() );
			UT_String UT_name = UT_String( name );
			float parmVal = float_parm( UT_name, 0, 0, context.getTime() );
			//cout << "new value: " << parmVal << "\n";
			this->system->SetParameterValueByName( name, parmVal );
			this->parmValues[i] = parmVal;
		}
        
        //this->system_resx = X_RES();
        //this->system_resy = Y_RES();
        //this->system_resz = Z_RES();
        this->system->SetDimensions( X_RES(), Y_RES(), Z_RES() );
        
        //have a checkbox to control this:
        int regen = REGEN();
        if (regen)
        {
            this->system->GenerateInitialPattern();
        }
        
		this->system->QueueReloadFormula();
        this->system->Update(0);
		//cout << "Updating copyBuffers if needed.\n";
		this->updateCopyBuffersIfNeeded( false );
	}
}

int readyHoudini::getReagentOffset( int baseReagentIndex )
{
	//if (doprint) cout << "checking numCopyReagents: " << this->numCopyReagents << "\n";
	for ( int i=0; i<this->numCopyReagents; i++ )
	{
		//if (doprint) cout << "Checking reagent " << (i+1) << " of " << this->numCopyReagents << ".\n";
		if ( this->reagentCopyMap[i] == baseReagentIndex )
		{
			//if (doprint) cout << "found baseReagentIndex " << baseReagentIndex << " at index " << i << ".\n";
			return i;
		}
	}
	//cout << "Failed to find target offset for " << baseReagentIndex << ".\n";
	//should probably return -1 here as a kind of 'not found' code, and check it outside
	return 0;
}

OP_ERROR readyHoudini::cookMySop(OP_Context &context)
{
    // Before we do anything, we must lock our inputs.  Before returning,
    // we have to make sure that the inputs get unlocked.
    if ( lockInputs(context) >= UT_ERROR_ABORT )
	{
		return error();
	}
	
	//fpreal frame = OPgetDirector()->getChannelManager()->getSample( context.getTime() );
	// compare frame to startFrame, and if the same, reinit
	
    // Duplicate input geometry
    duplicateSource(0, context);
	// Flag the SOP as being time dependent (i.e. cook on time changes)
    flags().timeDep = 1;
	
	UT_String vtiFilename;
	VTIFILE( vtiFilename, 0 );
	
	UT_String writeAttrib;
	WRITEATTRIBNAME( writeAttrib, 0 );
	
	this->writeAttributeName = new UT_String( writeAttrib.buffer(), true );
	
	int rReagent = REAGENT_R();
	int gReagent = REAGENT_G();
	int bReagent = REAGENT_B();
	int step_per_frame = STEPS_PER_FRAME();
	this->start_frame = START_FRAME();
	
	fpreal frame = OPgetDirector()->getChannelManager()->getSample( context.getTime() );
		
	if ( ( this->start_frame == (int)frame ) || ( vtiFilename != *this->loadedVtiName ) )
	{
		//cout << "VTI needs updating.\n";
		updateVtiFile( vtiFilename.buffer() );
	}
	
	if ( this->last_cooked_frame == FLT_MIN )
	{
		cout << "last_cooked_frame INIT to: " << frame << "\n";
		this->last_cooked_frame = frame;
		//don't solve on the first frame
	}
	
	try {
		//get difference in frames between max cooked and now, and step that many frames.		
		float frameDif = frame - start_frame;		
		int steps_to_do = (int)( frameDif * step_per_frame - this->step_count );
		
		// Think about adding a 'interactive' flag, that limtis the max_steps_to_do, temporaily, so that load on a late frame doesn't take ages.
		//cout << "System res: ( " << this->system->GetX() << ", " << this->system->GetY() << ", " << this->system->GetZ() << ")\n";
        //cout << "Block size: " << this->reagent_block_size << "\n";
        
		if ( steps_to_do > 0 )
		{
	        this->system->Update( steps_to_do );
			this->step_count += steps_to_do;
		}
		
		this->updateSystemFromParms( context );
		
		for (int reagent=0;reagent < this->numCopyReagents; reagent++)
		{
			this->system->GetFromOpenCLBuffers( rd_data+reagent*this->reagent_block_size, this->reagentCopyMap[reagent] );
		}
		
		//const char *save_file = "/home/dan/test.vti";
        //system->SaveFile(save_file,*this->render_settings,false);
		
		//numReagents detail attrib
		//stepCount detail attrib
		
		
		GA_Size nprims = gdp->getNumPrimitives();
		//cout << "nprims: " << nprims << "\n";
		GEO_PrimVolume *vol = NULL;
		const GEO_Primitive *prim = NULL;
		
		if (nprims > 0)
    	{
        	GA_Offset primoff = gdp->primitiveOffset( GA_Index( 0 ) );
        	prim = gdp->getGEOPrimitive( primoff );
        	if ( prim->getTypeId() == GEO_PRIMVOLUME )
			{
            	vol = ( GEO_PrimVolume *) prim;
			}
    	}
        
		if (vol == NULL)
        {
            GA_RWAttributeRef ptAttribRef = gdp->findAttribute(GA_ATTRIB_POINT, this->writeAttributeName->buffer() );

		    if (!ptAttribRef.isValid())
		    {
			    //cout << "Attrib not found, creating it.\n";
			    ptAttribRef = gdp->addFloatTuple(GA_ATTRIB_POINT, "Cd", 3, GA_Defaults(0.0));
			    ptAttribRef.setTypeInfo(GA_TYPE_COLOR);
		    }

		    GA_RWHandleV3 Phandle( ptAttribRef );
	        GA_Offset ptoff;

		    // need to check here whether there's enough room for the copy
		    // also need to check the incoming geometry type, mainly whether it's points or voxels, and switch the copy method.
	        GA_FOR_ALL_PTOFF(gdp, ptoff)
	        {
			    UT_Vector3 Pvalue = Phandle.get( ptoff );
			    Pvalue.x() = ( rReagent < 0 ) ? 0 : rd_data[ptoff + this->getReagentOffset(rReagent) * this->reagent_block_size];
			    Pvalue.y() = ( gReagent < 0 ) ? 0 : rd_data[ptoff + this->getReagentOffset(gReagent) * this->reagent_block_size];
			    Pvalue.z() = ( bReagent < 0 ) ? 0 : rd_data[ptoff + this->getReagentOffset(bReagent) * this->reagent_block_size];

			    Phandle.set( ptoff, Pvalue );
	        }
		} else {
            //cout << "Volume prim copy mode\n";
		    // if writing to a voxel primitive
		    UT_VoxelArrayWriteHandleF handle = vol->getVoxelWriteHandle();
            handle->size(this->system_resx, this->system_resy, this->system_resz);
            
            for (int z = 0; z < this->system_resz; z++)
            {
                for (int y = 0; y < this->system_resy; y++)
                {
                    for (int x = 0; x < this->system_resx; x++)
                    {
                        unsigned long offset = x + y * this->system_resx + z * this->system_resx * this->system_resy;
                        float v = rd_data[offset + this->getReagentOffset(0) * this->reagent_block_size];
                        handle->setValue(x, y, z, v);
                    }
                }
            }

            
            /*UT_VoxelArrayF *vox;
		    UT_VoxelArrayIterator vit;
		    vit.setArray(vol);
		    for (vit.rewind(); !vit.atEnd(); vit.advance())
		    {
	            writeLoc = vit.x() + vit.y() * xSize + vit.z() * xSize * ySize;
		        // May be able to use this eventually: vit.setArray()
		        vit.setValue( rd_data[ writeLoc ] );
                //total += vit.getValue();
		    }*/
            
            //vol.setVoxels( voxArray );
        }
		//cout << "Updating system from parms.\n";
		this->last_cooked_frame = frame;
	
	} catch(const exception& e) {
        cout << "Error:\n" << e.what() << "\n";
		unlockInputs();
        return error();
    }

    //fpreal frame = OPgetDirector()->getChannelManager()->getSample( context.getTime() );
    //frame *= 0.03;

    // NOTE: If you are only interested in the P attribute, use gdp->getP(),
    //       or don't bother making a new GA_RWHandleV3, and just use
    //       gdp->getPos3(ptoff) and gdp->setPos3(ptoff, Pvalue).
    //       This just gives an example supplying an attribute name.
    //GA_RWHandleV3 Phandle(gdp->findAttribute(GA_ATTRIB_POINT, "P"));
    //GA_Offset ptoff;
    //GA_FOR_ALL_PTOFF(gdp, ptoff)
    //{
	//	UT_Vector3 Pvalue = Phandle.get(ptoff);
	//	Pvalue.y() = sin( Pvalue.x() * 0.2 + Pvalue.z() * 0.3 + frame);
	//	Phandle.set(ptoff, Pvalue);
    //}

    unlockInputs();
    return error();
}
