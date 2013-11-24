/// Ready plugin for Houdini by Dan Wills.
/// Based initially on the SOP CPP Wave example from the HDK samples, and the 'rdy' commandline example from the ready codebase.

#include <UT/UT_DSOVersion.h>
#include <SYS/SYS_Math.h>
#include <GU/GU_Detail.h>
#include <GEO/GEO_AttributeHandle.h>
#include <PRM/PRM_Include.h>
#include <PI/PI_SpareProperty.h>
#include <PI/PI_EditScriptedParms.h>
#include <OP/OP_Director.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include "SOP_reactionDiffusion.h"

void SOP_reactionDiffusion::addSpareParms(const PRM_Template *spareParmTemplateList, const char *folderName )
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

void SOP_reactionDiffusion::initRenderProperties(Properties &render_settings)
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
	 cout << "Defining reaction_diffusion compiled operator type . . .\n";
     table->addOperator(new OP_Operator("reaction_diffusion",
					"Reaction Diffusion ",
					 SOP_reactionDiffusion::myConstructor,
					 SOP_reactionDiffusion::myTemplateList,
					 1,
					 1,
					 0));
	 cout << "Done.\n";
}

// SOP Parameters.
static PRM_Name	    names[] =
{
    PRM_Name("vtiFile","Vti Filename"),
	PRM_Name("writeAttribute","Write Attribute"),
    PRM_Name("startFrame","Start Frame"),
    PRM_Name("stepsPerFrame","Steps Per Frame"),
    PRM_Name("reagentR","Reagent R"),
    PRM_Name("reagentG","Reagent G"),
	PRM_Name("reagentB","Reagent B"),
	PRM_Name("setF","Set F"),
	PRM_Name("setK","Set k"),
};

int * SOP_reactionDiffusion::indexOffsets = 0;

//static PRM_Default  sopThresholdDefault(1e-03f);
static PRM_Default fileDefault(0, "/home/dan/bin/ready/Patterns/grayscott-djw/grayscott_demo_worms_moreDiffuse_256.vti");
static PRM_Default attribDefault(0, "Cd");
static PRM_Default startFrameDefault(1.0f);
static PRM_Default stepsDefault(100);
static PRM_Default reagentZeroDefault(0);
static PRM_Default reagentOneDefault(1);
static PRM_Default reagentMinusOneDefault(-1);

PRM_Template SOP_reactionDiffusion::myTemplateList[] =
{
    PRM_Template(PRM_FILE, 1, &names[0], &fileDefault),
	PRM_Template(PRM_STRING, 1, &names[1], &attribDefault),
    PRM_Template(PRM_FLT, 1, &names[2], &startFrameDefault),
    PRM_Template(PRM_INT, 1, &names[3], &stepsDefault),
    PRM_Template(PRM_INT, 1, &names[4], &reagentZeroDefault),
    PRM_Template(PRM_INT, 1, &names[5], &reagentOneDefault),
	PRM_Template(PRM_INT, 1, &names[6], &reagentMinusOneDefault),
    PRM_Template() // sentinel
};

OP_Node * SOP_reactionDiffusion::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
	cout << "RD Constructor.\n";
    return new SOP_reactionDiffusion(net, name, op);
}

SOP_reactionDiffusion::SOP_reactionDiffusion(OP_Network *net, const char *name, OP_Operator *op)
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

void SOP_reactionDiffusion::clearSpareParms()
{
	OP_Director *director = OPgetDirector();
	if ( (this->num_parameters > 0) && (this->parmNames != NULL) )
	{
		for (int parm=0;parm<this->num_parameters;parm++)
		{
			//UT_StringArray *errs; //= new UT_StringArray();
			//UT_StringArray *warn; // = new UT_StringArray();
			director->removeNodeSpareParm( this, parmNames[parm].getToken() );//, errs, warn );

		}
	}  
	//director->deleteAllNodeSpareParms( this );
}

