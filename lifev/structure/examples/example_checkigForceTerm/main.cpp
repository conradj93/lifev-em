//@HEADER
/*
*******************************************************************************

    Copyright (C) 2004, 2005, 2007 EPFL, Politecnico di Milano, INRIA
    Copyright (C) 2010 EPFL, Politecnico di Milano, Emory University

    This file is part of LifeV.

    LifeV is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LifeV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with LifeV.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************
*/
//@HEADER
/**
   \file main.cpp

   This test is the case of traction of a cube. It does not use the symmetry BCs
   This test uses the FESpace which is the standard in LifeV and the ETFESpace
   The FESpace is used for the BCs of Neumann type since in the ET branch there
   is not the integration over the boundary faces.

   \author Paolo Tricerri <paolo.tricerri@epfl.ch>
   \date 1861-03-17
 */

#ifdef TWODIM
#error test_structure cannot be compiled in 2D
#endif

// Tell the compiler to ignore specific kind of warnings:
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <Epetra_ConfigDefs.h>
#ifdef EPETRA_MPI
#include <mpi.h>
#include <Epetra_MpiComm.h>
#else
#include <Epetra_SerialComm.h>
#endif

//Tell the compiler to restore the warning previously silented
#pragma GCC diagnostic warning "-Wunused-variable"
#pragma GCC diagnostic warning "-Wunused-parameter"

#include <lifev/core/LifeV.hpp>
#include <lifev/core/algorithm/PreconditionerIfpack.hpp>
#include <lifev/core/algorithm/PreconditionerML.hpp>

#include <lifev/core/array/MapEpetra.hpp>
#include <lifev/core/array/VectorSmall.hpp>

#include <lifev/core/mesh/MeshData.hpp>
#include <lifev/core/mesh/MeshPartitioner.hpp>

#include <lifev/structure/solver/StructuralConstitutiveLawData.hpp>

#include <lifev/structure/solver/StructuralConstitutiveLaw.hpp>
#include <lifev/structure/solver/StructuralOperator.hpp>
#include <lifev/structure/solver/isotropic/VenantKirchhoffMaterialLinear.hpp>


#include <lifev/core/filter/ExporterEnsight.hpp>
#ifdef HAVE_HDF5
#include <lifev/core/filter/ExporterHDF5.hpp>
#endif
#include <lifev/core/filter/ExporterEmpty.hpp>

//Includes for the Expression Template
#include <lifev/eta/fem/ETFESpace.hpp>

#include <iostream>



using namespace LifeV;

namespace
{
static bool regIF = (PRECFactory::instance().registerProduct ( "Ifpack", &createIfpack ) );
static bool regML = (PRECFactory::instance().registerProduct ( "ML", &createML ) );
}


std::set<UInt> parseList ( const std::string& list )
{
    std::string stringList = list;
    std::set<UInt> setList;
    if ( list == "" )
    {
        return setList;
    }
    size_t commaPos = 0;
    while ( commaPos != std::string::npos )
    {
        commaPos = stringList.find ( "," );
        setList.insert ( atoi ( stringList.substr ( 0, commaPos ).c_str() ) );
        stringList = stringList.substr ( commaPos + 1 );
    }
    setList.insert ( atoi ( stringList.c_str() ) );
    return setList;
}


class Structure
{
public:

    // Public typedefs
    typedef StructuralOperator< RegionMesh<LinearTetra> >::vector_Type    vector_Type;
    typedef StructuralOperator< RegionMesh<LinearTetra> >::matrix_Type    matrix_Type;
    typedef StructuralOperator< RegionMesh<LinearTetra> >::matrixPtr_Type matrixPtr_Type;
    typedef std::shared_ptr<vector_Type>                              vectorPtr_Type;
    typedef FESpace< RegionMesh<LinearTetra>, MapEpetra >               solidFESpace_Type;
    typedef std::shared_ptr<solidFESpace_Type>                        solidFESpacePtr_Type;

    typedef ETFESpace< RegionMesh<LinearTetra>, MapEpetra, 3, 3 >       solidETFESpace_Type;
    typedef std::shared_ptr<solidETFESpace_Type>                      solidETFESpacePtr_Type;

    // typedefs for fibers
    // std function for fiber direction
    typedef std::function<Real ( Real const&, Real const&, Real const&, Real const&, ID const& ) > analyticalFunction_Type;
    typedef std::shared_ptr<analyticalFunction_Type> analyticalFunctionPtr_Type;

    typedef std::function<VectorSmall<3> ( Real const&, Real const&, Real const&, Real const& ) > bodyFunction_Type;
    typedef std::shared_ptr<bodyFunction_Type> bodyFunctionPtr_Type;


