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

/*!
 *  @file
 *  @brief This file contains an abstract class to implement different kinds of materials for structural dynamic problems (St. Venant-Kirchhoff, Neo-Hookean and Exponential materials right now )
 *
 *  @version 1.0
 *  @date 01-01-2010
 *  @author Paolo Tricerri
 *  @author Gianmarco Mengaldo
 *
 *  @version 2.0
 *  @date 12-03-2010
 *  @author Paolo Tricerri
 *
 *  @maintainer  Paolo Tricerri <paolo.tricerri@epfl.ch>
 *  @contributor  Gianmarco Mengaldo <gianmarco.mengaldo@gmail.com>
 */

#ifndef _STRUCTURALCONSTITUTIVELAW_H_
#define _STRUCTURALCONSTITUTIVELAW_H_ 1

#include <string>
//#include <sstream>
#include <iostream>
#include <stdexcept>


#include <Epetra_Vector.h>
#include <EpetraExt_MatrixMatrix.h>
#include <Epetra_SerialDenseMatrix.h>


#include <lifev/core/array/MatrixEpetra.hpp>
#include <lifev/core/array/VectorEpetra.hpp>

#include <lifev/structure/fem/AssemblyElementalStructure.hpp>
#include <lifev/core/fem/FESpace.hpp>

#include <lifev/core/LifeV.hpp>
#include <lifev/core/util/Displayer.hpp>

#include <lifev/structure/solver/StructuralConstitutiveLawData.hpp>

#include <lifev/structure/solver/isotropic/StructuralIsotropicConstitutiveLaw.hpp>

#include <lifev/structure/solver/anisotropic/StructuralAnisotropicConstitutiveLaw.hpp>

//ET include for assemblings
#include <lifev/eta/fem/ETFESpace.hpp>
#include <lifev/eta/expression/Integrate.hpp>
#include <lifev/eta/expression/Evaluate.hpp>

namespace LifeV
{
/*!
  \class StructuralConstitutiveLaw
  \brief
  This class is an abstract class to define different type of models for the arterial wall.
  This class has just pure virtual methods. They are implemented in the specific class for one material
*/

template <typename MeshType>
class StructuralConstitutiveLaw
{
public:

    //!@name Type definitions
    //@{
    typedef StructuralConstitutiveLawData          data_Type;


    typedef StructuralIsotropicConstitutiveLaw<MeshType>                 isotropicLaw_Type;
    typedef std::shared_ptr<isotropicLaw_Type>                         isotropicLawPtr_Type;

    typedef StructuralAnisotropicConstitutiveLaw<MeshType>               anisotropicLaw_Type;
    typedef std::shared_ptr<anisotropicLaw_Type>                       anisotropicLawPtr_Type;

    typedef MatrixEpetra<Real>                                           matrix_Type;
    typedef std::shared_ptr<matrix_Type>                               matrixPtr_Type;
    typedef VectorEpetra                                                 vector_Type;
    typedef std::shared_ptr<vector_Type>                               vectorPtr_Type;

    typedef typename std::shared_ptr<data_Type>                        dataPtr_Type;
    typedef typename std::shared_ptr<const Displayer>                  displayerPtr_Type;

    typedef std::vector< typename MeshType::element_Type* >              vectorVolumes_Type;

    typedef std::map< UInt, vectorVolumes_Type>           mapMarkerVolumes_Type;
    typedef std::shared_ptr<mapMarkerVolumes_Type>      mapMarkerVolumesPtr_Type;

    typedef std::vector<UInt>                             vectorIndexes_Type;
    typedef std::map< UInt, vectorIndexes_Type>           mapMarkerIndexes_Type;
    typedef std::shared_ptr<mapMarkerIndexes_Type>      mapMarkerIndexesPtr_Type;


    typedef ETFESpace<MeshType, MapEpetra, 3, 3 >         ETFESpace_Type;
    typedef std::shared_ptr<ETFESpace_Type>             ETFESpacePtr_Type;

    typedef ETFESpace<MeshType, MapEpetra, 3, 1 >         scalarETFESpace_Type;
    typedef std::shared_ptr<scalarETFESpace_Type>       scalarETFESpacePtr_Type;