void SOP_reactionDiffusion::updateCopyBuffersIfNeeded( bool force )
{
	//cout << "Starting updateCopyBuffersIfNeeded.\n";
	int rReagent = REAGENT_R();
	int gReagent = REAGENT_G();
	int bReagent = REAGENT_B();
	if ( force || (this->reagentR != rReagent) || (this->reagentG != gReagent) || (this->reagentB != bReagent) || (this->rd_data == NULL) )
	{
		//cout << "Need to update copyBuffersAndMap.\n";
		this->updateCopyBuffersAndMap( rReagent, gReagent, bReagent );
	}
}

void SOP_reactionDiffusion::updateCopyBuffersAndMap( int rReagent, int gReagent, int bReagent )
{
	//cout << "Starting updateCopyBuffersAndMap.\n";
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
	
	this->numCopyReagents = copyCount;
	cout << "Copying: " << this->numCopyReagents << " reagents.\n";
	
	for (int k=0; k<this->numCopyReagents; k++)
	{
		cout << "Reagent map[" << k << "] = " << reagentCopyMap[k] << ".\n";
	}
	
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

void SOP_reactionDiffusion::updateVtiFile( const char *update_file )
{
	cout << "Loading vti: " << update_file << "\n";
	if ( this->system != NULL )
	{
		delete this->system;
	}
	cout << "Deleted system.\n";
	delete this->loadedVtiName;
	cout << "Deleted loadedVtiName.\n";
    this->system = SystemFactory::CreateFromFile(update_file,this->is_opencl_available,this->opencl_platform,this->opencl_device,this->raw_context,*this->render_settings,this->warn_to_update);
	cout << "Loaded system.\n";
	this->loadedVtiName = new UT_String( update_file, true );
	cout << "loaded VTI.." << this->loadedVtiName->buffer() << "\n";
	
	this->last_cooked_frame = FLT_MIN;
	this->system->Update( 0 );
	this->step_count = 0;
	
	this->reagent_block_size = sizeof(float) * this->system->GetX() * this->system->GetY() * this->system->GetZ();
	int nreagents = this->system->GetNumberOfChemicals();
	
	cout << "Num reagents:" << nreagents << "\n";
	this->num_parameters = this->system->GetNumberOfParameters();
	
	if ( ( this->reagentCopyMap == NULL ) || (this->num_reagents != nreagents) )
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
	this->parmValues = new float[this->num_parameters];
	this->parmNames = new PRM_Name[ this->num_parameters ];
	PRM_Default parmDefaults[ this->num_parameters ];
	PRM_Template templateList[ this->num_parameters+1 ];
	
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
		this->parmNames[i] = PRM_Name( strdup(parmName.c_str()), strdup(parmName.c_str()) );
		parmDefaults[i] = PRM_Default( parmVal );
		templateList[i] = PRM_Template( PRM_FLT, 1, &this->parmNames[i], &parmDefaults[i] );
	}
	templateList[this->num_parameters] = PRM_Template(); //add sentinel terminator
	
	this->clearSpareParms();
	//add the parms
	this->addSpareParms( templateList, NULL );
		
	//cout << ".. new update file .. :" << this->loadedVtiName->buffer() << "\n";
	cout << "VTI load finished.\n";
}


SOP_reactionDiffusion::~SOP_reactionDiffusion()
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

float SOP_reactionDiffusion::float_parm( const char *name, int idx, int vidx, fpreal t )
{
	    return evalFloat( name, &indexOffsets[idx], vidx, t);
}

