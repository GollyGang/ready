/*  Copyright 2011-2018 The Ready Bunch

    This file is part of Ready.

    Ready is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ready is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ready. If not, see <http://www.gnu.org/licenses/>.         */

// STL:
#include <iostream>
using namespace std;

// Boost:
#include <boost/program_options.hpp>
namespace po = boost::program_options;

// stdlib:
#include <stdlib.h>

// readybase:
#include <SystemFactory.hpp>
#include <Properties.hpp>
#include <OpenCL_utils.hpp>
#include <AbstractRD.hpp>
#include <OpenCLImageRD.hpp>

// -------------------------------------------------------------------------------------------------------------

void InitializeDefaultRenderSettings(Properties &render_settings);

// Please keep the following in sync with the blurb below!:
// -------------------------------------------------------------------------------------------------------------
/*
        Ready commandline utility that uses readybase as a processing (and data-retrieval) back-end.

        It responds to various commandline arguments to load and print out parts of the system,
        and can also still be used (like it used to work) to iterate the system to a specified number
        of steps, and save it to a new vti file.
        
        The various print options can facilitate the import of ready simulations into other applications, (such 
        as Houdini), without the need to actually link the ready libraries. This includes reagent initial-states,
        (via -m), be ready for some large lumps of text on stdout when using that argument.
        
        Please let the Ready team (especially Dan Wills) know if there is something that you wish to print 
        that currently isn't supported.
*/
// -------------------------------------------------------------------------------------------------------------
void printSeparator()
{
    cout << "================================\n";
}

