//============================================//
// Includes
//============================================//

#include <lifev/core/LifeV.hpp>

// Passive material
#include <lifev/electrophysiology/solver/ElectroETAMonodomainSolver.hpp>
#include <lifev/electrophysiology/util/HeartUtility.hpp>
#include <lifev/structure/solver/StructuralConstitutiveLawData.hpp>
#include <lifev/em/solver/mechanics/EMStructuralOperator.hpp>
#include <lifev/em/solver/mechanics/EMStructuralConstitutiveLaw.hpp>
#include <lifev/bc_interface/3D/bc/BCInterface3D.hpp>

// Solver
#include <lifev/em/solver/EMSolver.hpp>

// Exporter
#include <lifev/core/filter/ExporterEnsight.hpp>
#include <lifev/core/filter/ExporterVTK.hpp>
#ifdef HAVE_HDF5
#include <lifev/core/filter/ExporterHDF5.hpp>
#endif
#include <lifev/core/filter/ExporterEmpty.hpp>

// Resize mesh
#include <lifev/core/mesh/MeshUtility.hpp>

// B.C. modification
#include <lifev/core/fem/BCVector.hpp>

// Circulation
#include <lifev/em/solver/circulation/Circulation.hpp>

// Volume computation
#include <lifev/em/solver/circulation/CirculationVolumeIntegrator.hpp>

// Heart solver
#include <lifev/em/solver/HeartSolver.hpp>

// Track nan
// #include <fenv.h>





// Namespaces
using namespace LifeV;


//============================================//
// Functions
//============================================//




Real patchForce (const Real& t, const Real& Tmax, const Real& tmax, const Real& tduration)
{
    bool time ( fmod(t-tmax+0.5*tduration, 700.) < tduration && fmod(t-tmax+0.5*tduration, 700.) > 0);
    Real force = std::pow( std::sin(fmod(t-tmax+0.5*tduration, 700.)*3.14159265359/tduration) , 2 ) * Tmax;
    return ( time ? force : 0 );
}

Real patchFunction (const Real& t, const Real&  X, const Real& Y, const Real& Z, const ID& /*i*/)
{
    Real disp = std::pow( std::sin(fmod(t, 700.) * 3.14159265359/300) , 2 )*15;
    return disp;
}

Real Iapp (const Real& t, const Real&  X, const Real& Y, const Real& Z, const ID& /*i*/)
{
    bool coords ( Y < -7. );
    //bool coords ( Y > 4. ); //( Y > 1.5 && Y < 3 );
    bool time ( fmod(t, 700.) < 4 && fmod(t, 700.) > 2);
    return ( coords && time ? 30 : 0 );
}

Real potentialMultiplyerFcn (const Real& t, const Real&  X, const Real& Y, const Real& Z, const ID& /*i*/)
{
    bool time ( fmod(t, 700.) < 4 && fmod(t, 700.) > 2);
    return 1.4 * time; // ( Y < 2.5 && Y > 0.5 ? 1.0 : 0.0 );
}