    typedef FESpace< MeshType, MapEpetra >                FESpace_Type;
    typedef std::shared_ptr<FESpace_Type>               FESpacePtr_Type;

    //Vector for vector parameters
    typedef std::vector<std::vector<Real> >           vectorsParameters_Type;
    typedef std::shared_ptr<vectorsParameters_Type> vectorsParametersPtr_Type;
    //@}



    //! @name Constructor &  Deconstructor
    //@{

    StructuralConstitutiveLaw();

    virtual ~StructuralConstitutiveLaw() {}

    //@}



    //!@name Methods
    //@{

    //! Setup the created object of the class StructuralConstitutiveLaw
    /*!
      \param dFespace: the FiniteElement Space
      \param monolithicMap: the MapEpetra
      \param offset: the offset parameter used assembling the matrices
    */

    void setup ( const FESpacePtr_Type& dFESpace,
		 const ETFESpacePtr_Type& ETFESpace,
		 const std::shared_ptr<const MapEpetra>&   monolithicMap,
		 const UInt offset, const dataPtr_Type& dataMaterial,
		 const displayerPtr_Type& displayer  );


    //! Computes the linear part of the stiffness matrix StructuralSolver::buildSystem
    /*!
      \param dataMaterial the class with Material properties data
    */
    void computeLinearStiff ( dataPtr_Type& dataMaterial,
			      const mapMarkerVolumesPtr_Type /*mapsMarkerVolumes*/,
			      const mapMarkerIndexesPtr_Type /*mapsMarkerIndexes*/ );

    //! Updates the Jacobian matrix in StructuralSolver::updateJacobian
    /*!
      \param disp: solution at the k-th iteration of NonLinearRichardson Method
      \param dataMaterial: a pointer to the dataType member in StructuralSolver class to get the
                           material coefficients (e.g. Young modulus, Poisson ratio..)
      \param displayer: a pointer to the Dysplaier member in the StructuralSolver class
    */
    void updateJacobianMatrix ( const vector_Type& disp, const dataPtr_Type& dataMaterial,
                                const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                                const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                                const displayerPtr_Type& displayer );

    virtual  void updateJacobianMatrix ( const vector_Type& disp, const vector_Type& pressure, const dataPtr_Type& dataMaterial ) {}

    //! Computes the new Stiffness matrix in StructuralSolver given a certain displacement field.
    //! This function is used both in StructuralSolver::evalResidual and in
    //! StructuralSolver::updateSystem since the matrix is the expression of the matrix is the same.
    //!This is virtual and not pure virtual since in the linear St. Venant-Kirchhoff law it is not needed.
    /*!
      \param sol:  the solution vector
      \param factor: scaling factor used in FSI
      \param dataMaterial: a pointer to the dataType member in StructuralSolver class to get the
                           material coefficients (e.g. Young modulus, Poisson ratio..)
      \param displayer: a pointer to the Dysplaier member in the StructuralSolver class
    */
    void computeStiffness ( const vector_Type& sol, const UInt iter, Real factor, const dataPtr_Type& dataMaterial,
                            const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                            const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                            const displayerPtr_Type& displayer );

    virtual  void computeStiffness ( const vector_Type& sol, const vector_Type& pressure,
    		                         const dataPtr_Type& dataMaterial ) {}


    //! Output of the class
    /*!
       \param fileNamelinearStiff the filename where to apply the spy method for the linear part of the Stiffness matrix
       \param fileNameStiff the filename where to apply the spy method for the Stiffness matrix
    */
    void showMe ( std::string const& fileNameStiff, std::string const& fileNameJacobian );

    //! Compute the First Piola Kirchhoff Tensor
    /*!
       \param firstPiola Epetra_SerialDenseMatrix that has to be filled
       \param tensorF Epetra_SerialDenseMatrix the deformation gradient
       \param cofactorF Epetra_SerialDenseMatrix cofactor of F
       \param invariants std::vector with the invariants of C and the detF
       \param material UInt number to get the material parameteres form the VenantElasticData class
    */
    void computeLocalFirstPiolaKirchhoffTensor ( Epetra_SerialDenseMatrix& firstPiola,
                                                 const Epetra_SerialDenseMatrix& tensorF,
                                                 const Epetra_SerialDenseMatrix& cofactorF,
                                                 const std::vector<Real>& invariants,
                                                 const UInt material);

