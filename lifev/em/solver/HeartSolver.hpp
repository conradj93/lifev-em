//
//  HeartSolver.cpp
//  LifeV
//
//  Created by Thomas Kummer on 03.05.16.
//  Copyright © 2016 Thomas Kummer. All rights reserved.
//

#ifndef _HEARTDATA_H_
#define _HEARTDATA_H_


#include <stdio.h>
#include <lifev/em/solver/EMSolver.hpp>
#include <lifev/em/solver/circulation/Circulation.hpp>

#include <lifev/em/solver/HeartData.hpp>


namespace LifeV
{

    
class HeartSolver {
   
public:
    
    typedef RegionMesh<LinearTetra>                         mesh_Type;
    typedef boost::shared_ptr<mesh_Type>                    meshPtr_Type;

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
    
    
    HeartSolver(Displayer& displayer) :
        M_displayer         (displayer),
        M_emSolver          (emSolver_type (displayer.comm())),
        M_heartData         (HeartData()),
        M_circulationSolver (Circulation())
    {}
    
    virtual ~HeartSolver() {}

    emSolver_type& emSolver()
    {
        return M_emSolver;
    }
    
    Circulation& circulation()
    {
        return M_circulationSolver;
    }
    
    const HeartData& data() const
    {
        return M_heartData;
    }
    
    void readData(const GetPot& datafile)
    {
        M_heartData.setup(datafile);
    }
    
    void loadMesh()
    {
        M_emSolver.loadMesh ( M_heartData.meshName(), M_heartData.meshPath() );
    }
    
    void loadCirculation(const std::string& circulationFilename)
    {
        M_circulationSolver.setup(circulationFilename);
    }
    
    void transformMesh()
    {
        MeshUtility::MeshTransformer<mesh_Type> transformerFull (* (M_emSolver.fullMeshPtr() ) );
        MeshUtility::MeshTransformer<mesh_Type> transformerLocal (* (M_emSolver.localMeshPtr() ) );
        
        transformerFull.transformMesh (M_heartData.scale(), M_heartData.rotate(), M_heartData.translate());
        transformerLocal.transformMesh (M_heartData.scale(), M_heartData.rotate(), M_heartData.translate());
    }
    
    void setupAnisotropyFields()
    {
        std::string fiberDir       =  M_heartData.meshPath();
        std::string sheetDir       =  M_heartData.meshPath();
        
        M_emSolver.structuralOperatorPtr() -> data() -> dataTime() -> setTime(0.0);

        M_emSolver.setupFiberVector ( M_heartData.fiberFileName(), M_heartData.fiberFieldName(), fiberDir, M_heartData.elementOrder() );
        M_emSolver.setupMechanicalSheetVector ( M_heartData.sheetFileName(), M_heartData.sheetFieldName(), sheetDir, M_heartData.elementOrder() );
    }
    
    template <class lambda>
    void preload(const lambda& modifyFeBC, const std::vector<Real>& bcValues)
    {
        M_emSolver.structuralOperatorPtr() -> data() -> dataTime() -> setTime(0.0);
        
        auto preloadPressure = [] (std::vector<double> p, const int& step, const int& steps)
        {
            for (auto& i : p) {i *= double(step) / double(steps);}
            return p;
        };
        
        LifeChrono chronoSave;
        chronoSave.start();
        
        M_emSolver.saveSolution (-1.0);
        
        if ( 0 == M_emSolver.comm()->MyPID() )
        {
            std::cout << "\n*****************************************************************";
            std::cout << "\nData stored in " << chronoSave.diff() << " s";
            std::cout << "\n*****************************************************************\n";
        }
        
        LifeChrono chronoPreload;
        chronoPreload.start();
        
        for (int i (1); i <= data().preloadSteps(); i++)
        {
            if ( 0 == M_emSolver.comm()->MyPID() )
            {
                std::cout << "\n*****************************************************************";
                std::cout << "\nPreload step: " << i << " / " << data().preloadSteps();
                std::cout << "\n*****************************************************************\n";
            }
            
            // Update pressure b.c.
            modifyFeBC(preloadPressure(bcValues, i, data().preloadSteps() ));
            
            // Solve mechanics
            M_emSolver.bcInterfacePtr() -> updatePhysicalSolverVariables();
            M_emSolver.solveMechanics();
            
            // Safe preload steps
            if ( data().safePreload() ) M_emSolver.saveSolution (i-1);
        }
        
        if ( 0 == M_emSolver.comm()->MyPID() )
        {
            std::cout << "\n*****************************************************************";
            std::cout << "\nPreload done in: " << chronoPreload.diff();
            std::cout << "\n*****************************************************************\n";
        }

    }
    
    
    void restart(std::string& restartInput, const GetPot& command_line, Real& t)
    {
        const std::string restartDir = ""; //command_line.follow (problemFolder.c_str(), 2, "-rd", "--restartDir");
        
        Real dtExport = 10.;
        
        // Set time variable
        const unsigned int restartInputStr = std::stoi(restartInput);
        const unsigned int nIter = (restartInputStr - 1) * dtExport / data().dt_mechanics();
        t = nIter * data().dt_mechanics();
        
        // Set time exporter time index
        M_emSolver.setTimeIndex(restartInputStr + 1);
        //solver.importHdf5();

        // Load restart solutions from output files
        std::string polynomialDegree = data().elementOrder();

        ElectrophysiologyUtility::importVectorField ( M_emSolver.structuralOperatorPtr() -> displacementPtr(), "MechanicalSolution" , "displacement", M_emSolver.localMeshPtr(), restartDir, polynomialDegree, restartInput );
        ElectrophysiologyUtility::importScalarField (M_emSolver.activationModelPtr() -> fiberActivationPtr(), "ActivationSolution" , "Activation", M_emSolver.localMeshPtr(), restartDir, polynomialDegree, restartInput );
        //ElectrophysiologyUtility::importScalarField (solver.activationTimePtr(), "ActivationTimeSolution" , "Activation Time", solver.localMeshPtr(), restartDir, polynomialDegree, restartInput );
        
        for ( unsigned int i = 0; i < M_emSolver.electroSolverPtr()->globalSolution().size() ; ++i )
        {
            ElectrophysiologyUtility::importScalarField (M_emSolver.electroSolverPtr()->globalSolution().at(i), "ElectroSolution" , ("Variable" + std::to_string(i)), M_emSolver.localMeshPtr(), restartDir, polynomialDegree, restartInput );
        }

        if ( 0 == M_emSolver.comm()->MyPID() )
        {
            std::cout << "\nLoad from restart: " << restartInput << ",  nIterCirculation = " << nIter << ",  time = " << t << std::endl;
        }
        
        M_circulationSolver.restartFromFile ( restartDir + "solution.dat" , nIter );
    }
    
    
    
protected:
    