int main (int argc, char** argv)
{

    //feenableexcept(FE_INVALID | FE_OVERFLOW);
    
    
    //============================================//
    // Typedefs
    //============================================//
    
    typedef RegionMesh<LinearTetra>                         mesh_Type;
    typedef boost::shared_ptr<mesh_Type>                    meshPtr_Type;
    
    typedef boost::function < Real (const Real & t,
                                    const Real &   x,
                                    const Real &   y,
                                    const Real & z,
                                    const ID&   /*i*/ ) >   function_Type;
    
    typedef VectorEpetra                                    vector_Type;
    typedef boost::shared_ptr<vector_Type>                  vectorPtr_Type;
    
    typedef BCVector                                        bcVector_Type;
    typedef boost::shared_ptr<bcVector_Type>                bcVectorPtr_Type;
    
    typedef EMMonodomainSolver<mesh_Type>                   monodomain_Type;
    typedef EMSolver<mesh_Type, monodomain_Type>            emSolver_type;

    typedef FESpace< mesh_Type, MapEpetra >                 solidFESpace_Type;
    typedef boost::shared_ptr<solidFESpace_Type>            solidFESpacePtr_Type;
    
    typedef ETFESpace< mesh_Type, MapEpetra, 3, 3 >         solidETFESpace_Type;
    typedef boost::shared_ptr<solidETFESpace_Type>          solidETFESpacePtr_Type;
    
    typedef StructuralConstitutiveLawData                   constitutiveLaw_Type;
    typedef boost::shared_ptr<constitutiveLaw_Type>         constitutiveLawPtr_Type;
    
    typedef BCHandler                                       bc_Type;
    typedef StructuralOperator< mesh_Type >                 physicalSolver_Type;
    
    typedef BCInterface3D< bc_Type, physicalSolver_Type >   bcInterface_Type;
    typedef boost::shared_ptr< bcInterface_Type >           bcInterfacePtr_Type;
    

    //============================================//
    // Communicator and displayer
    //============================================//
#ifdef HAVE_MPI
    MPI_Init ( &argc, &argv );
#endif

    boost::shared_ptr<Epetra_Comm>  comm ( new Epetra_MpiComm (MPI_COMM_WORLD) );
    
    Displayer displayer ( comm );
    displayer.leaderPrint ("\nUsing MPI\n");

    
    //============================================//
    // Create HeartSolver object
    //============================================//
    HeartSolver heartSolver (displayer);
    
    
    //============================================//
    // Read data file and create output folder
    //============================================//
    GetPot command_line (argc, argv);
    const std::string data_file_name = command_line.follow ("data", 2, "-f", "--file");
    GetPot dataFile (data_file_name);
    std::string problemFolder = EMUtility::createOutputFolder (command_line, *comm);

    heartSolver.readData(dataFile);
    
    
    //============================================//
    // Electromechanic solver
    //============================================//
    emSolver_type& solver = heartSolver.emSolver();
    
    
    //============================================//
    // Body circulation
    //============================================//
    const std::string circulationInputFile = command_line.follow ("circulation", 2, "-cif", "--cifile");
    const std::string circulationOutputFile = command_line.follow ( (problemFolder + "solution.dat").c_str(), 2, "-cof", "--cofile");
    
    heartSolver.loadCirculation(circulationInputFile);
    
    Circulation& circulationSolver = heartSolver.circulation();
    
    // Flow rate between two vertices
    auto Q = [&circulationSolver] (const std::string& N1, const std::string& N2) { return circulationSolver.solution ( std::vector<std::string> {N1, N2} ); };
    auto p = [&circulationSolver] (const std::string& N1) { return circulationSolver.solution ( N1 ); };
    
    
    //============================================//
    // Load mesh
    //============================================//
    heartSolver.loadMesh();
    
    
    //============================================//
    // Resize mesh
    //============================================//
    heartSolver.transformMesh();
    
    
    //============================================//
    // Setup solver (including fe-spaces & b.c.)
    //============================================//
    solver.setup (dataFile);
    
    
    //============================================//
    // Setup anisotropy vectors
    //============================================//
    heartSolver.setupAnisotropyFields();
    
    
    //============================================//
    // Initialize electrophysiology
    //============================================//
    displayer.leaderPrint ("\nInitialize electrophysiology ... ");

    solver.initialize();
    
    displayer.leaderPrint ("\ndone!");

    
    //============================================//
    // Building Matrices
    //============================================//
    displayer.leaderPrint ("\nBuilding matrices ... ");

    solver.oneWayCoupling();
    solver.structuralOperatorPtr()->setNewtonParameters(dataFile);
    solver.buildSystem();
    
    displayer.leaderPrint ("\ndone!");

    
    //============================================//
    // Setup exporters for EMSolver
    //============================================//
    displayer.leaderPrint ("\nSetting up exporters ... ");

    solver.setupExporters (problemFolder);
    
    displayer.leaderPrint ("\ndone!");
    
    
    //============================================//
    // Electric stimulus function
    //============================================//
    function_Type stim = &Iapp;
    
    
    //============================================//
    // Kept-normal boundary conditions
    //============================================//
    // Get b.c. flags
    // ID LvFlag =  dataFile ( "solid/boundary_conditions/LvFlag", 0);
    
    // Boundary vector normal in deformed configuration
    // solver.structuralOperatorPtr() -> setBCFlag( LvFlag );
    
    //solver.bcInterfacePtr() -> handler() -> addBC("LvPressure", LVFlag, Natural, Full, *pLvBCVectorPtr, 3); // BC for using function which keeps bc normal
    // Todo: Normal boundary condition!!

    
    //============================================//
    // B.C. endocardia and patches
    //============================================//
    std::vector<vectorPtr_Type> pVecPtrs;
    std::vector<bcVectorPtr_Type> pBCVecPtrs;
    std::vector<vectorPtr_Type> pVecPatchesPtrs;
    std::vector<bcVectorPtr_Type> pBCVecPatchesPtrs;

    std::vector<ID> flagsBC;
    std::vector<ID> flagsBCPatches;

    std::vector<UInt> ventIdx;

    // Endocardia
    UInt nVarBC = dataFile.vector_variable_size ( ( "solid/boundary_conditions/listVariableBC" ) );
    for ( UInt i (0) ; i < nVarBC ; ++i )
    {
        std::string varBCSection = dataFile ( ( "solid/boundary_conditions/listVariableBC" ), " ", i );
        flagsBC.push_back( dataFile ( ("solid/boundary_conditions/" + varBCSection + "/flag").c_str(), 0 ) );
        ventIdx.push_back ( dataFile ( ("solid/boundary_conditions/" + varBCSection + "/index").c_str(), 0 ) );
        
        pVecPtrs.push_back ( vectorPtr_Type ( new vector_Type ( solver.structuralOperatorPtr() -> displacement().map(), Repeated ) ) );
        pBCVecPtrs.push_back ( bcVectorPtr_Type( new bcVector_Type( *pVecPtrs[i], solver.structuralOperatorPtr() -> dispFESpacePtr() -> dof().numTotalDof(), 1 ) ) );
        solver.bcInterfacePtr() -> handler() -> addBC(varBCSection, flagsBC[i], Natural, Full, *pBCVecPtrs[i], 3);
    }
    
    // Force vector patches
    UInt nVarPatchesBC = dataFile.vector_variable_size ( ( "solid/boundary_conditions/listForcePatchesBC" ) );
    for ( UInt i (0) ; i < nVarPatchesBC ; ++i )
    {
        std::string varBCPatchesSection = dataFile ( ( "solid/boundary_conditions/listPatchesBC" ), " ", i );
        flagsBCPatches.push_back ( dataFile ( ("solid/boundary_conditions/" + varBCPatchesSection + "/flag").c_str(), 0 ) );
        
        pVecPatchesPtrs.push_back ( vectorPtr_Type ( new vector_Type ( solver.structuralOperatorPtr() -> displacement().map(), Repeated ) ) );
        pBCVecPatchesPtrs.push_back ( bcVectorPtr_Type( new bcVector_Type( *pVecPatchesPtrs[i], solver.structuralOperatorPtr() -> dispFESpacePtr() -> dof().numTotalDof(), 1 ) ) );
        solver.bcInterfacePtr() -> handler() -> addBC(varBCPatchesSection, flagsBCPatches[i], Natural, Full, *pBCVecPatchesPtrs[i], 3);
    }

    // Displacement function patches (not working properly yet)
    BCFunctionBase bcFunction;
    bcFunction.setFunction(patchFunction);
    UInt nVarDispPatchesBC = dataFile.vector_variable_size ( ( "solid/boundary_conditions/listDispPatchesBC" ) );
    for ( UInt i (0) ; i < nVarDispPatchesBC ; ++i )
    {
        std::string varBCPatchesSection = dataFile ( ( "solid/boundary_conditions/listDispPatchesBC" ), " ", i );
        ID flag = dataFile ( ("solid/boundary_conditions/" + varBCPatchesSection + "/flag").c_str(), 0 );
        
        solver.bcInterfacePtr() -> handler() -> addBC(varBCPatchesSection, flag, Essential, Normal, bcFunction);
    }

    
    solver.bcInterfacePtr() -> handler() -> bcUpdate( *solver.structuralOperatorPtr() -> dispFESpacePtr() -> mesh(), solver.structuralOperatorPtr() -> dispFESpacePtr() -> feBd(), solver.structuralOperatorPtr() -> dispFESpacePtr() -> dof() );
    
    // Functions to modify b.c.
    auto modifyFeBC = [&] (const std::vector<Real>& bcValues)
    {
        for ( UInt i (0) ; i < nVarBC ; ++i )
        {
            *pVecPtrs[i] = - bcValues[ ventIdx[i] ] * 1333.224;
            // Check coordinates of pVecPtrs and assign only values to certain cells
            // Implement vector for both natural and essential b.c.
            pBCVecPtrs[i].reset ( ( new bcVector_Type (*pVecPtrs[i], solver.structuralOperatorPtr() -> dispFESpacePtr() -> dof().numTotalDof(), 1) ) );
            solver.bcInterfacePtr() -> handler() -> modifyBC(flagsBC[i], *pBCVecPtrs[i]);
        }
    };

    Real Tmax = dataFile ( "solid/patches/Tmax", 0. );
    Real tmax = dataFile ( "solid/patches/tmax", 0. );
    Real tduration = dataFile ( "solid/patches/tduration", 0. );

    auto modifyFeBCPatches = [&] (const Real& time)
    {
        for ( UInt i (0) ; i < nVarPatchesBC ; ++i )
        {
            if ( 0 == comm->MyPID() ) std::cout << "\nPatch force: " << patchForce(time, Tmax, tmax, tduration) << std::endl;
            *pVecPatchesPtrs[i] = - patchForce(time, Tmax, tmax, tduration) * 1333.224;
            pBCVecPatchesPtrs[i].reset ( ( new bcVector_Type (*pVecPatchesPtrs[i], solver.structuralOperatorPtr() -> dispFESpacePtr() -> dof().numTotalDof(), 1) ) );
            solver.bcInterfacePtr() -> handler() -> modifyBC(flagsBCPatches[i], *pBCVecPatchesPtrs[i]);
        }
    };

    if ( 0 == comm->MyPID() ) solver.bcInterfacePtr() -> handler() -> showMe();
    
    
    //============================================//
    // Volume integrators
    //============================================//
    auto& disp = solver.structuralOperatorPtr() -> displacement();
    auto FESpace = solver.structuralOperatorPtr() -> dispFESpacePtr();
    auto dETFESpace = solver.electroSolverPtr() -> displacementETFESpacePtr();
    auto ETFESpace = solver.electroSolverPtr() -> ETFESpacePtr();
    
    std::vector<int> LVFlags;
    std::vector<int> RVFlags;

    for ( UInt i (0) ; i < nVarBC ; ++i )
    {
        std::string varBCSection = dataFile ( ( "solid/boundary_conditions/listVariableBC" ), " ", i );
        ID flag  =  dataFile ( ("solid/boundary_conditions/" + varBCSection + "/flag").c_str(), 0 );
        ID index =  dataFile ( ("solid/boundary_conditions/" + varBCSection + "/index").c_str(), 0 );
        switch ( index )
        {
            case 0: LVFlags.push_back ( flag );
                break;
            case 1: RVFlags.push_back ( flag );
                break;
            default:
                break;
        }
    }
    
    VolumeIntegrator LV (LVFlags, "Left Ventricle", solver.fullMeshPtr(), solver.localMeshPtr(), ETFESpace, FESpace);
    VolumeIntegrator RV (RVFlags, "Right Ventricle", solver.fullMeshPtr(), solver.localMeshPtr(), ETFESpace, FESpace);

    
    //============================================//
    // Set variables and functions
    //============================================//
    
    
    const Real dpMax = dataFile ( "solid/coupling/dpMax", 0.1 );

    std::vector<std::vector<std::string> > bcNames { { "lv" , "p" } , { "rv" , "p" } };
    std::vector<double> bcValues { p ( "lv" ) , p ( "rv") };
    std::vector<double> bcValuesPre ( bcValues );

    VectorSmall<4> ABdplv, ABdprv;

    VectorSmall<2> VCirc, VCircNew, VCircPert, VFe, VFeNew, VFePert, R, dp;
    MatrixSmall<2,2> JFe, JCirc, JR;

    UInt iter (0);
    Real t (0);
    
    auto printCoupling = [&] ( std::string label ) { if ( 0 == comm->MyPID() )
    {
        std::cout << "\n===============================================================";
        std::cout << "\nCoupling: " << label;
        std::cout << "\nNewton iteration nr. " << iter << " at time " << t;
        std::cout << "\nLV - Pressure: \t\t\t" << bcValues[0];
        std::cout << "\nLV - FE-Volume: \t\t" << VFeNew[0];
        std::cout << "\nLV - Circulation-Volume: \t" << VCircNew[0];
        std::cout << "\nLV - Residual: \t\t\t" << std::abs(VFeNew[0] - VCircNew[0]);
        std::cout << "\nRV - Pressure: \t\t\t" << bcValues[1];
        std::cout << "\nRV - FE-Volume : \t\t" << VFeNew[1];
        std::cout << "\nRV - Circulation-Volume: \t" << VCircNew[1];
        std::cout << "\nRV - Residual: \t\t\t" << std::abs(VFeNew[1] - VCircNew[1]);
        //std::cout << "\nJFe   = " << JFe;
        //std::cout << "\nJCirc = " << JCirc;
        //std::cout << "\nJR    = " << JR;
        std::cout << "\n==============================================================="; }
    };
    
    
    
    //============================================//
    // Load restart file
    //============================================//
    
    std::string restartInput = command_line.follow ("noRestart", 2, "-r", "--restart");
    const bool restart ( restartInput != "noRestart" );

    if ( restart )
    {
        heartSolver.restart(restartInput, command_line, t);
        
        // Set boundary mechanics conditions
        bcValues = { p ( "lv" ) , p ( "rv" ) };
        bcValuesPre = { p ( "lv" ) , p ( "rv" ) };
        modifyFeBC(bcValues);

    }

    
    //============================================//
    // Preload
    //============================================//
    
    if ( ! restart )
    {
        heartSolver.preload(modifyFeBC, bcValues);
    }
    
    
    //============================================//
    // Time loop
    //============================================//
    
    VFe[0] = LV.volume(disp, dETFESpace, - 1);
    VFe[1] = RV.volume(disp, dETFESpace, 1);
    VCirc = VFe;
    
    VectorEpetra dispPre ( disp );
    VectorEpetra dispCurrent ( disp );
    ID bdPowerFlag  =  dataFile ( ("solid/boundary_conditions/LVEndo/flag") , 0 );
    
    printCoupling("Initial values");
    
    auto perturbedPressure = [] (std::vector<double> p, const double& dp)
    {
        for (auto& i : p) {i += dp;}
        return p;
    };
    
    auto perturbedPressureComp = [] (std::vector<double> p, const double& dp, int comp)
    {
        p[comp] += dp;
        return p;
    };

    if ( ! restart )
    {
        solver.saveSolution (t);
        circulationSolver.exportSolution( circulationOutputFile );
    }
    
    LifeChrono chrono;
    chrono.start();

    
    for (int k (1); k <= heartSolver.data().maxiter(); ++k)
    {
        if ( 0 == comm->MyPID() )
        {
            std::cout << "\n*****************************************************************";
            std::cout << "\nTIME = " << t + heartSolver.data().dt_activation();
            std::cout << "\n*****************************************************************\n";
        }

        t = t + heartSolver.data().dt_activation();

        //============================================//
        // Solve electrophysiology and activation
        //============================================//

        solver.solveElectrophysiology (stim, t);
        solver.solveActivation (heartSolver.data().dt_activation());

        auto minActivationValue ( solver.activationModelPtr() -> fiberActivationPtr() -> minValue() );
        auto meanActivationValue ( solver.activationModelPtr() -> fiberActivationPtr() -> meanValue() );

        if ( 0 == comm->MyPID() )
        {
            std::cout << "\n*****************************************************************";
            std::cout << "\nLoad step at time = " << t;
            std::cout << "\nMinimal activation value = " << minActivationValue;
            std::cout << "\nMean activation value = " << meanActivationValue;
            std::cout << "\n*****************************************************************\n";
        }

        //============================================//
        // Load steps mechanics (activation & b.c.)
        //============================================//

        auto mechanicsCouplingIter = heartSolver.data().mechanicsCouplingIter();
        
        if ( k % heartSolver.data().mechanicsLoadstepIter() == 0 && k % mechanicsCouplingIter != 0 && minActivationValue < heartSolver.data().activationLimit_loadstep() )
        {
            if ( 0 == comm->MyPID() )
            {
                std::cout << "\n*****************************************************************";
                std::cout << "\nLoad step at time = " << t;
                std::cout << "\n*****************************************************************\n";
            }
            
            // Linear b.c. extrapolation
            auto bcValuesLoadstep ( bcValues );
            bcValuesLoadstep[0] = bcValues[0] + ( bcValues[0] - bcValuesPre[0] ) * ( k % mechanicsCouplingIter ) / mechanicsCouplingIter;
            bcValuesLoadstep[1] = bcValues[1] + ( bcValues[1] - bcValuesPre[1] ) * ( k % mechanicsCouplingIter ) / mechanicsCouplingIter;
            
            if ( 0 == comm->MyPID() )
            {
                std::cout << "\n***************************************************************";
                std::cout << "\nLV-Pressure extrapolation from " <<  bcValues[0] << " to " <<  bcValuesLoadstep[0];
                std::cout << "\nRV-Pressure extrapolation from " <<  bcValues[1] << " to " <<  bcValuesLoadstep[1];
                std::cout << "\n***************************************************************\n\n";
            }
            
            // Load step mechanics
            solver.structuralOperatorPtr() -> data() -> dataTime() -> setTime(t);
            modifyFeBC(bcValuesLoadstep);
            solver.bcInterfacePtr() -> updatePhysicalSolverVariables();
            solver.solveMechanics();
        }
        
        
        //============================================//
        // Iterate mechanics / circulation
        //============================================//
        
        if ( k % heartSolver.data().mechanicsCouplingIter() == 0 )
        {
            iter = 0;
            const double dt_circulation ( heartSolver.data().dt_mechanics() / 1000 );
            solver.structuralOperatorPtr() -> data() -> dataTime() -> setTime(t);
            
            modifyFeBCPatches(t);
            
            //============================================//
            // 4th order Adam-Bashforth pressure extrapol.
            //============================================//
            heartSolver.extrapolate4thOrderAdamBashforth(bcValues, bcValuesPre, ABdplv, ABdprv);

            
            //============================================//
            // Solve mechanics
            //============================================//
            modifyFeBC(bcValues);
            solver.bcInterfacePtr() -> updatePhysicalSolverVariables();
            solver.solveMechanics();
            
            VFeNew[0] = LV.volume(disp, dETFESpace, - 1);
            VFeNew[1] = RV.volume(disp, dETFESpace, 1);

            //============================================//
            // Solve circlation
            //============================================//
            circulationSolver.iterate(dt_circulation, bcNames, bcValues, iter);
            VCircNew[0] = VCirc[0] + dt_circulation * ( Q("la", "lv") - Q("lv", "sa") );
            VCircNew[1] = VCirc[1] + dt_circulation * ( Q("ra", "rv") - Q("rv", "pa") );

            //============================================//
            // Residual computation
            //============================================//
            R = VFeNew - VCircNew;
            printCoupling("Residual Computation");
            
            //============================================//
            // Newton iterations
            //============================================//
            while ( R.norm() > heartSolver.data().couplingError() )
            {
                ++iter;
                
                //============================================//
                // Jacobian circulation
                //============================================//
                
                // Left ventricle
                circulationSolver.iterate(dt_circulation, bcNames, perturbedPressureComp(bcValues, heartSolver.data().pPerturbationCirc(), 0), iter);
                VCircPert[0] = VCirc[0] + dt_circulation * ( Q("la", "lv") - Q("lv", "sa") );
                VCircPert[1] = VCirc[1] + dt_circulation * ( Q("ra", "rv") - Q("rv", "pa") );

                JCirc(0,0) = ( VCircPert[0] - VCircNew[0] ) / heartSolver.data().pPerturbationCirc();
                JCirc(1,0) = ( VCircPert[1] - VCircNew[1] ) / heartSolver.data().pPerturbationCirc();
                
                // Right ventricle
                circulationSolver.iterate(dt_circulation, bcNames, perturbedPressureComp(bcValues, heartSolver.data().pPerturbationCirc(), 1), iter);
                VCircPert[0] = VCirc[0] + dt_circulation * ( Q("la", "lv") - Q("lv", "sa") );
                VCircPert[1] = VCirc[1] + dt_circulation * ( Q("ra", "rv") - Q("rv", "pa") );
                
                JCirc(0,1) = ( VCircPert[0] - VCircNew[0] ) / heartSolver.data().pPerturbationCirc();
                JCirc(1,1) = ( VCircPert[1] - VCircNew[1] ) / heartSolver.data().pPerturbationCirc();
                
                //============================================//
                // Jacobian fe
                //============================================//

                const UInt couplingJFeSubIter = heartSolver.data().couplingJFeSubIter();
                const UInt couplingJFeSubStart = heartSolver.data().couplingJFeSubStart();
                const UInt couplingJFeIter = heartSolver.data().couplingJFeIter();
                
                const bool jFeIter ( ! ( k % (couplingJFeIter * heartSolver.data().mechanicsCouplingIter() ) ) );
                const bool jFeSubIter ( ! ( (iter - couplingJFeSubStart) % couplingJFeSubIter) && iter >= couplingJFeSubStart );
                const bool jFeEmpty ( JFe.norm() == 0 );
                
                if ( jFeIter || jFeSubIter || jFeEmpty )
                {
                    JFe *= 0.0;
                    dispCurrent = disp;
                    
                    // Left ventricle
                    modifyFeBC(perturbedPressureComp(bcValues, heartSolver.data().pPerturbationFe(), 0));
                    solver.bcInterfacePtr() -> updatePhysicalSolverVariables();
                    solver.solveMechanicsLin();
                    
                    VFePert[0] = LV.volume(disp, dETFESpace, - 1);
                    VFePert[1] = RV.volume(disp, dETFESpace, 1);

                    JFe(0,0) = ( VFePert[0] - VFeNew[0] ) / heartSolver.data().pPerturbationFe();
                    JFe(1,0) = ( VFePert[1] - VFeNew[1] ) / heartSolver.data().pPerturbationFe();
                    
                    disp = dispCurrent;
                    
                    // Right ventricle
                    modifyFeBC(perturbedPressureComp(bcValues, heartSolver.data().pPerturbationFe(), 1));
                    solver.bcInterfacePtr() -> updatePhysicalSolverVariables();
                    solver.solveMechanicsLin();
                    
                    VFePert[0] = LV.volume(disp, dETFESpace, - 1);
                    VFePert[1] = RV.volume(disp, dETFESpace, 1);
                    
                    JFe(0,1) = ( VFePert[0] - VFeNew[0] ) / heartSolver.data().pPerturbationFe();
                    JFe(1,1) = ( VFePert[1] - VFeNew[1] ) / heartSolver.data().pPerturbationFe();
                    
                    disp = dispCurrent;
                }
                
                //============================================//
                // Update pressure b.c.
                //============================================//
                JR = JFe - JCirc;

                if ( JR.determinant() != 0 )
                {
                    dp = ( JR | R );
                    if ( iter > 5 ) dp *= 0.7;
                    if ( iter > 20 ) dp *= 0.7;
                    bcValues[0] -= std::min( std::max( dp(0) , - dpMax ) , dpMax );
                    bcValues[1] -= std::min( std::max( dp(1) , - dpMax ) , dpMax );
                }
                
                printCoupling("Pressure Update");

                //============================================//
                // Solve circulation
                //============================================//
                circulationSolver.iterate(dt_circulation, bcNames, bcValues, iter);
                VCircNew[0] = VCirc[0] + dt_circulation * ( Q("la", "lv") - Q("lv", "sa") );
                VCircNew[1] = VCirc[1] + dt_circulation * ( Q("ra", "rv") - Q("rv", "pa") );

                //============================================//
                // Solve mechanics
                //============================================//
                modifyFeBC(bcValues);
                solver.bcInterfacePtr() -> updatePhysicalSolverVariables();
                solver.solveMechanics();
                
                VFeNew[0] = LV.volume(disp, dETFESpace, - 1);
                VFeNew[1] = RV.volume(disp, dETFESpace, 1);

                //============================================//
                // Residual update
                //============================================//
                R = VFeNew - VCircNew;
                printCoupling("Residual Update");
            }
 
            if ( 0 == comm->MyPID() )
            {
                std::cout << "\n*****************************************************************";
                std::cout << "\nCoupling converged after " << iter << " iteration" << ( iter > 1 ? "s" : "" );
                std::cout << "\n*****************************************************************\n\n";
            }
            
//            Real extPow = externalPower(disp, dispPre, dETFESpace, p("lv"), dt_mechanics, bdPowerFlag);
//            
//            if ( 0 == comm->MyPID() )
//            {
//                std::cout << "\n******************************************";
//                std::cout << "\nExternal power is " << extPow;
//                std::cout << "\n******************************************\n\n";
//            }
//            
//            dispPre = disp;
            
            //============================================//
            // Update volume variables
            //============================================//
            VCirc = VCircNew;
            VFe = VFeNew;
            
            //============================================//
            // Export circulation solution
            //============================================//
            if ( 0 == comm->MyPID() ) circulationSolver.exportSolution( circulationOutputFile );
            
        }
        
        //============================================//
        // Export FE-solution
        //============================================//
        bool save ( std::abs(std::remainder(t, heartSolver.data().dt_save() )) < 0.01 );
        if ( save ) solver.saveSolution(t, restart);
        
        
        //============================================//
        // Time for certain simulation time
        //============================================//
        if ( k % heartSolver.data().chronoIter() == 0 )
        {
            std::cout << "\n*****************************************************************";
            std::cout << "\nTime to compute last " << heartSolver.data().dt_chrono() << " ms: " <<  chrono.globalDiff(comm) << " s";
            std::cout << "\n*****************************************************************\n\n";
        }
        
    }

    
    //============================================//
    // Close all exporters
    //============================================//
    solver.closeExporters();
    

#ifdef HAVE_MPI
    MPI_Finalize();
#endif
    return 0;
}