int main(int argc,char *argv[])
{
    // Declare the (boost/program_options) supported options.
    po::options_description desc("Options");
    
    std::string vti_in;
    std::string vti_out;
    
    std::string blurb = "Ready commandline utility that uses readybase as a processing (and data-retrieval) back-end.\n"
                        "\n"
                        "It responds to various commandline arguments to load and print out parts of the system,\n"
                        "and can also still be used (like it used to work) to iterate the system to a specified number\n"
                        "of steps, and save it to a new vti file.\n"
                        "\n"
                        "The various print options can facilitate the import of ready simulations into other applications, (such\n"
                        "as Houdini), without the need to actually link the ready libraries. This includes reagent initial-states,\n"
                        "(via -m), be ready for some large lumps of text on stdout when using that argument.\n"
                        "\n"
                        "Please let the Ready team (especially Dan Wills) know if there is something that you wish to print\n"
                        "that currently isn't supported.\n";
    
    desc.add_options()
        ("help,h", "Print the help message")
        // Went to 0 iterations by default. Need to provide option for flags
        ("num-iterations,n", po::value<int>()->default_value(0), "Number of iterations to run before saving")
        ("print-kernel,k", po::bool_switch()->default_value(false), "Print (full) OpenCL kernel source (when possible)")
        ("print-formula,f", po::bool_switch()->default_value(false), "Print kernel formula (when possible)")
        ("print-rule-info,u", po::bool_switch()->default_value(false), "Print rule info")
        ("print-reagent-info,r", po::bool_switch()->default_value(false), "Print reagent info")
        ("print-parameter-info,p", po::bool_switch()->default_value(false), "Print parameter info")
        ("print-render-settings,s", po::bool_switch()->default_value(false), "Print render Settings")
        ("print-formula-description,d", po::bool_switch()->default_value(false), "Print formula Description")
        ("print-initial-state-images,m", po::bool_switch()->default_value(false), "Print initial state images (Warning: May be large!)")
        ("vti-in,i", po::value(&vti_in)->required(), "VTI file to load (required)")
        ("vti-out,o", po::value(&vti_out), "VTI file to save (optional)")
        // TODO don't crash if incorrect, fail more gracefully!
        ("opencl-platform,l", po::value<int>(), "OpenCL platform number (Currently will crash if incorrect!)")
        ("opencl-device,g", po::value<int>(), "OpenCL device number (Currently will crash if incorrect!)")
        ("verbose,v", po::bool_switch()->default_value(false), "Verbose output.");
  
    po::variables_map vm;
    try {
        po::store( po::parse_command_line( argc, argv, desc ), vm );
        po::notify( vm );
    } catch ( const exception& e ) {
        if ( vm.count("help") == 0 )
        {
            cout << "\nIt looks like one of the required arguments is missing or malformed:\n";
            cout << e.what() << "\n\n";
            cout << "Here's the help:\n";
        } else {
            cout << "\n" << blurb << "\n";
            cout << "\n" << desc << "\n";
            return 1;
        }
    }
    
    int numiter = 1000;
    bool print_kernel = false;
    bool print_formula = false;
    bool print_rule_info = false;
    bool print_reagent_info = false;
    bool print_parameter_info = false;
    bool print_render_settings = false;
    bool print_initial_state_images = false;
    bool print_formula_description = false;
    bool verbose = false;
    int opencl_platform = 0;
    int opencl_device = 0;
    
    if ( vm.count("help") )
    {
        cout << "\n" << blurb << "\n";
        cout << "\n" << desc << "\n";
        return 1;
    }

    if ( vm.count("verbose") )
    {
        verbose = vm["verbose"].as<bool>();
        if (verbose)
        {
            cout << "Verbose was enabled.\n";
        }
    }
    
    if ( vm.count("num-iterations") )
    {
        numiter = vm["num-iterations"].as<int>();
        if (verbose) 
        {
            cout << "Num Iterations was set to: " << vm["num-iterations"].as<int>() << ".\n";
        }
    } else {
        if (verbose) 
        {
            cout << "Num Iterations was not set (default is 1000).\n";
        }
    }
    
    if ( vm.count("print-kernel") )
    {
        print_kernel = vm["print-kernel"].as<bool>();
    }
    
    if ( vm.count("print-formula") )
    {
        print_formula = vm["print-formula"].as<bool>();
    }
    
    if ( vm.count("print-rule-info") )
    {
        print_rule_info = vm["print-rule-info"].as<bool>();
    }
    
    if ( vm.count("print-reagent-info") )
    {
        print_reagent_info = vm["print-reagent-info"].as<bool>();
    }
    
    if ( vm.count("print-parameter-info") )
    {
        print_parameter_info = vm["print-parameter-info"].as<bool>();
    }
    
    if ( vm.count("print-render-settings") )
    {
        print_render_settings = vm["print-render-settings"].as<bool>();
    }
    
    if ( vm.count("print-formula-description") )
    {
        print_formula_description = vm["print-formula-description"].as<bool>();
    }
    
    if ( vm.count("print-initial-state-images") )
    {
        print_initial_state_images = vm["print-initial-state-images"].as<bool>();
    }
    
    if ( vm.count("vti-in") )
    {
        if (verbose) 
        {
            cout << "VTI-in flag detected!\n";
        }
    }
    
    if ( vm.count("vti-out") )
    {
        if (verbose) 
        {
            cout << "VTI-out flag detected!\n";
        }
    }
    
    if ( vm.count("opencl-platform") )
    {
        opencl_platform = vm["opencl-platform"].as<int>();
        if (verbose) 
        {
            cout << "OpenCl platform was set to: " << opencl_platform << ".\n";
        }
    }
    
    if ( vm.count("opencl-device") )
    {
        opencl_device = vm["opencl-device"].as<int>();
        if (verbose) 
        {
            cout << "OpenCl device was set to: " << opencl_device << ".\n";
        }
    }
    
    bool is_opencl_available = OpenCL_utils::IsOpenCLAvailable();
    
    if( is_opencl_available )
    {
        if (verbose)
        {
            cout << "OpenCL found.\n";
        }
    } else {
        // Still print (despite not verbose) since it's a warning:
        cout << "Warning: OpenCL not found! (This may not bode well for what's about to happen..).\n";
    }
    
    Properties render_settings("render_settings");
    InitializeDefaultRenderSettings(render_settings);

    AbstractRD *system;
    try 
    {
        // Read the file
        if (verbose)
        {
            cout << "Loading VTI file: " << vti_in << "...\n";
        }
        bool warn_to_update;
        try {
            system = SystemFactory::CreateFromFile( vti_in.c_str(), is_opencl_available, opencl_platform, 
                                                    opencl_device, render_settings, warn_to_update );
            if (verbose)
            {
                cout << "Loaded VTI: " << vti_in.c_str() << "\n";
            }
            
            system->Update( 0 );
            if (verbose)
            {
                cout << "System updated to zeroth step..\n";
            }
                            
            if ( print_reagent_info ) 
            {
                int num_chemicals = system->GetNumberOfChemicals();
                int xres = system->GetX();
                int yres = system->GetY();
                int zres = system->GetZ();
                int arena_dimensionality = system->GetArenaDimensionality();
                cout << "\n";
                cout << "Reagent info:\n";
                printSeparator();
                //cout << "================================\n";
                cout << "num_chemicals=" << num_chemicals << "\n";
                cout << "arena_dimensionality=" << arena_dimensionality << "\n";
                cout << "xres=" << xres << "\n";
                cout << "yres=" << yres << "\n";
                cout << "zres=" << zres << "\n";
                cout << "================================\n";
            }

            if ( print_kernel ) 
            {
                cout << "\n";
                cout << "Kernel source:\n";
                cout << "================================\n";
                cout << system->GetKernel();
                cout << "================================\n";
            }
            
            if ( print_formula ) 
            {
                cout << "\n";
                cout << "Kernel formula:\n";
                cout << "================================\n";
                // Maybe work out how to strip leading whitespace from lines in this:
                cout << system->GetFormula();
                cout << "================================\n";
            }
            
            if ( print_rule_info )
            {
                cout << "\n";
                cout << "Rule info:\n";
                cout << "================================\n";
                cout << "type=" << system->GetRuleType() << "\n";
                cout << "name=" << system->GetRuleName() << "\n";
                cout << "neighborhood_type=" << system->GetNeighborhoodType() << "\n";
                // next one is an int!
                cout << "neighborhood_range=" << system->GetNeighborhoodRange() << "\n";
                cout << "neighborhood_weight=" << system->GetNeighborhoodWeight() << "\n";
                cout << "================================\n";
            }
            if ( print_parameter_info )
            {
                int nparams = system->GetNumberOfParameters();
                
                cout << "\n";
                cout << "Parameter info:\n";
                cout << "================================\n";
                for ( int ix=0; ix < nparams; ix++ )
                {
                    std::string parm_name = system->GetParameterName( ix );
                    float parm_value = system->GetParameterValue( ix );
                    cout << parm_name << "=" << parm_value << "\n";
                }
                cout << "================================\n";
            }
            
            if ( print_render_settings )
            {
                cout << "\n";
                cout << "Render settings:\n";
                cout << "================================\n";
                
                // A good example cast, left as a relic:
                //AbstractRD* rdPlayer = dynamic_cast<AbstractRD*>(system);
                //FormulaOpenCLImageRD* rdPlayer = dynamic_cast<FormulaOpenCLImageRD*>(system);                
                //Properties *render_settings = rdPlayer->render_settings;
                
                int num_properties = render_settings.GetNumberOfProperties();
                cout << "Number of properties is: " << num_properties << "\n";
                for (int i=0;i<num_properties;i++)
                {
                    Property this_property = render_settings.GetProperty( i );
                    
                    std::string property_name = this_property.GetName();
                    std::string property_type = this_property.GetType();
                    
                    cout << "    name: " << property_name << ", type: " << property_type << ", value:";
                    if (property_type == "int")
                    {
                        int property_value = this_property.GetInt();
                        cout << " " << property_value << "\n";
                    } else if (property_type == "bool") {
                        bool property_value = this_property.GetBool();
                        cout << " " << property_value << "\n";
                    } else if (property_type == "float") {
                        float property_value = this_property.GetFloat();
                        cout << " " << property_value << "\n";
                    } else if (property_type == "color") {
                        float property_value_r, property_value_g, property_value_b;
                        this_property.GetColor( property_value_r, property_value_g, property_value_b);
                        cout << " (" << property_value_r << "," << property_value_g << "," << property_value_b << ")\n";
                    } else if (property_type == "chemical") {
                        std::string property_value = this_property.GetChemical();
                        cout << " " << property_value << "\n";
                    } else if (property_type == "axis") {
                        std::string property_value = this_property.GetAxis();
                        cout << " " << property_value << "\n";
                    }
                }
                cout << "================================\n";
            }
            
            if ( print_formula_description )
            {
                cout << "\n";
                cout << "Formula description:\n";
                cout << "================================\n";
                cout << system->GetDescription();
                cout << "================================\n";
            }
            
            if ( print_initial_state_images )
            {
                int num_chemicals = system->GetNumberOfChemicals();
                cout << "\n";
                cout << "Parameter initial-state images:\n";
                cout << "================================\n";
                // I'm just going to gloss over numberOfScalarComponents for the moment! (just hope it's always 1 or above!)
                // so given that, in theory num_chemicals and nc should be equal!
                
                // Update to get everything initialised (and copy in the initial states)..
                system->Update(0);
                
                for ( int ix=0; ix < num_chemicals; ix++ )
                {
                    cout << "\n";
                    cout << "nchem: " << ix << "\n";
                    OpenCLImageRD *isystem = dynamic_cast< OpenCLImageRD* >(system);
                    
                    cout << "xres=" << system->GetX() << "\n";
                    cout << "yres=" << system->GetY() << "\n";
                    cout << "zres=" << system->GetZ() << "\n";
                    
                    // not in bytes!
                    unsigned long reagent_block_size = system->GetX() * system->GetY() * system->GetZ();
                    
                    cout << "Reagent block size is: " << reagent_block_size << "\n";
                    float *rd_data = new float[reagent_block_size];
                    
                    isystem->GetFromOpenCLBuffers( rd_data, ix );
                    
                    cout << "\nRD data for reagent " << ix << ": [ ";
                    for (unsigned long rix=0; rix<reagent_block_size; rix++)
                    {
                        cout << rd_data[rix];
                        if ( rix < reagent_block_size-1 )
                        {
                            cout << ",";
                        }
                    }
                    cout << " ]\n";
                    delete rd_data;
                }
                cout << "================================\n";
            }
        } catch(const exception& e) {
            cout << "Error creating system!:\n" << e.what() << "\n"; 

            /* TODO: find equivalent of backtrace that works on all supported platforms
            void* callstack[128];
            int i, frames = backtrace( callstack, 128 );
            char** strs = backtrace_symbols( callstack, frames );
            for (i = 0; i < frames; ++i) 
            {
                printf( "%s\n", strs[i] );
            }
            free(strs);
            */
            cout << "Ignoring exception.\n";
        }
        
        if( warn_to_update )
            cout << "This pattern was created with a newer version of Ready. You should update your copy.\n";
        
        if ( numiter > 0 )
        {
            cout << "Run the simulation for " << numiter << " steps...\n";
            system->Update( numiter );

            if ( vm.count("vti-out") )
            {
                // save something out
                cout << "Saving file as " << vti_out << " ...\n";
                try {
                    system->SaveFile( vti_out.c_str(), render_settings, false );
                } catch(const exception& e) { //doesn't catch segfaults! :/
                    cout << "Something went wrong when saving file to: " << vti_out.c_str() << "\n";
                    cout << e.what() << "\n";
                }
            } else {
                cout << "Output file not specified, not saving anything.\n";
            }
        } else { 
            if (verbose)
            {
                cout << "Zero iterations, simulation skipped.\n";
            }
        }
    }
    catch(const exception& e)
    {
        cout << "Error:\n" << e.what() << "\n";
        return EXIT_FAILURE;
    }

    delete system;
    return EXIT_SUCCESS;
}

// -------------------------------------------------------------------------------------------------------------

void InitializeDefaultRenderSettings(Properties &render_settings)
{
    render_settings.SetDefaultRenderSettings();
}

// -------------------------------------------------------------------------------------------------------------