bool SOP_reactionDiffusion::parmChanged()
{
	for (int i=0;i<this->num_parameters;i++)
	{
		UT_String parmName = UT_String( this->parmNames[i].getToken() );
		float parmVal = float_parm( parmName.buffer(), 0, 0, 0 );
		if ( parmVal != this->parmValues[i] )
		{
			cout << "RD Parm changed.";
			return true;
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
	return false;
}

void SOP_reactionDiffusion::updateSystemFromParms()
{
	//cout << "Starting updateSystemFromParms.\n";
	if ( SOP_reactionDiffusion::parmChanged() )
	{
		//cout << "Parm changed!\n";
		for ( int i=0; i < this->num_parameters; i++ )
		{ 
			string name( this->parmNames[i].getToken() );
			UT_String UT_name = UT_String( name );
			float parmVal = float_parm( UT_name, 0, 0, 0 );
			//cout << "new value: " << parmVal << "\n";
			this->system->SetParameterValueByName( name, parmVal );
			this->parmValues[i] = parmVal;
		}
		
		this->system->QueueReloadFormula();
		//cout << "Updating copyBuffers if needed.\n";
		this->updateCopyBuffersIfNeeded( false );
	}
}

int SOP_reactionDiffusion::getReagentOffset( int baseReagentIndex )
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

OP_ERROR SOP_reactionDiffusion::cookMySop(OP_Context &context)
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
	//cout << "start frame: " << this->start_frame << "\n";
	
	fpreal frame = OPgetDirector()->getChannelManager()->getSample( context.getTime() );
	
	//cout << "frame: " << frame << "\n";
	
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
		//cout << "step_per_frame: " << step_per_frame << "\n";
		
		float frameDif = frame - start_frame;
		
		//cout << "frameDif: " << frameDif << "\n";
		
		int steps_to_do = (int)( frameDif * step_per_frame - this->step_count );
		
		//cout << "steps_to_do: " << steps_to_do << "\n";
		// think about adding a 'interactive' flag, that limtis the max_steps_to_do, temporaily, so that load on a late frame doesn't take ages.
		
		if ( steps_to_do > 0 )
		{
	        this->system->Update( steps_to_do );
			this->step_count += steps_to_do;
		}
		
		//cout << "did steps: " << steps_to_do << "\n";
		this->updateSystemFromParms();
		
		//cout << "copying N " << this->numCopyReagents << " reagents.\n";
		for (int reagent=0;reagent < this->numCopyReagents; reagent++)
		{
			/*if ( ((int(frame) % 20) == 0) || (this->start_frame == (int)frame) )
			{
				cout << "grabbing reagent " << reagent << " copy map value: " << this->reagentCopyMap[reagent] << "\n";
			}*/
			this->system->GetFromOpenCLBuffers( rd_data+reagent*this->reagent_block_size, this->reagentCopyMap[reagent] );
		}
		
		//cout << "copied n buffers: " << this->numCopyReagents << "\n";
		
		//const char *save_file = "/home/dan/test.vti";
        //system->SaveFile(save_file,*this->render_settings,false);
		
		//numReagents detail attrib
		//stepCount detail attrib
		
		GA_RWAttributeRef ptAttribRef = gdp->findAttribute(GA_ATTRIB_POINT, this->writeAttributeName->buffer() );
		
		if (!ptAttribRef.isValid())
		{
			//cout << "Attrib not found, creating it.\n";
			ptAttribRef = gdp->addFloatTuple(GA_ATTRIB_POINT, "Cd", 3, GA_Defaults(0.0));
			ptAttribRef.setTypeInfo(GA_TYPE_COLOR);
		}
		
		GA_RWHandleV3 Phandle( ptAttribRef );

	    GA_Offset ptoff;
	    GA_FOR_ALL_PTOFF(gdp, ptoff)
	    {
			//if ((ptoff % 10000) == 0)
			//{
			//	cout << "making rgb, rReagent: " << rReagent << " offset " << this->getReagentOffset(rReagent,true) << "\n";
			//	cout << "making rgb, gReagent: " << gReagent << " offset " << this->getReagentOffset(gReagent,true) << "\n";
			//	cout << "making rgb, bReagent: " << bReagent << " offset " << this->getReagentOffset(bReagent,true) << "\n";
			//}

			UT_Vector3 Pvalue = Phandle.get( ptoff );
			Pvalue.x() = ( rReagent < 0 ) ? 0 : rd_data[ptoff + this->getReagentOffset(rReagent) * this->reagent_block_size];
			Pvalue.y() = ( gReagent < 0 ) ? 0 : rd_data[ptoff + this->getReagentOffset(gReagent) * this->reagent_block_size];
			Pvalue.z() = ( bReagent < 0 ) ? 0 : rd_data[ptoff + this->getReagentOffset(bReagent) * this->reagent_block_size];

			Phandle.set( ptoff, Pvalue );
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