    //! Compute the First Piola Kirchhoff Tensor
    /*!
       \param disp the displacement field from which we compute the fisrt piola-Kirchhoff tensor
       \param sigma_1 the first column of the Cauchy stress tensor
       \param sigma_2 the second column of the Cauchy stress tensor
       \param sigma_3 the third column of the Cauchy stress tensor
    */
    void computeCauchyStressTensor ( const vectorPtr_Type disp,
				     const QuadratureRule& evalQuad,
				     vectorPtr_Type sigma_1,
				     vectorPtr_Type sigma_2,
				     vectorPtr_Type sigma_3);



    //! @name Set Methods
    //@{

    //No set Methods

    //@}


    //! @name Get Methods
    //@{

    //! Getters
    //! Get the Epetramap
    MapEpetra   const& map()     const
    {
        return *M_localMap;
    }

    //! Get the FESpace object
    FESpace_Type& dFESpace()
    {
        return *M_dispFESpace;
    }

    //! Get the FESpace object
    ETFESpace_Type& dETFESpace()
    {
        return *M_dispETFESpace;
    }

    FESpace_Type& pressureFESpace()
    {
        return M_pressureFESpace;
    }

    inline void setPressureETFESpace( scalarETFESpacePtr_Type space )
    {
    	M_pressureETFESpace = space;
    }

    inline void setPressureFESpace( FESpacePtr_Type space )
    {
    	M_pressureFESpace = space;
    }


    //! Get the Stiffness matrix
    matrixPtr_Type const jacobian()    const
    {
        return M_jacobian;
    }

    isotropicLawPtr_Type isotropicLaw( ) const
    {
        return M_isotropicLaw;
    }

    anisotropicLawPtr_Type anisotropicLaw( ) const
    {
        return M_anisotropicLaw;
    }

    //! Get the Stiffness matrix (linear case)
    const matrixPtr_Type  stiffMatrix();

    //! Get the Stiffness vector (nonlinear case)
    const vectorPtr_Type  stiffVector();

    void apply ( const vector_Type& sol, vector_Type& res,
                 const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                 const mapMarkerIndexesPtr_Type mapsMarkerIndexes);

//    vectorsParametersPtr_Type vectorsParameters() const
//    {
//        return M_vectorsParameters;
//    }
    //@}

protected:

    //!Protected Members

    FESpacePtr_Type                                M_dispFESpace;

    ETFESpacePtr_Type                              M_dispETFESpace;

    FESpacePtr_Type                                M_pressureFESpace;

    scalarETFESpacePtr_Type                        M_pressureETFESpace;

    std::shared_ptr<const MapEpetra>             M_localMap;

    //! Matrix jacobian
    matrixPtr_Type                                 M_jacobian;

    //! The Offset parameter
    UInt                                           M_offset;

    dataPtr_Type                                   M_dataMaterial;

    isotropicLawPtr_Type                           M_isotropicLaw;

    anisotropicLawPtr_Type                         M_anisotropicLaw;

    displayerPtr_Type                              M_displayer;

    matrixPtr_Type                                 M_matrixStiffness;