    typedef std::vector<analyticalFunctionPtr_Type>                     vectorFiberFunction_Type;
    typedef std::shared_ptr<vectorFiberFunction_Type>                 vectorFiberFunctionPtr_Type;

    typedef std::vector<vectorPtr_Type>                                 listOfFiberDirections_Type;

    /** @name Constructors, destructor
     */
    //@{
    Structure ( int                                   argc,
                char**                                argv,
                std::shared_ptr<Epetra_Comm>        structComm );

    ~Structure()
    {}
    //@}

    //@{
    void run()
    {
        run3d();
    }
    //@}

protected:

private:

    /**
     * run the 2D driven cylinder simulation
     */
    void run2d();

    /**
     * run the 3D driven cylinder simulation
     */
    void run3d();

private:
    struct Private;
    std::shared_ptr<Private> parameters;
};



struct Structure::Private
{
    Private() :
        rho (1), poisson (1), young (1), bulk (1), alpha (1), gamma (1)
    {}
    double rho, poisson, young, bulk, alpha, gamma;

    std::string data_file_name;

    std::shared_ptr<Epetra_Comm>     comm;

    static Real lifeVedF (const Real& t, const Real& x, const Real& y, const Real& z, const ID& i)
    {

        switch (i)
        {
            case 0:
                return  1.0;
                break;
            case 1:
                return  1.0;
                break;
            case 2:
                return  1.0;
                break;
            default:
                ERROR_MSG ("This entry is not allowed: ud_functions.hpp");
                return 0.;
                break;
        }

    }

    static VectorSmall<3> f (const Real& t, const Real& x, const Real& y, const Real& z)
    {
        VectorSmall<3>   evaluationOfF;

        evaluationOfF[ 0 ] = 1.0;
        evaluationOfF[ 1 ] = 1.0;
        evaluationOfF[ 2 ] = 1.0;

        return evaluationOfF;
    }


};



Structure::Structure ( int                                   argc,
                       char**                                argv,
                       std::shared_ptr<Epetra_Comm>        structComm) :
    parameters ( new Private() )
{
    GetPot command_line (argc, argv);
    std::string data_file_name = command_line.follow ("data", 2, "-f", "--file");
    GetPot dataFile ( data_file_name );
    parameters->data_file_name = data_file_name;

    parameters->comm = structComm;
    int ntasks = parameters->comm->NumProc();

    if (!parameters->comm->MyPID() )
    {
        std::cout << "My PID = " << parameters->comm->MyPID() << " out of " << ntasks << " running." << std::endl;
    }
}



void
Structure::run2d()
{
    std::cout << "2D cylinder test case is not available yet\n";
}