    Displayer& M_displayer;
    
    emSolver_type M_emSolver;
    Circulation M_circulationSolver;
    
    HeartData M_heartData;
    
    
    VectorSmall<2> M_pressure;
    VectorSmall<2> M_volume;

    

    
    std::string pipeToString ( const char* command )
    {
        FILE* file = popen( command, "r" ) ;
        std::ostringstream stm ;
        char line[6] ;
        fgets( line, 6, file );
        stm << line;
        pclose(file) ;
        return stm.str() ;
    };
    
    Real patchForce (const Real& t, const Real& Tmax, const Real& tmax, const Real& tduration) const
    {
        bool time ( fmod(t-tmax+0.5*tduration, 700.) < tduration && fmod(t-tmax+0.5*tduration, 700.) > 0);
        Real force = std::pow( std::sin(fmod(t-tmax+0.5*tduration, 700.)*3.14159265359/tduration) , 2 ) * Tmax;
        return ( time ? force : 0 );
    }
    
    Real patchFunction (const Real& t, const Real&  X, const Real& Y, const Real& Z, const ID& /*i*/) const
    {
        Real disp = std::pow( std::sin(fmod(t, 700.) * 3.14159265359/300) , 2 )*15;
        return disp;
    }
    
    Real Iapp (const Real& t, const Real&  X, const Real& Y, const Real& Z, const ID& /*i*/) const
    {
        bool coords ( Y < -7. );
        //bool coords ( Y > 4. ); //( Y > 1.5 && Y < 3 );
        bool time ( fmod(t, 700.) < 4 && fmod(t, 700.) > 2);
        return ( coords && time ? 30 : 0 );
    }
    
    Real potentialMultiplyerFcn (const Real& t, const Real&  X, const Real& Y, const Real& Z, const ID& /*i*/) const
    {
        bool time ( fmod(t, 700.) < 4 && fmod(t, 700.) > 2);
        return 1.4 * time; // ( Y < 2.5 && Y > 0.5 ? 1.0 : 0.0 );
    }
    
    Real
    externalPower ( const VectorEpetra& dispCurrent,
                    const VectorEpetra& dispPrevious,
                    const boost::shared_ptr<ETFESpace< RegionMesh<LinearTetra>, MapEpetra, 3, 3 > > dispETFESpace,
                    Real pressure,
                    Real dt,
                    const unsigned int bdFlag) const
    {
        VectorEpetra traction ( dispCurrent.map() );
        VectorEpetra velocity ( (dispCurrent - dispPrevious) / dt );
        
        MatrixSmall<3,3> Id;
        Id(0,0) = 1.; Id(0,1) = 0., Id(0,2) = 0.;
        Id(1,0) = 0.; Id(1,1) = 1., Id(1,2) = 0.;
        Id(2,0) = 0.; Id(2,1) = 0., Id(2,2) = 1.;
        
        {
            using namespace ExpressionAssembly;
            
            auto I = value(Id);
            auto Grad_u = grad( dispETFESpace, dispCurrent, 0);
            auto F =  Grad_u + I;
            auto FmT = minusT(F);
            auto J = det(F);
            auto p = value(pressure);
            
            QuadratureBoundary myBDQR (buildTetraBDQR (quadRuleTria7pt) );
            
            integrate ( boundary ( dispETFESpace->mesh(), bdFlag),
                       myBDQR,
                       dispETFESpace,
                       p * J * dot( FmT * Nface,  phi_i)
                       //p * J * dot( FmT * Nface,  phi_i)
                       //value(-1.0) * J * dot (vE1, FmT * Nface) * phi_i) >> intergral
                       ) >> traction;
            
            traction.globalAssemble();
        }
        
        return traction.dot(velocity);
    }
    
    
};

}

#endif