    vectorPtr_Type                                 M_vectorStiffness;
};

//=====================================
// Constructor
//=====================================

template <typename MeshType>
StructuralConstitutiveLaw<MeshType>::StructuralConstitutiveLaw( ) :
    M_dispFESpace                ( ),
    M_dispETFESpace              ( ),
    M_localMap                   ( ),
    M_jacobian                   ( ),
    M_offset                     ( 0 ),
    M_dataMaterial               ( ),
    M_isotropicLaw               ( ),
    M_anisotropicLaw             ( ),
    M_displayer                  ( ),
    M_matrixStiffness            ( ),
    M_vectorStiffness            ( )
{
    //    std::cout << "I am in the constructor of StructuralConstitutiveLaw" << std::endl;
}

template <typename MeshType>
void
StructuralConstitutiveLaw<MeshType>::setup (const FESpacePtr_Type& dFESpace,
                                            const ETFESpacePtr_Type& dETFESpace,
                                            const std::shared_ptr<const MapEpetra>&  monolithicMap,
                                            const UInt offset, const dataPtr_Type& dataMaterial, const displayerPtr_Type& displayer
                                            )
{
    M_dispFESpace                   = dFESpace;
    M_dispETFESpace                 = dETFESpace;
    M_localMap                      = monolithicMap;
    M_jacobian.reset                (new matrix_Type (*M_localMap) );
    M_offset                        = offset;
    M_dataMaterial                  = dataMaterial;

    // Creation of the abstract classes for the isotropic and anisotropic laws
    M_isotropicLaw.reset ( isotropicLaw_Type::StructureIsotropicMaterialFactory::instance().createObject ( M_dataMaterial->solidTypeIsotropic() ) );

    if( !M_dataMaterial->constitutiveLaw().compare("anisotropic") )
      {
          M_anisotropicLaw.reset ( anisotropicLaw_Type::StructureAnisotropicMaterialFactory::instance().createObject ( M_dataMaterial->solidTypeAnisotropic() ) );
      }


    M_displayer = displayer;

    M_matrixStiffness.reset         ( new matrix_Type(*M_localMap) );
    M_vectorStiffness.reset         ( new vector_Type(*M_localMap) );

    // Setting the isotropic and anisotropic part
    M_isotropicLaw->setup( dFESpace, dETFESpace, monolithicMap, offset, M_dataMaterial );
    if( !M_dataMaterial->constitutiveLaw().compare("anisotropic") )
    {
        M_anisotropicLaw->setup( dFESpace, dETFESpace, monolithicMap, offset, M_dataMaterial );
    }

}

template <typename MeshType>
void StructuralConstitutiveLaw<MeshType>::computeLinearStiff (dataPtr_Type& dataMaterial,
                                                              const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                                                              const mapMarkerIndexesPtr_Type mapsMarkerIndexes)
{

  M_isotropicLaw->computeLinearStiff(dataMaterial, mapsMarkerVolumes, mapsMarkerIndexes);

  // The anisotropic part has no need for such a method since it is for sure nonlinear.

}


template <typename MeshType>
void StructuralConstitutiveLaw<MeshType>::updateJacobianMatrix (const vector_Type& disp,
                                                                const dataPtr_Type& dataMaterial,
                                                                const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                                                                const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                                                                const displayerPtr_Type& displayer)
{
    // Resetting and initializing the pointer to the Jacobian
    M_jacobian.reset (new matrix_Type (*M_localMap) );
    *M_jacobian *= 0.0;

    // Isotropic part
    M_displayer->leaderPrint ("\n  S-  Updating the Jacobian Matrix ( isotropic part )\n");
    M_isotropicLaw->updateJacobianMatrix ( disp, dataMaterial, mapsMarkerVolumes, mapsMarkerIndexes, displayer);

    *M_jacobian += *M_isotropicLaw->jacobian();

    if( !M_dataMaterial->constitutiveLaw().compare("anisotropic") )
      {
	// Anisotropic part
	M_displayer->leaderPrint ("\n  S-  Updating the Jacobian Matrix ( anisotropic part )\n");

	M_anisotropicLaw->updateJacobianMatrix (disp, dataMaterial, mapsMarkerVolumes, mapsMarkerIndexes, displayer);

	*M_jacobian += *M_anisotropicLaw->jacobian();
      }

    M_jacobian->globalAssemble();
}

template <typename MeshType>
void StructuralConstitutiveLaw<MeshType>::computeStiffness ( const vector_Type& sol, const UInt iter, Real factor, const dataPtr_Type& dataMaterial,
                                                             const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                                                             const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                                                             const displayerPtr_Type& displayer )
{

    // For the linear elastic case the compute stiffness is an empty method.
    // We use the general interface but then we get the vector using stiffVector

    // Resetting and initializing the pointer to the vector
    M_vectorStiffness.reset (new vector_Type (*M_localMap) );
    *M_vectorStiffness *= 0.0;

    // Isotropic part
    M_displayer->leaderPrint ("\n  S-  Updating the VectorStiffness ( isotropic part )\n");
    M_isotropicLaw->computeStiffness ( sol, factor, dataMaterial, mapsMarkerVolumes, mapsMarkerIndexes, displayer);

    *M_vectorStiffness += *M_isotropicLaw->stiffVector();


    if( !M_dataMaterial->constitutiveLaw().compare("anisotropic") )
    {
        // Anisotropic part
        displayer->leaderPrint ("\n  S-  Updating the VectorStiffness ( anisotropic part )\n");

        // if( !M_dataMaterial->solidTypeAnisotropic().compare("multimechanism") &&
        //     !M_dataMaterial->fiberActivation().compare("explicit") &&
        //     !iter)
        //   {
        //     M_anisotropicLaw->computeReferenceConfigurations( sol, dataMaterial, displayer );
        //   }

        M_anisotropicLaw->computeStiffness (sol, iter, factor, dataMaterial, mapsMarkerVolumes, mapsMarkerIndexes, displayer);

        *M_vectorStiffness += *M_anisotropicLaw->stiffVector();
    }

    M_vectorStiffness->globalAssemble();
}

template <typename MeshType>
void
StructuralConstitutiveLaw<MeshType>::showMe ( std::string const& fileNameStiff,
					      std::string const& fileNameJacobian
					      )
{
    // Spying the isotropic part
    M_isotropicLaw->showMe ( fileNameStiff, fileNameJacobian);

    if( !M_dataMaterial->constitutiveLaw().compare("anisotropic") )
    {
        // Spying the anisotropic part
        M_anisotropicLaw->showMe ( fileNameStiff, fileNameJacobian);
    }

}

template <typename MeshType>
void
StructuralConstitutiveLaw<MeshType>::computeLocalFirstPiolaKirchhoffTensor ( Epetra_SerialDenseMatrix& firstPiola,
                                                                             const Epetra_SerialDenseMatrix& tensorF,
                                                                             const Epetra_SerialDenseMatrix& cofactorF,
                                                                             const std::vector<Real>& invariants,
                                                                             const UInt marker)
{
    firstPiola.Scale(0.0);

    Epetra_SerialDenseMatrix isotropicFirstPiola(firstPiola);


    Epetra_SerialDenseMatrix anisotropicFirstPiola(firstPiola);

    // Computing the first part
    M_isotropicLaw->computeLocalFirstPiolaKirchhoffTensor ( isotropicFirstPiola, tensorF, cofactorF, invariants, marker);

    firstPiola += isotropicFirstPiola;

    if( !M_dataMaterial->constitutiveLaw().compare("anisotropic") )
    {
        // Computing the first part
        M_anisotropicLaw->computeLocalFirstPiolaKirchhoffTensor ( anisotropicFirstPiola, tensorF, cofactorF, invariants, marker);

        firstPiola += anisotropicFirstPiola;
    }

}

template <typename MeshType>
const typename StructuralConstitutiveLaw<MeshType>::matrixPtr_Type StructuralConstitutiveLaw<MeshType>::stiffMatrix()
{
    // Verify that the law that is being using is coherent with the formulation: linear <-> matrix, nonlinear <-> vector
    ASSERT( !M_dataMaterial->solidTypeIsotropic().compare("linearVenantKirchhoff"), " No Stiffness Matrix defined for the nonlinear case! ");

    // Resetting and initializing the pointer to the vector
    M_matrixStiffness.reset (new matrix_Type(*M_localMap) );
    *M_matrixStiffness *= 0.0;

    // Isotropic part
    *M_matrixStiffness += *M_isotropicLaw->stiffMatrix();

    M_matrixStiffness->globalAssemble();

    // Here we have just the return
    return M_matrixStiffness;
}


template <typename MeshType>
const typename StructuralConstitutiveLaw<MeshType>::vectorPtr_Type StructuralConstitutiveLaw<MeshType>::stiffVector()
{
    // Verify that the law that is being using is coherent with the formulation: linear <-> matrix, nonlinear <-> vector
    ASSERT( M_dataMaterial->solidTypeIsotropic().compare("linearVenantKirchhoff"), " No Stiffness Vector defined for the linear case! ");

    // Resetting and initializing the pointer to the vector
    M_vectorStiffness.reset (new vector_Type (*M_localMap) );
    *M_vectorStiffness *= 0.0;

    // Isotropic part
    *M_vectorStiffness += *M_isotropicLaw->stiffVector();

    if( !M_dataMaterial->constitutiveLaw().compare("anisotropic") )
    {
        // Anisotropic part
        *M_vectorStiffness += *M_anisotropicLaw->stiffVector();
    }

    M_vectorStiffness->globalAssemble();

    // Here we have just the return
    return M_vectorStiffness;
}

template <typename MeshType>
void StructuralConstitutiveLaw<MeshType>::apply ( const vector_Type& sol, vector_Type& res,
                                                  const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                                                  const mapMarkerIndexesPtr_Type mapsMarkerIndexes)
{
  // Creating vectors for the isotropic and anisotropic vector which will be summed
  vector_Type copyResIsotropic(res);

  // Computing isotropic and, eventually, anisotropic part of res
  M_isotropicLaw->apply ( sol, copyResIsotropic, mapsMarkerVolumes, mapsMarkerIndexes, M_displayer);

  // Putting the new vectors in the res vector
  res *= 0.0;

  res += copyResIsotropic;

  if( !M_dataMaterial->constitutiveLaw().compare("anisotropic") )
  {
      vector_Type copyResAnisotropic(res);
      M_anisotropicLaw->apply ( sol, copyResAnisotropic, mapsMarkerVolumes, mapsMarkerIndexes, M_displayer);
      res += copyResAnisotropic;
    }

}

template <typename MeshType>
void
StructuralConstitutiveLaw<MeshType>::computeCauchyStressTensor ( const vectorPtr_Type disp,
								 const QuadratureRule& evalQuad,
								 vectorPtr_Type sigma_1,
								 vectorPtr_Type sigma_2, vectorPtr_Type sigma_3 )
{
  /* Resetting pointers
     In this method we use the same vector we pass to it. We could make copies of them and then
     sum them in the right places.
  */
  vectorPtr_Type sigma1CopyIso;
  vectorPtr_Type sigma2CopyIso;
  vectorPtr_Type sigma3CopyIso;

  sigma1CopyIso.reset( new vector_Type(*M_localMap) );
  sigma2CopyIso.reset( new vector_Type(*M_localMap) );
  sigma3CopyIso.reset( new vector_Type(*M_localMap) );

  vectorPtr_Type sigma1CopyAniso;
  vectorPtr_Type sigma2CopyAniso;
  vectorPtr_Type sigma3CopyAniso;

  sigma1CopyAniso.reset( new vector_Type(*M_localMap) );
  sigma2CopyAniso.reset( new vector_Type(*M_localMap) );
  sigma3CopyAniso.reset( new vector_Type(*M_localMap) );

  M_isotropicLaw->computeCauchyStressTensor( disp, evalQuad, sigma1CopyIso, sigma2CopyIso, sigma3CopyIso );

  *sigma_1 += *sigma1CopyIso;
  *sigma_2 += *sigma2CopyIso;
  *sigma_3 += *sigma3CopyIso;

  if( !M_dataMaterial->constitutiveLaw().compare("anisotropic") )
    {
      M_anisotropicLaw->computeCauchyStressTensor( disp, evalQuad, sigma1CopyAniso, sigma2CopyAniso,sigma3CopyAniso );

      *sigma_1 += *sigma1CopyAniso;
      *sigma_2 += *sigma2CopyAniso;
      *sigma_3 += *sigma3CopyAniso;
    }

  // Closing the vectors
  sigma_1->globalAssemble();
  sigma_2->globalAssemble();
  sigma_3->globalAssemble();
}


}
#endif /*_STRUCTURALMATERIAL_H*/