void
Structure::run3d()
{
    bool verbose = (parameters->comm->MyPID() == 0);

    //! dataElasticStructure
    GetPot dataFile ( parameters->data_file_name.c_str() );

    //! Number of boundary conditions for the velocity and mesh motion
    std::shared_ptr<BCHandler> BCh ( new BCHandler() );

    std::shared_ptr<StructuralConstitutiveLawData> dataStructure (new StructuralConstitutiveLawData( ) );
    dataStructure->setup (dataFile);

    dataStructure->showMe();

    MeshData             meshData;
    meshData.setup (dataFile, "solid/space_discretization");

    std::shared_ptr<RegionMesh<LinearTetra> > fullMeshPtr (new RegionMesh<LinearTetra> (  parameters->comm  ) );
    readMesh (*fullMeshPtr, meshData);

    MeshPartitioner< RegionMesh<LinearTetra> > meshPart ( fullMeshPtr, parameters->comm );
    std::shared_ptr<RegionMesh<LinearTetra> > localMeshPtr (new RegionMesh<LinearTetra> (  parameters->comm  ) );
    localMeshPtr = meshPart.meshPartition();

    std::string dOrder =  dataFile ( "solid/space_discretization/order", "P1");

    //Mainly used for BCs assembling (Neumann type)
    solidFESpacePtr_Type dFESpace ( new solidFESpace_Type (localMeshPtr, dOrder, 3, parameters->comm) );
    solidETFESpacePtr_Type dETFESpace ( new solidETFESpace_Type (meshPart, & (dFESpace->refFE() ), & (dFESpace->fe().geoMap() ), parameters->comm) );

    //! 1. Constructor of the structuralSolver
    StructuralOperator< RegionMesh<LinearTetra> > solid;

    //! 2. Setup of the structuralSolver
    solid.setup (dataStructure,
                 dFESpace,
                 dETFESpace,
                 BCh,
                 parameters->comm);

    //! 3. Setting data from getPot for preconditioners
    solid.setDataFromGetPot (dataFile);

    //! Setting the vectors
    vectorPtr_Type rhs (new vector_Type (solid.displacement(), Unique) );
    vectorPtr_Type disp (new vector_Type (solid.displacement(), Unique) );

    // Comment this part in order not to have body force in the RHS of the equation
    bool bodyForce = dataFile ( "solid/physics/bodyForce" , false );
    if( bodyForce )
    {
        solid.setHavingSourceTerm(  bodyForce );
        bodyFunctionPtr_Type bodyTerm( new bodyFunction_Type( Private::f ) );

        solid.setSourceTerm( bodyTerm );
    }

    // Build the mass matrix
    solid.computeMassMatrix ( 1.0 );

    // Build the rhs
    solid.updateRightHandSideWithBodyForce( 1.0, *rhs );

    MPI_Barrier (MPI_COMM_WORLD);

    std::shared_ptr< Exporter<RegionMesh<LinearTetra> > > exporter;
    std::shared_ptr< Exporter<RegionMesh<LinearTetra> > > exporterCheck;

    std::string const exporterType =  dataFile ( "exporter/type", "ensight");
    std::string const exporterNameFile =  dataFile ( "exporter/nameFile", "structure");

#ifdef HAVE_HDF5
    if (exporterType.compare ("hdf5") == 0)
    {
        exporter.reset ( new ExporterHDF5<RegionMesh<LinearTetra> > ( dataFile, exporterNameFile ) );
    }
    else
#endif
    {
        if (exporterType.compare ("none") == 0)
        {
            exporter.reset ( new ExporterEmpty<RegionMesh<LinearTetra> > ( dataFile, meshPart.meshPartition(), "structure", parameters->comm->MyPID() ) );
        }

        else
        {
            exporter.reset ( new ExporterEnsight<RegionMesh<LinearTetra> > ( dataFile, meshPart.meshPartition(), "structure", parameters->comm->MyPID() ) );
        }
    }

    exporter->setPostDir ( "./" );
    exporter->setMeshProcId ( meshPart.meshPartition(), parameters->comm->MyPID() );

    vectorPtr_Type interpolatedF         ( new vector_Type ( solid.map() ) );
    vectorPtr_Type solutionLinearSystem  ( new vector_Type ( solid.map() ) );
    vectorPtr_Type bodyForcePointer      ( new vector_Type ( solid.map() ) );
    vectorPtr_Type scalarProduct         ( new vector_Type ( solid.map() ) );

    exporter->addVariable ( ExporterData<RegionMesh<LinearTetra> >::VectorField, "scalarProduct", dFESpace, scalarProduct, UInt (0) );
    exporter->addVariable ( ExporterData<RegionMesh<LinearTetra> >::VectorField, "solution",      dFESpace, solutionLinearSystem,  UInt (0) );
    exporter->addVariable ( ExporterData<RegionMesh<LinearTetra> >::VectorField, "assembledBodyForce",      dFESpace, bodyForcePointer,  UInt (0) );

    exporter->postProcess ( 0 );
    std::cout.precision(16);

    // Interpolating the function and multiplying by the mass matrix
    dFESpace->interpolate ( static_cast< FESpace<RegionMesh<LinearTetra> , MapEpetra>::function_Type> ( Private::lifeVedF ), *interpolatedF, 0.0);
    *solutionLinearSystem = *( solid.massMatrix( ) ) * (*interpolatedF);

    // getting the vector from the expression template assembling
    *bodyForcePointer = solid.bodyForce();

    // computing the scalar product f \phi_i
    dFESpace->l2ScalarProduct( static_cast< FESpace<RegionMesh<LinearTetra> , MapEpetra>::function_Type> ( Private::lifeVedF ), *scalarProduct, 0.0);


    // Exporting the solution and the interpolation
    exporter->postProcess ( 1.0 );
    //!--------------------------------------------------------------------------------------------------

    MPI_Barrier (MPI_COMM_WORLD);
}


int
main ( int argc, char** argv )
{

#ifdef HAVE_MPI
    MPI_Init (&argc, &argv);
    std::shared_ptr<Epetra_MpiComm> Comm (new Epetra_MpiComm ( MPI_COMM_WORLD ) );
    if ( Comm->MyPID() == 0 )
    {
        std::cout << "% using MPI" << std::endl;
    }
#else
    std::shared_ptr<Epetra_SerialComm> Comm ( new Epetra_SerialComm() );
    std::cout << "% using serial Version" << std::endl;
#endif

    Structure structure ( argc, argv, Comm );
    structure.run();

#ifdef HAVE_MPI
    MPI_Finalize();
#endif
    return EXIT_SUCCESS ;
}
