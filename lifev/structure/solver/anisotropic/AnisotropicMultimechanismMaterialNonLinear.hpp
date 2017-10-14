//@HEADER
/*
*******************************************************************************

    Copyright (C) 2004, 2005, 2007 EPFL, Politecnico di Milano, INRIA
    Copyright (C) 2010 EPFL, Politecnico di Milano, Emory University

    This file is part of LifeV.

    LifeV is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
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
 *  @brief This file contains the definition for the St. Venant Kirchhoff linear material
 *
 *  @version 1.0
 *  @date 29-07-2010
 *  @author Paolo Tricerri
 *
 *  @maintainer  Paolo Tricerri      <paolo.tricerri@epfl.ch>
 */

#ifndef _ANISOTROPICMULTIMECHANISM_H_
#define _ANISOTROPICMULTIMECHANISM_H_

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"


#include <lifev/structure/solver/anisotropic/StructuralAnisotropicConstitutiveLaw.hpp>
#include <lifev/structure/fem/AssemblyElementalStructure.hpp>
#include <lifev/structure/fem/ExpressionDefinitions.hpp>
#include <lifev/core/fem/QuadratureRule.hpp>
#include <lifev/eta/expression/Evaluate.hpp>

#define PI 3.14159265359

namespace LifeV
{

class SelectionFunctor
{
public:
    typedef VectorEpetra                    vector_Type;
    typedef std::shared_ptr<vector_Type>  vectorPtr_Type;

    SelectionFunctor ( )
    :
    M_selectionVector ( ),
    M_value( )
    {}

    SelectionFunctor ( const Real value )
    :
    M_selectionVector ( ),
    M_value( value )
    {}

    ~SelectionFunctor()
    {}

    void setSelectionVector( const vector_Type& selectionVector )
    {
        M_selectionVector.reset( new vector_Type( selectionVector ) );
    }

    void setValue( const Real value )
    {
        M_value = value;
    }

    bool operator() ( const UInt i,
                      const UInt nTotDof,
                      const UInt offset) const
    {
        Int LIDx = M_selectionVector->blockMap().LID( static_cast<EpetraInt_Type>(i) );
        Int LIDy = M_selectionVector->blockMap().LID( static_cast<EpetraInt_Type>(i + nTotDof + offset) );
        Int LIDz = M_selectionVector->blockMap().LID( static_cast<EpetraInt_Type>(i + 2 * nTotDof + offset) );

        /*
           Note: the selectionVector of this functor has the same map the
           origin vector in the AssemblyElementalStructure namespace.
           Therefore, once the i-th Dof is verified to be part of the
           blockMap in the origin vector the following assert should never fail.
        */
        ASSERT( ( LIDx >= 0 ) &&
                ( LIDy >= 0 ) &&
                ( LIDz >= 0 ) , "Problem in the epetra maps cut!" );
        /*
          Since the quantity that we are checking it's scalar, we only
          check one component.
         */

        Int GIDx = M_selectionVector->blockMap().GID( LIDx );
        Int GIDy = M_selectionVector->blockMap().GID( LIDy );
        Int GIDz = M_selectionVector->blockMap().GID( LIDz );

        // if( std::fabs(( *M_selectionVector )( GIDx )) <= M_value ||
	    // ( *M_selectionVector )( GIDx ) > 0 )

        if( ( *M_selectionVector )( GIDx ) > M_value )
        {
            return true;
        }

        else
        {
            return false;
        }

        return false;

    }

protected:
    vectorPtr_Type M_selectionVector;
    Real           M_value;
}; // selector functor

class booleanSelector
{
public:
    typedef VectorEpetra                    vector_Type;
    typedef std::shared_ptr<vector_Type>  vectorPtr_Type;

    booleanSelector ( const vector_Type& selectionVector )
    :
    M_selectionVector ( selectionVector )
    {}

    ~booleanSelector()
    {}

    bool operator() ( const UInt i,
                      const UInt nTotDof,
                      const UInt offset ) const
    {
        Int LIDx = M_selectionVector.blockMap().LID( static_cast<EpetraInt_Type>(i) );
        Int LIDy = M_selectionVector.blockMap().LID( static_cast<EpetraInt_Type>(i + nTotDof + offset) );
        Int LIDz = M_selectionVector.blockMap().LID( static_cast<EpetraInt_Type>(i + 2 * nTotDof + offset) );

        /*
           Note: the selectionVector of this functor is the same as the
           origin vector in the AssemblyElementalStructure namespace.
           Therefore, once the i-th Dof is verified to be part of the
           blockMap in the origin vector the following assert should never fail.
        */
        ASSERT( ( LIDx >= 0 ) &&
                ( LIDy >= 0 ) &&
                ( LIDz >= 0 ) , "Problem in the epetra maps cut!" );

        Int GIDx = M_selectionVector.blockMap().GID( LIDx );
        Int GIDy = M_selectionVector.blockMap().GID( LIDy );
        Int GIDz = M_selectionVector.blockMap().GID( LIDz );

        if( ( M_selectionVector )( GIDx ) == 1.0 ||
            ( M_selectionVector )( GIDy ) == 1.0 ||
            ( M_selectionVector )( GIDz ) == 1.0 )
        {
            return true;
        }
        else
        {
            return false;
        }

        return false;

    }

protected:
    vector_Type M_selectionVector;
}; // selector functor


template <typename MeshType>
class AnisotropicMultimechanismMaterialNonLinear : public StructuralAnisotropicConstitutiveLaw<MeshType>
{
    //!@name Type definitions
    //@{

public:
    typedef StructuralAnisotropicConstitutiveLaw<MeshType>           super;

    typedef typename super::data_Type                data_Type;

    typedef typename super::vector_Type              vector_Type;
    typedef typename super::matrix_Type              matrix_Type;

    typedef typename super::matrixPtr_Type           matrixPtr_Type;
    typedef typename super::vectorPtr_Type           vectorPtr_Type;
    typedef typename super::dataPtr_Type             dataPtr_Type;
    typedef typename super::displayerPtr_Type        displayerPtr_Type;

    typedef typename super::mapMarkerVolumesPtr_Type mapMarkerVolumesPtr_Type;
    typedef typename super::mapMarkerVolumes_Type mapMarkerVolumes_Type;
    typedef typename mapMarkerVolumes_Type::const_iterator mapIterator_Type;

    typedef typename super::mapMarkerIndexesPtr_Type mapMarkerIndexesPtr_Type;
    typedef typename super::mapMarkerIndexes_Type    mapMarkerIndexes_Type;
    typedef typename mapMarkerIndexes_Type::const_iterator mapIteratorIndex_Type;

    typedef std::vector<typename MeshType::element_Type*> vectorVolumes_Type;
    typedef std::shared_ptr<vectorVolumes_Type>    vectorVolumesPtr_Type;

    typedef std::vector<UInt>                        vectorIndexes_Type;
    typedef std::shared_ptr<vectorIndexes_Type>    vectorIndexesPtr_Type;

    typedef typename super::FESpace_Type             FESpace_Type;
    typedef typename super::FESpacePtr_Type          FESpacePtr_Type;
    typedef typename super::ETFESpacePtr_Type        ETFESpacePtr_Type;

    typedef ETFESpace< RegionMesh<LinearTetra>, MapEpetra, 3, 1 >       scalarETFESpace_Type;
    typedef std::shared_ptr<scalarETFESpace_Type>                     scalarETFESpacePtr_Type;

    //Vector for vector parameters
    typedef typename super::vectorsParameters_Type       vectorsParameters_Type;
    typedef typename super::vectorsParametersPtr_Type    vectorsParametersPtr_Type;

    typedef MatrixSmall<3, 3>                          matrixSmall_Type;

    typedef typename super::fiberFunction_Type            fiberFunction_Type;
    typedef typename super::fiberFunctionPtr_Type         fiberFunctionPtr_Type;

    typedef typename super::vectorFiberFunction_Type      vectorFiberFunction_Type;
    typedef typename super::vectorFiberFunctionPtr_Type   vectorFiberFunctionPtr_Type;

    typedef std::shared_ptr<QuadratureRule>             quadratureRulePtr_Type;

    typedef std::vector<SelectionFunctor>                selectionFunctors_Type;

    // Typedefs for expression definitions
    typedef typename super::tensorF_Type               tensorF_Type;
    typedef typename super::determinantF_Type          determinantF_Type;
    typedef typename super::tensorC_Type               tensorC_Type;
    typedef typename super::minusT_Type                minusT_Type;
    typedef typename super::traceTensor_Type           traceTensor_Type;
    typedef typename super::powerExpression_Type       powerExpression_Type;
    typedef typename
    ExpressionDefinitions::isochoricChangeOfVariable_Type isochoricDet_Type;

    // Anisotropic typedefs
    typedef typename super::interpolatedValue_Type       interpolatedValue_Type;
    typedef typename ExpressionDefinitions::interpolatedScalarValue_Type  interpolatedScalarValue_Type;
    typedef typename super::outerProduct_Type            outerProduct_Type;
    typedef typename super::stretch_Type                 stretch_Type;
    typedef typename super::isochoricStretch_Type        isochoricStretch_Type;

    typedef ExpressionDefinitions::inverseTensor_Type                         invTensor_Type;
    typedef ExpressionMultimechanism::rightCauchyGreenMultiMechanism_Type     tensorCmultiMech_Type;
    typedef ExpressionMultimechanism::activatedFiber_Type                     activateFiber_Type;
    typedef ExpressionMultimechanism::normActivatedFiber_Type                 normActivateFiber_Type;
    typedef ExpressionMultimechanism::normalizedVector_Type                   normalizedVector_Type;
    typedef ExpressionMultimechanism::activatedDeterminantF_Type              activatedDeterminantF_Type;
    typedef ExpressionMultimechanism::activePowerExpression_Type              activePowerExpression_Type;
    typedef ExpressionMultimechanism::activeNormalizedOuterProduct_Type       activeNormalizedOuterProduct_Type;
    typedef ExpressionMultimechanism::activeStretch_Type                      activeStretch_Type;
    typedef ExpressionMultimechanism::activeInterpolatedFiberStretch_Type     activeInterpolatedStretch_Type;
    typedef ExpressionMultimechanism::activeIsochoricStretch_Type             activeIsochoricStretch_Type;
    typedef ExpressionMultimechanism::activeIsochoricDeterminant_Type         activeIsochoricDeterminant_Type;

    typedef ExpressionMultimechanism::deformationActivatedTensor_Type         deformationActivatedTensor_Type;
    typedef ExpressionMultimechanism::activeMinusTtensor_Type                 activeMinusTtensor_Type;
    typedef ExpressionMultimechanism::activeLinearization_Type                activeLinearization_Type;
    typedef ExpressionMultimechanism::activeTestGradient_Type                 activeTestGradient_Type;
    //@}



    //! @name Constructor &  Destructor
    //@{

    AnisotropicMultimechanismMaterialNonLinear();

    virtual  ~AnisotropicMultimechanismMaterialNonLinear();

    //@}



    //!@name Methods
    //@{

    //! Setup the created object of the class StructuralIsotropicConstitutiveLaw
    /*!
      \param dFespace: the FiniteElement Space
      \param monolithicMap: the MapEpetra
      \param offset: the offset parameter used assembling the matrices
    */
    void setup ( const FESpacePtr_Type& dFESpace,
                 const ETFESpacePtr_Type& dETFESpace,
                 const std::shared_ptr<const MapEpetra>&  monolithicMap,
                 const UInt offset,const dataPtr_Type& dataMaterial);


    //! Compute the Stiffness matrix in StructuralSolver::buildSystem()
    /*!
      \param dataMaterial the class with Material properties data
    */
    void computeLinearStiff ( dataPtr_Type& /*dataMaterial*/,
                              const mapMarkerVolumesPtr_Type /*mapsMarkerVolumes*/,
                              const mapMarkerIndexesPtr_Type /*mapsMarkerIndexes*/);


    //! Updates the Jacobian matrix in StructualSolver::updateJacobian
    /*!
      \param disp: solution at the k-th iteration of NonLinearRichardson Method
      \param dataMaterial: a pointer to the dataType member in StructuralSolver class to get
                           the material coefficients (e.g. Young modulus, Poisson ratio..)
      \param displayer: a pointer to the Dysplaier member in the StructuralSolver class
    */
    void updateJacobianMatrix ( const vector_Type& disp,
                                const dataPtr_Type& dataMaterial,
                                const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                                const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                                const displayerPtr_Type& displayer );


    //! Updates the nonlinear terms in the Jacobian matrix in StructualSolver::updateJacobian
    /*!
      \param stiff: stiffness matrix provided from outside
      \param disp: solution at the k-th iteration of NonLinearRichardson Method
      \param dataMaterial: a pointer to the dataType member in StructuralSolver class to get
                           the material coefficients (e.g. Young modulus, Poisson ratio..)
      \param displayer: a pointer to the Dysplaier member in the StructuralSolver class
    */
    void updateNonLinearJacobianTerms ( matrixPtr_Type& jacobian,
                                        const vector_Type& disp,
                                        const dataPtr_Type& dataMaterial,
                                        const mapMarkerVolumesPtr_Type /*mapsMarkerVolumes*/,
                                        const mapMarkerIndexesPtr_Type /*mapsMarkerIndexes*/,
                                        const displayerPtr_Type& displayer );


    //! Interface method to compute the new Stiffness matrix in StructuralSolver::evalResidual and in
    //! StructuralSolver::updateSystem since the matrix is the expression of the matrix is the same.
    /*!
      \param sol:  the solution vector
      \param factor: scaling factor used in FSI
      \param dataMaterial: a pointer to the dataType member in StructuralSolver class to get the
                           material coefficients (e.g. Young modulus, Poisson ratio..)
      \param displayer: a pointer to the Dysplaier member in the StructuralSolver class
    */
    void computeStiffness ( const vector_Type& disp,
                            const UInt iter,
                            Real factor,
                            const dataPtr_Type& dataMaterial,
                            const mapMarkerVolumesPtr_Type /*mapsMarkerVolumes*/,
                            const mapMarkerIndexesPtr_Type /*mapsMarkerIndexes*/,
                            const displayerPtr_Type& displayer );

    void computeReferenceConfigurations ( const vector_Type& disp,
                                          const dataPtr_Type& dataMaterial,
                                          const displayerPtr_Type& displayer );


    //! Computes the new Stiffness vector for Neo-Hookean and Holzapfel materials in StructuralSolver
    //! given a certain displacement field.
    //! This function is used both in StructuralSolver::evalResidual and in StructuralSolver::updateSystem
    //! since the matrix is the expression of the matrix is the same.
    /*!
      \param sol:  the solution vector
      \param factor: scaling factor used in FSI
      \param dataMaterial: a pointer to the dataType member in StructuralSolver class to get
                           the material coefficients (e.g. Young modulus, Poisson ratio..)
      \param displayer: a pointer to the Dysplaier member in the StructuralSolver class
    */

    //! Computes the deformation gradient F, the cofactor matrix Cof(F),
    //! the determinant of F (J = det(F)), the trace of right Cauchy-Green tensor tr(C)
    //! This function is used in StructuralIsotropicConstitutiveLaw::computeStiffness
    /*!
      \param dk_loc: the elemental displacement
    */
    void computeKinematicsVariables ( const VectorElemental& dk_loc )
    {
    }

    //! Computes the deformation Gradient F, the cofactor of F Cof(F), the determinant of F J = det(F), the trace of C Tr(C).
    /*!
      \param dk_loc: local displacement vector
    */
    //void computeStress( const vector_Type& sol );

    //! ShowMe method of the class (saved on a file the stiffness vector and the jacobian)
    void showMe ( std::string const& fileNameVectStiff,
                  std::string const& fileNameJacobain );

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
                                                 const UInt marker);

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


    void setupFiberDirections( vectorFiberFunctionPtr_Type vectorOfFibers );


    void evaluateActivationFibers( const vector_Type&  displacement,
                                   vector_Type&  fourthInvariant){}
    //@}

    //! @name Get Methods
    //@{

    //! Get the Stiffness matrix
    matrixPtr_Type  const stiffMatrix() const
    {
        return super::M_jacobian;
    }


    //! Get the stiffness vector
    vectorPtr_Type  const stiffVector() const
    {
        return M_stiff;
    }

    //! Specific for multimechanism
    vectorPtr_Type  const selectionCriterion( const UInt i ) const
    {
      ASSERT( i <= this->M_vectorInterpolated.size(), " No such fiber family in the class" );
      return M_selectionCriterion[ i - 1 ];
    }


    vectorPtr_Type  const activationDisplacement( const UInt i ) const
    {
      ASSERT( i <= this->M_vectorInterpolated.size(), " No such fiber family in the class" );
      return M_activationDisplacement[ i - 1 ];
    }

    vectorPtr_Type  const activatedUnitFiber( const UInt i ) const
    {
      ASSERT( i <= this->M_vectorInterpolated.size(), " No such fiber family in the class" );
      return M_unitFiberActivation[ i - 1 ];
    }

    vectorPtr_Type  const activatedDeterminant( const UInt i ) const
    {
      ASSERT( i <= this->M_vectorInterpolated.size(), " No such fiber family in the class" );
      return M_jacobianActivation[ i - 1 ];
    }


    void apply ( const vector_Type& sol, vector_Type& res,
                 const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                 const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                 const displayerPtr_Type displayer) ;

    //@}



protected:

    //! construct the vectors for the parameters
    /*!
      \param VOID
      \return VOID
    */
    // void setupVectorsParameters ( void );

    quadratureRulePtr_Type                M_quadrature;
    vectorPtr_Type                        M_patchAreaVector;
    vectorPtr_Type                        M_patchAreaVectorScalar;

    //! Vector: stiffness non-linear
    std::vector<vectorPtr_Type>            M_selectionCriterion;

    //! Vector: stiffness non-linear
    selectionFunctors_Type                 M_selector;

    //! Vector: stiffness non-linear
    std::vector<vectorPtr_Type>            M_firstActivation;

    //! Vector: stiffness non-linear
    std::vector<vectorPtr_Type>            M_activationDisplacement;


    //! Kinematics quantities related to the activation
    std::vector<vectorPtr_Type>            M_jacobianActivation;
    std::vector<vectorPtr_Type>            M_unitFiberActivation;

    //! Vector: stiffness non-linear
    vectorPtr_Type                         M_stiff;

    //Create the indentity for F
    matrixSmall_Type                       M_identity;

    // Scalar ET FESpace to reconstruct scalar fields
    scalarETFESpacePtr_Type                M_scalarETFESpace;
};





template <typename MeshType>
AnisotropicMultimechanismMaterialNonLinear<MeshType>::AnisotropicMultimechanismMaterialNonLinear() :
    super                     ( ),
    M_quadrature              ( ),
    M_patchAreaVector         ( ),
    M_patchAreaVectorScalar   ( ),
    M_selectionCriterion      (0),
    M_selector                (0),
    M_firstActivation         (0),
    M_activationDisplacement  (0),
    M_jacobianActivation      (0),
    M_unitFiberActivation     (0),
    M_stiff                   ( ),
    M_identity                ( ),
    M_scalarETFESpace         ( )
{
}





template <typename MeshType>
AnisotropicMultimechanismMaterialNonLinear<MeshType>::~AnisotropicMultimechanismMaterialNonLinear()
{}





template <typename MeshType>
void
AnisotropicMultimechanismMaterialNonLinear<MeshType>::setup ( const FESpacePtr_Type&                       dFESpace,
					      const ETFESpacePtr_Type&                     dETFESpace,
					      const std::shared_ptr<const MapEpetra>&   monolithicMap,
					      const UInt                                  offset,
					      const dataPtr_Type& dataMaterial)
{

    this->M_dataMaterial                = dataMaterial;
    this->M_dispFESpace                 = dFESpace;
    this->M_dispETFESpace               = dETFESpace;
    this->M_localMap                    = monolithicMap;
    this->M_offset                      = offset;


    this->M_scalarETFESpace.reset ( new scalarETFESpace_Type (dFESpace->mesh(), & (dFESpace->refFE() ),
                                                              & (dFESpace->fe().geoMap() ), dFESpace->mapPtr()->commPtr() ) );

    // Setting the quadrature rule for the evaluation
    QuadratureRule fakeQuadratureRule;

    Real refElemArea (0); //area of reference element
    //compute the area of reference element
    for (UInt iq = 0; iq < this->M_dispFESpace->qr().nbQuadPt(); iq++)
    {
        refElemArea += this->M_dispFESpace->qr().weight (iq);
    }

    Real wQuad (refElemArea / this->M_dispFESpace->refFE().nbDof() );

    //Setting the quadrature Points = DOFs of the element and weight = 1
    std::vector<GeoVector> coords = this->M_dispFESpace->refFE().refCoor();
    std::vector<Real> weights (this->M_dispFESpace->fe().nbFEDof(), wQuad);
    fakeQuadratureRule.setDimensionShape ( shapeDimension (this->M_dispFESpace->refFE().shape() ), this->M_dispFESpace->refFE().shape() );
    fakeQuadratureRule.setPoints (coords, weights);

    M_quadrature.reset( new QuadratureRule( fakeQuadratureRule ) );
    M_patchAreaVector.reset(new vector_Type(*this->M_localMap) );

    M_patchAreaVectorScalar.reset(new vector_Type( this->M_scalarETFESpace->map() ) );

    // Sizing the std::vectors
    M_selectionCriterion.resize( this->M_dataMaterial->numberFibersFamilies() );
    M_selector.resize( this->M_dataMaterial->numberFibersFamilies() );
    M_firstActivation.resize( this->M_dataMaterial->numberFibersFamilies() );
    M_activationDisplacement.resize( this->M_dataMaterial->numberFibersFamilies() );

    M_jacobianActivation.resize( this->M_dataMaterial->numberFibersFamilies() );
    M_unitFiberActivation.resize( this->M_dataMaterial->numberFibersFamilies() );

    //Resetting pointers
    for( UInt i(0); i < this->M_dataMaterial->numberFibersFamilies(); i++ )
    {
        (M_selectionCriterion[i]).reset(new vector_Type (*this->M_localMap) );
        (M_firstActivation[i]).reset(new vector_Type (*this->M_localMap) );
        *(M_firstActivation[i]) *= 0.0;

        (M_activationDisplacement[i]).reset(new vector_Type (*this->M_localMap) );

        (M_jacobianActivation[i]).reset(new vector_Type ( M_scalarETFESpace->map() ) );
        (M_unitFiberActivation[i]).reset(new vector_Type (*this->M_localMap) );
    }


    M_stiff.reset                       (new vector_Type (*this->M_localMap) );

    M_identity (0, 0) = 1.0;
    M_identity (0, 1) = 0.0;
    M_identity (0, 2) = 0.0;
    M_identity (1, 0) = 0.0;
    M_identity (1, 1) = 1.0;
    M_identity (1, 2) = 0.0;
    M_identity (2, 0) = 0.0;
    M_identity (2, 1) = 0.0;
    M_identity (2, 2) = 1.0;

    this->M_epsilon = this->M_dataMaterial->smoothness();


    // Computing patch area vector
    // Assembling
    ExpressionVectorFromNonConstantScalar<ExpressionMeas, 3  > vMeas( meas_K );
    evaluateNode( elements ( this->M_dispETFESpace->mesh() ),
                  *M_quadrature,
                  this->M_dispETFESpace,
                  dot( vMeas , phi_i )
                  ) >> M_patchAreaVector;
    M_patchAreaVector->globalAssemble();

    evaluateNode( elements ( this->M_scalarETFESpace->mesh() ),
                  *M_quadrature,
                  this->M_scalarETFESpace,
                  meas_K * phi_i
                  ) >> M_patchAreaVectorScalar;
    M_patchAreaVectorScalar->globalAssemble();

    //this->setupVectorsParameters( );
}


// template <typename MeshType>
// void
// AnisotropicMultimechanismMaterialNonLinear<MeshType>::setupVectorsParameters ( void )
// {
//     // Paolo Tricerri: February, 20th
//     // In each class, the name of the parameters has to inserted in the law
//     // TODO: move the saving of the material parameters to more abstract objects
//     //       such that in the class of the material we do not need to call explicitly
//     //       the name of the parameter.

//     // Number of volume on the local part of the mesh
//     UInt nbElements = this->M_dispFESpace->mesh()->numVolumes();

//     // Parameter alpha
//     // 1. resize the vector in the first element of the vector.
//     (* (this->M_vectorsParameters) ) [0].resize ( nbElements );

//     // Parameter gamma
//     (* (this->M_vectorsParameters) ) [1].resize ( nbElements );

//     // Parameter bulk
//     (* (this->M_vectorsParameters) ) [2].resize ( nbElements );

//     for (UInt i (0); i < nbElements; i++ )
//     {
//         // Extracting the marker
//         UInt markerID = this->M_dispFESpace->mesh()->element ( i ).markerID();

//         Real alpha = this->M_dataMaterial->alpha ( markerID );
//         Real gamma = this->M_dataMaterial->gamma ( markerID );
//         Real bulk  = this->M_dataMaterial->bulk ( markerID );

//         ( (* (this->M_vectorsParameters) ) [0]) [ i ] = alpha;
//         ( (* (this->M_vectorsParameters) ) [1]) [ i ] = gamma;
//         ( (* (this->M_vectorsParameters) ) [2]) [ i ] = bulk;
//     }
// }


template <typename MeshType>
void
AnisotropicMultimechanismMaterialNonLinear<MeshType>::setupFiberDirections ( vectorFiberFunctionPtr_Type vectorOfFibers  )
{
    // Allocating the right space for the vector of fiber function
    this->M_vectorOfFibers.reset( new vectorFiberFunction_Type( ) );

    // In this method, the vector of fiber functions has to be properly set  in the main
    // of the test. The functions are then projected on a FESpace for the integration

    // Number of volume on the local part of the mesh
    UInt nbFamilies = (*vectorOfFibers).size();

    ASSERT( nbFamilies == this->M_dataMaterial->numberFibersFamilies(),
	    " The number of families set in the test is different from the one in the data" );

    this->M_vectorOfFibers->resize( nbFamilies );

    for( UInt k(0); k < nbFamilies; k++ )
    {
        ( *(this->M_vectorOfFibers) )[ k ] = ( *vectorOfFibers )[ k ];
    }

    // Setting the vectors that will be used
    this->M_vectorInterpolated.resize( nbFamilies );

    for( UInt k(0); k < nbFamilies; k++ )
    {
        this->M_vectorInterpolated[ k ].reset( new vector_Type(*this->M_localMap) );
        this->M_dispFESpace->interpolate ( *( ( *(this->M_vectorOfFibers) )[ k ] ) ,
                                           * ( ( this->M_vectorInterpolated )[ k ] ),
                                           0.0 );
    }

}


template <typename MeshType>
void AnisotropicMultimechanismMaterialNonLinear<MeshType>::computeLinearStiff (dataPtr_Type& /*dataMaterial*/,
                                                                               const mapMarkerVolumesPtr_Type /*mapsMarkerVolumes*/,
                                                                               const mapMarkerIndexesPtr_Type /*mapsMarkerIndexes*/)
{
    //! Empty method for exponential material
}





template <typename MeshType>
void AnisotropicMultimechanismMaterialNonLinear<MeshType>::updateJacobianMatrix ( const vector_Type&       disp,
                                                                                  const dataPtr_Type&      dataMaterial,
                                                                                  const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                                                                                  const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                                                                                  const displayerPtr_Type& displayer )
{

    this->M_jacobian.reset (new matrix_Type (*this->M_localMap) );

    displayer->leaderPrint (" \n*********************************\n  ");
    updateNonLinearJacobianTerms (this->M_jacobian, disp, this->M_dataMaterial, mapsMarkerVolumes, mapsMarkerIndexes, displayer);
    displayer->leaderPrint (" \n*********************************\n  ");

}

template <typename MeshType>
void AnisotropicMultimechanismMaterialNonLinear<MeshType>::updateNonLinearJacobianTerms ( matrixPtr_Type&         jacobian,
                                                                                          const vector_Type&     disp,
                                                                                          const dataPtr_Type&     dataMaterial,
                                                                                          const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                                                                                          const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                                                                                          const displayerPtr_Type& displayer )
{
    using namespace ExpressionAssembly;

    // In the following definitions, the critical template argument is MapEpetra
    // When other maps will be available in LifeV, the Holzapfel class and its mother
    // should have as template the MeshType and the MapType.

    // Definition of F
    tensorF_Type F = ExpressionDefinitions::deformationGradient( this->M_dispETFESpace,  disp, this->M_offset, this->M_identity );

    // Definition of J
    determinantF_Type J = ExpressionDefinitions::determinantF( F );

    //Definition of C
    tensorC_Type C = ExpressionDefinitions::tensorC( transpose(F), F );

    // Definition of J^-(2/3) = det( C ) using the isochoric/volumetric decomposition
    isochoricDet_Type  Jel = ExpressionDefinitions::isochoricDeterminant( J );

    // Definition of F^-T
    minusT_Type  F_T = ExpressionDefinitions::minusT( F );

    // Update the heaviside function for the stretch of the fibers
    * (jacobian) *= 0.0;

    displayer->leaderPrint ("   Non-Linear S - updating non linear terms in the Jacobian Matrix (Multi-mechanism model): \n");
    displayer->leaderPrint ("   Non-Linear S - the computation of the activation configuration has been carried out in computeStiffness: \n");

    for( UInt i(0); i < this->M_vectorInterpolated.size(); i++ )
    {
        if( M_activationDisplacement[ i ]->norm2() )
        {
            displayer->leaderPrint ("                ", i + 1,"-th fiber family \n" );

            // As in other classes, the specialization of the MapType = MapEpetra makes this expression
            // not always usable. When other maps will be available in LifeV, the class should be re-templated.

            // Definition of F_0(ta)
            tensorF_Type ithFzeroA = ExpressionDefinitions::deformationGradient( this->M_dispETFESpace,  *(M_activationDisplacement[ i ]),
                                                                                 this->M_offset, this->M_identity );

            // Determinant of F_0(ta)
            interpolatedScalarValue_Type ithJzeroA = ExpressionDefinitions::interpolateScalarValue( this->M_scalarETFESpace, *( M_jacobianActivation[ i ] ) );

            // Definition of J_a
            activatedDeterminantF_Type Ja = ExpressionMultimechanism::activateDeterminantF( J, ithJzeroA );

            // Definition of J_a^{-2.0/3.0}
            activeIsochoricDeterminant_Type  JactiveEl = ExpressionMultimechanism::activeIsochoricDeterminant( Ja );

            // Definition of F_0^{-1}(ta)
            invTensor_Type FzeroAminus1 = ExpressionDefinitions::inv( ithFzeroA );

            // Definition of Fa = F_0 * F_0(ta)^{-1}
            deformationActivatedTensor_Type Fa = ExpressionMultimechanism::createDeformationActivationTensor( F , FzeroAminus1);

            // Definition of F_0^{-T}(ta)
            minusT_Type FzeroAminusT = ExpressionDefinitions::minusT( ithFzeroA );

            activeMinusTtensor_Type FAminusT = ExpressionMultimechanism::createActiveMinusTtensor( F_T, transpose( ithFzeroA ) );
            // Definition of C_a = F_0^{-T}(ta) * C_0 * F_0^{-1}(ta)
            tensorCmultiMech_Type Ca = ExpressionMultimechanism::activationRightCauchyGreen( FzeroAminusT, C, FzeroAminus1 );

            // Interpolating the unit fiber computed before
            interpolatedValue_Type transformedFiber = ExpressionDefinitions::interpolateValue( this->M_dispETFESpace, *( M_unitFiberActivation[ i ] ) );

            // Definition of the tensor M = ithFiber \otimes ithFiber
            // At the moment, it's automatic that the method constructs the expression M = ithFiber \otimes ithFiber
            // For a more general case, the file ExpressionDefinitions.hpp should be changed
            outerProduct_Type Mith = ExpressionDefinitions::fiberTensor( transformedFiber );

            // Definition of the fourth invariant : I_4^i = C:Mith
            activeInterpolatedStretch_Type IVith = ExpressionMultimechanism::activeInterpolatedFiberStretch( Ca, Mith );

            // Definition of the fouth isochoric invariant : J^(-2.0/3.0) * I_4^i
            activeIsochoricStretch_Type IVithBar = ExpressionMultimechanism::activeIsochoricFourthInvariant( JactiveEl, IVith );

            // Linearization with respect to activation configuration
            activeLinearization_Type dFa = ExpressionMultimechanism::activatedLinearization( grad(phi_j), FzeroAminus1 );

            activeTestGradient_Type grPhiI = ExpressionMultimechanism::activatedTestGradient( grad(phi_i), FzeroAminus1);

            integrate( elements ( this->M_dispETFESpace->mesh() ),
                       this->M_dispFESpace->qr(),
                       this->M_dispETFESpace,
                       this->M_dispETFESpace,
                       ithJzeroA * value( 2.0 ) * value( this->M_dataMaterial->ithStiffnessFibers( i ) ) * JactiveEl *
                       exp( value( this->M_dataMaterial->ithNonlinearityFibers( i ) ) * ( IVithBar- value( 1.0 ) ) * ( IVithBar- value( 1.0 ) ) ) *
                       (
                        derAtan( IVithBar - value( 1.0 ), this->M_epsilon, ( 1.0 / PI ) ) * ( IVithBar - value(1.0) ) +
                        atan( IVithBar - value(1.0), this->M_epsilon, ( 1 / PI ), (1.0/2.0) ) +
                        atan( IVithBar - value(1.0), this->M_epsilon, ( 1 / PI ), (1.0/2.0) ) *
                        value( 2.0 ) * value( this->M_dataMaterial->ithNonlinearityFibers( i ) ) *
                        ( IVithBar- value( 1.0 ) ) * ( IVithBar- value( 1.0 ) )
                        ) *
                       ( dot( value(-2.0/3.0) * IVithBar * FAminusT, dFa ) + JactiveEl * dot( transpose(dFa)*Fa + transpose(Fa) * dFa , Mith ) ) *
                       dot( Fa * Mith - value(1.0/3.0) * IVith * FAminusT, grPhiI )
                       ) >> jacobian;

            integrate( elements ( this->M_dispETFESpace->mesh() ),
                       this->M_dispFESpace->qr(),
                       this->M_dispETFESpace,
                       this->M_dispETFESpace,
                       ithJzeroA * value( 2.0 ) * value( this->M_dataMaterial->ithStiffnessFibers( i ) ) *
                       exp( value( this->M_dataMaterial->ithNonlinearityFibers( i ) ) * ( IVithBar- value( 1.0 ) ) * ( IVithBar- value( 1.0 ) ) ) *
                       atan( IVithBar - value(1.0), this->M_epsilon, ( 1 / PI ), (1.0/2.0) ) * ( IVithBar - value(1.0) ) *
                       dot( value(-2.0/3.0) * JactiveEl * FAminusT , dFa ) *
                       dot( Fa * Mith - value(1.0/3.0) * IVith * FAminusT, grPhiI )
                       ) >> jacobian;

            integrate( elements ( this->M_dispETFESpace->mesh() ),
                       this->M_dispFESpace->qr(),
                       this->M_dispETFESpace,
                       this->M_dispETFESpace,
                       ithJzeroA * value( 2.0 ) * value( this->M_dataMaterial->ithStiffnessFibers( i ) ) *
                       JactiveEl * ( IVithBar - value( 1.0 ) ) * atan( IVithBar - value(1.0), this->M_epsilon, ( 1 / PI ), (1.0/2.0) ) *
                       exp( value( this->M_dataMaterial->ithNonlinearityFibers( i ) ) * ( IVithBar- value( 1.0 ) ) * ( IVithBar- value( 1.0 ) ) ) *
                       dot( dFa * Mith - value(1.0/3.0) * ( dot( transpose(dFa)*Fa + transpose(Fa) * dFa , Mith ) ) * FAminusT +
                            value(1.0/3.0) * IVith * FAminusT * transpose(dFa) * FAminusT, grPhiI )
                       ) >> jacobian;

        }
        else
        {
            displayer->leaderPrint ("The activation displacement of the ", i + 1,"-th fiber family is null. No Jacobian! \n" );
        }
    } // closing loop on fibers
    jacobian->globalAssemble();
}





template <typename MeshType>
void AnisotropicMultimechanismMaterialNonLinear<MeshType>::computeStiffness ( const vector_Type& disp,
                                                                              const UInt iter,
                                                                              Real /*factor*/,
                                                                              const dataPtr_Type& dataMaterial,
                                                                              const mapMarkerVolumesPtr_Type /*mapsMarkerVolumes*/,
                                                                              const mapMarkerIndexesPtr_Type /*mapsMarkerIndexes*/,
                                                                              const displayerPtr_Type& displayer )
{

    using namespace ExpressionAssembly;

    M_stiff.reset (new vector_Type (*this->M_localMap) );
    * (M_stiff) *= 0.0;

    displayer->leaderPrint (" \n*********************************\n  ");
    displayer->leaderPrint (" Non-Linear S-  Computing the Multi-mechanism nonlinear stiffness vector \n");
    displayer->leaderPrint (" \n*********************************\n  ");

    if( ( !this->M_dataMaterial->fiberActivation().compare("implicit") ||
          !iter /* iter == 0 means explicit approach */ ) )
    {
        this->computeReferenceConfigurations( disp, this->M_dataMaterial, displayer );
    }

    // For anisotropic part of the Piola-Kirchhoff is assemble summing up the parts of the
    // Piola-Kirchhoff using the fiber index

    // Here the fiber vector at the quadrature node is deduced using the method
    // Interpolate value of the expression template.

    // Definition of F
    tensorF_Type F = ExpressionDefinitions::deformationGradient( this->M_dispETFESpace,  disp, this->M_offset, this->M_identity );

    // Definition of J
    determinantF_Type J = ExpressionDefinitions::determinantF( F );

    //Definition of C
    tensorC_Type C = ExpressionDefinitions::tensorC( transpose(F), F );

    // Definition of J^-(2/3) = det( C ) using the isochoric/volumetric decomposition
    isochoricDet_Type  Jel = ExpressionDefinitions::isochoricDeterminant( J );

    // Definition of F^-T
    minusT_Type  F_T = ExpressionDefinitions::minusT( F );

    displayer->leaderPrint (" Non-Linear S-  Computing contributions to the stiffness vector... \n");

    for( UInt i(0); i < this->M_vectorInterpolated.size() ; i++ )
    {
        if( M_activationDisplacement[ i ]->norm2() )
        {

            displayer->leaderPrint ("                ", i + 1,"-th fiber family \n" );
            // As in other classes, the specialization of the MapType = MapEpetra makes this expression
            // not always usable. When other maps will be available in LifeV, the class should be re-templated.

            // Definition of F_0(ta)
            tensorF_Type ithFzeroA = ExpressionDefinitions::deformationGradient( this->M_dispETFESpace,  *(M_activationDisplacement[ i ]),
                                                                                 this->M_offset, this->M_identity );

            // Determinant of F_0(ta)
            interpolatedScalarValue_Type ithJzeroA = ExpressionDefinitions::interpolateScalarValue( this->M_scalarETFESpace, *( M_jacobianActivation[ i ] ) );

            // Definition of J_a
            activatedDeterminantF_Type Ja = ExpressionMultimechanism::activateDeterminantF( J, ithJzeroA );

            // Definition of J_a^{-2.0/3.0}
            activeIsochoricDeterminant_Type  JactiveEl = ExpressionMultimechanism::activeIsochoricDeterminant( Ja );

            // Definition of F_0^{-1}(ta)
            invTensor_Type FzeroAminus1 = ExpressionDefinitions::inv( ithFzeroA );

            // Definition of Fa = F_0 * F_0(ta)^{-1}
            deformationActivatedTensor_Type Fa = ExpressionMultimechanism::createDeformationActivationTensor( F , FzeroAminus1);

            // Definition of F_0^{-T}(ta)
            minusT_Type FzeroAminusT = ExpressionDefinitions::minusT( ithFzeroA );

            activeMinusTtensor_Type FAminusT = ExpressionMultimechanism::createActiveMinusTtensor( F_T, transpose( ithFzeroA ) );
            // Definition of C_a = F_0^{-T}(ta) * C_0 * F_0^{-1}(ta)
            tensorCmultiMech_Type Ca = ExpressionMultimechanism::activationRightCauchyGreen( FzeroAminusT, C, FzeroAminus1 );

            interpolatedValue_Type fiberIth =
                ExpressionDefinitions::interpolateFiber( this->M_dispETFESpace, *(this->M_unitFiberActivation[ i ] ) );

            // Definition of the tensor M = ithFiber \otimes ithFiber
            // At the moment, it's automatic that the method constructs the expression M = ithFiber \otimes ithFiber
            // For a more general case, the file ExpressionDefinitions.hpp should be changed
            outerProduct_Type Mith = ExpressionDefinitions::fiberTensor( fiberIth );

            // Definition of the fourth invariant : I_4^i = C:Mith
            activeInterpolatedStretch_Type IVith = ExpressionMultimechanism::activeInterpolatedFiberStretch( Ca, Mith );

            // Definition of the fouth isochoric invariant : J^(-2.0/3.0) * I_4^i
            activeIsochoricStretch_Type IVithBar = ExpressionMultimechanism::activeIsochoricFourthInvariant( JactiveEl, IVith );

            // vectorPtr_Type checkIVithBar( new vector_Type( M_scalarETFESpace->map() ) );

            // *checkIVithBar *= 0.0;
            // evaluateNode ( elements ( this->M_scalarETFESpace->mesh() ),
            //                *M_quadrature,
            //                this->M_scalarETFESpace,
            //                meas_K *  atan( IVithBar - value( 1.0 ) , this->M_epsilon, ( 1 / PI ), ( 1.0/2.0 )  )  * ithJzeroA *
            //                (value( 2.0 ) * value( this->M_dataMaterial->ithStiffnessFibers( i ) ) * JactiveEl * ( IVithBar - value( 1.0 ) ) *
            //                 exp( value( this->M_dataMaterial->ithNonlinearityFibers( i ) ) * ( IVithBar- value( 1.0 ) ) * ( IVithBar- value( 1.0 ) )  ) ) * phi_i
            //                ) >> checkIVithBar;
            // checkIVithBar->globalAssemble();
            // *checkIVithBar = *checkIVithBar / *M_patchAreaVectorScalar;
            // std::cout << "norm of the vector"<< checkIVithBar->norm2()<< std::endl;
            // std::cout << "normInf of the vector"<< checkIVithBar->normInf()<< std::endl;
            // checkIVithBar->spy("checkVector");

            // The terms for the piola kirchhoff tensor come from the holzapfel model. Then they are rescaled
            // according to the change of variable given by the multi-mechanism model.

            // // First term:
            // // 2 alpha_i J^(-2.0/3.0) ( \bar{I_4} - 1 ) exp( gamma_i * (\bar{I_4} - 1)^2 ) * F M : \grad phi_i
            // // where alpha_i and gamma_i are the fiber parameters and M is the 2nd order tensor of type f_i \otimes \ f_i

            integrate ( elements ( this->M_dispETFESpace->mesh() ),
                        this->M_dispFESpace->qr(),
                        this->M_dispETFESpace,
                        atan( IVithBar - value( 1.0 ) , this->M_epsilon, ( 1 / PI ), ( 1.0/2.0 ) ) * ithJzeroA *
                        (value( 2.0 ) * value( this->M_dataMaterial->ithStiffnessFibers( i ) ) * JactiveEl * ( IVithBar - value( 1.0 ) ) *
                         exp( value( this->M_dataMaterial->ithNonlinearityFibers( i ) ) *
                              ( IVithBar - value( 1.0 ) ) * ( IVithBar - value( 1.0 ))))*
                        dot( ( Fa * Mith - value(1.0/3.0) * IVithBar * FAminusT) * FzeroAminusT, grad(phi_i) )
                        ) >> this->M_stiff;
        }
        else
        {
            displayer->leaderPrint ("The activation displacement of the ", i + 1,"-th fiber family is null. No contribution to stiffness! \n" );
        }
    }
    this->M_stiff->globalAssemble();


}


template <typename MeshType>
void AnisotropicMultimechanismMaterialNonLinear<MeshType>::showMe ( std::string const& fileNameStiff,
                                                      std::string const& fileNameJacobian )
{
    this->M_stiff->spy (fileNameStiff);
    this->M_jacobian->spy (fileNameJacobian);
}


template <typename MeshType>
void AnisotropicMultimechanismMaterialNonLinear<MeshType>::apply ( const vector_Type& sol,
								   vector_Type& res, const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
								   const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
								   const displayerPtr_Type displayer)
{
    computeStiffness (sol, 0, 0, this->M_dataMaterial, mapsMarkerVolumes, mapsMarkerIndexes, displayer);
    res += *M_stiff;
}


template <typename MeshType>
void AnisotropicMultimechanismMaterialNonLinear<MeshType>::computeReferenceConfigurations ( const vector_Type& disp,
                                                                                            const dataPtr_Type& dataMaterial,
                                                                                            const displayerPtr_Type& displayer )
{
    displayer->leaderPrint (" Non-Linear S-  Computing reference configurations... \n");
    vectorPtr_Type booleanVector;

    // Definition of F
    tensorF_Type F = ExpressionDefinitions::deformationGradient( this->M_dispETFESpace,  disp, this->M_offset, this->M_identity );

    // Definition of J
    determinantF_Type J = ExpressionDefinitions::determinantF( F );

    //Definition of C
    tensorC_Type C = ExpressionDefinitions::tensorC( transpose(F), F );

    // // Definition of J^-(2/3) = det( C ) using the isochoric/volumetric decomposition
    // isochoricDet_Type  Jel = ExpressionDefinitions::isochoricDeterminant( J  );

    // // Definition of F^-T
    // minusT_Type  F_T = ExpressionDefinitions::minusT( F );

    // 1. Evaluating fiber stretch
    for( UInt i(0); i < this->M_vectorInterpolated.size() ; i++ )
    {

        displayer->leaderPrint ("                ", i + 1,"-th fiber family \n" );
        // Note: M_vectorInterpolated.size() == numberOfFibers which has to be equal,
        // given a certain assert in the data class to the number of characteristic stretches
        // and therefore to the size of the vector that are used to measure the activation.

        // Initializing vectors
        (M_selectionCriterion[i]).reset(new vector_Type (*this->M_localMap) );
        *(M_selectionCriterion[i]) *= 0.0;

        // As in other classes, the specialization of the MapType = MapEpetra makes this expression
        // not always usable. When other maps will be available in LifeV, the class should be re-templated.

        // Defining the expression for the i-th fiber
        // Definitions of the quantities which depend on the fiber directions e.g. I_4^i
        interpolatedValue_Type fiberIth =
            ExpressionDefinitions::interpolateFiber( this->M_dispETFESpace, *(this->M_vectorInterpolated[ i ] ) );

        // Definition of the tensor M = ithFiber \otimes ithFiber
        // At the moment, it's automatic that the method constructs the expression M = ithFiber \otimes ithFiber
        // For a more general case, the file ExpressionDefinitions.hpp should be changed
        outerProduct_Type Mith = ExpressionDefinitions::fiberTensor( fiberIth );

        // Definition of the fourth invariant : I_4^i = C:Mith
        stretch_Type IVith = ExpressionDefinitions::fiberStretch( C, Mith );


        Real referenceStretch = this->M_dataMaterial->ithCharacteristicStretch(i) * this->M_dataMaterial->ithCharacteristicStretch(i);
        // Difference between IVith - IVth(t_A)
        ExpressionMultimechanism::incompressibleDifference_Type absStretch =
            ExpressionMultimechanism::incompressibleAbsoluteStretch( IVith, referenceStretch );

        // Difference between IVith - IVth(t_A) / IVth(t_A)
        ExpressionMultimechanism::relativeDifference_Type relStretch =
            ExpressionMultimechanism::relativeDifference( absStretch, referenceStretch );

        // Trick to have vector with the scalar expression
        ExpressionMultimechanism::expressionVectorFromRelativeDifference_Type vActivation =
            ExpressionMultimechanism::vectorFromRelativeDifference( relStretch );

        // Computing expression that determines activation
        evaluateNode( elements ( this->M_dispETFESpace->mesh() ),
                      *M_quadrature,
                      this->M_dispETFESpace,
                      meas_K * dot( vActivation , phi_i )
                      ) >> M_selectionCriterion[ i ];
        M_selectionCriterion[ i ]->globalAssemble();
        *( M_selectionCriterion[ i ] ) = *( M_selectionCriterion[ i ] ) / *M_patchAreaVector;

        // Setting values in the selector
        M_selector[i].setSelectionVector( *(M_selectionCriterion[i]) );
        M_selector[i].setValue( this->M_dataMaterial->toleranceActivation() );

        // Saving the vector;
        AssemblyElementalStructure::saveVectorAccordingToFunctor( this->M_dispFESpace, M_selector[ i ],
                                                                  disp, this->M_firstActivation[i],
                                                                  M_activationDisplacement[i], this->M_offset);

        vectorPtr_Type regularizationVector(new vector_Type( *this->M_localMap ) );
        *regularizationVector *= 0.0;

        evaluateNode( elements ( this->M_dispETFESpace->mesh() ),
                      *M_quadrature,
                      this->M_dispETFESpace,
                      meas_K * dot( value( this->M_dispETFESpace, *(M_activationDisplacement[i]) ) , phi_i )
                      ) >> regularizationVector;
        regularizationVector->globalAssemble();
        *(regularizationVector ) = *( regularizationVector ) / *M_patchAreaVector;

        *M_activationDisplacement[i] = *regularizationVector;

        booleanVector.reset(new vector_Type( *this->M_localMap ) );
        *booleanVector *= 0.0;

        //vector_Type vectorReference( *(M_activationDisplacement[i]) );
        vector_Type vectorReference( *(M_firstActivation[i]) );

        std::cout << "Activation: " << M_firstActivation[i]->norm2() << std::endl;
        std::cout << "Activation: " << M_activationDisplacement[i]->norm2() << std::endl;

        booleanSelector boolSelector(vectorReference);
        AssemblyElementalStructure::saveBooleanVectorAccordingToFunctor( this->M_dispFESpace, boolSelector, (M_activationDisplacement[i]),
                                                                         booleanVector, this->M_offset);

        *( M_jacobianActivation[ i ] ) *= 0.0;
        *( M_unitFiberActivation[ i ]) *= 0.0;

        if( M_activationDisplacement[i]->norm2() )
        {
            /*
              For each of the fiber families the jacobian, the activation stretch
              normalized fiber are computed
            */

            // Jacobian
            tensorF_Type Fa = ExpressionDefinitions::deformationGradient( this->M_dispETFESpace, *(M_activationDisplacement[i]),
                                                                          this->M_offset, this->M_identity );
            // Definition of J
            determinantF_Type Ja = ExpressionDefinitions::determinantF( Fa );

            evaluateNode( elements ( M_scalarETFESpace->mesh() ),
                          *M_quadrature,
                          M_scalarETFESpace,
                          meas_K * Ja  * phi_i
                          ) >> M_jacobianActivation[ i ];
            M_jacobianActivation[ i ]->globalAssemble();
            *( M_jacobianActivation[ i ] ) = *( M_jacobianActivation[ i ] ) / *M_patchAreaVectorScalar;
            // Normalized fiber
            // Definitions of the quantities which depend on the fiber directions e.g. I_4^i
            interpolatedValue_Type fiberIth = ExpressionDefinitions::interpolateFiber( this->M_dispETFESpace, *(this->M_vectorInterpolated[ i ] ) );

            // Definition of the direction of the fiber at the activation moment = F_0(ta) * f_0
            activateFiber_Type activeIthFiber = ExpressionMultimechanism::activateFiberDirection( Fa, fiberIth );

            normalizedVector_Type normalizedFiber = ExpressionMultimechanism::unitVector( activeIthFiber );
            evaluateNode( elements ( this->M_dispETFESpace->mesh() ),
                          *M_quadrature,
                          this->M_dispETFESpace,
                          meas_K * dot( normalizedFiber , phi_i )
                          ) >> M_unitFiberActivation[ i ];
            M_unitFiberActivation[ i ]->globalAssemble();
            *( M_unitFiberActivation[ i ] ) = *( M_unitFiberActivation[ i ] ) / *M_patchAreaVector;

            // Resetting the vector of the fiber to zero when the element of the displacement are zero.
            *( M_unitFiberActivation[ i ] ) = *( M_unitFiberActivation[ i ] ) * ( *booleanVector );

            // // Definition of F_0^{-1}(ta)
            // invTensor_Type FzeroAminus1 = ExpressionDefinitions::inv( Fa );
            // // Definition of F_0^{-T}(ta)
            // minusT_Type FzeroAminusT = ExpressionDefinitions::minusT( Fa );
            // // Definition of C_a = F_0^{-T}(ta) * C_0 * F_0^{-1}(ta)
            // tensorCmultiMech_Type Ca = ExpressionMultimechanism::activationRightCauchyGreen( FzeroAminusT, C, FzeroAminus1 );

            // activeNormalizedOuterProduct_Type Mith = ExpressionMultimechanism::activeNormalizedOuterProduct( normalizedFiber );

            // // Definition of the fourth invariant : I_4^i = C:Mith
            // activeStretch_Type IVith = ExpressionMultimechanism::activeFiberStretch( Ca, Mith );

            // evaluateNode( elements ( M_scalarETFESpace->mesh() ),
            //       *M_quadrature,
            //       M_scalarETFESpace,
            //       meas_K * IVith  * phi_i
            //       ) >> M_stretchActivation[ i ];
            // M_stretchActivation[ i ]->globalAssemble();
            // *( M_stretchActivation[ i ] ) = *( M_stretchActivation[ i ] ) / *M_patchAreaVectorScalar;
        }
        //( M_unitFiberActivation[ i ] )->spy("interpolatedFiberActivation");
    }
}

template <typename MeshType>
void AnisotropicMultimechanismMaterialNonLinear<MeshType>::computeLocalFirstPiolaKirchhoffTensor ( Epetra_SerialDenseMatrix& firstPiola,
                                                                                                   const Epetra_SerialDenseMatrix& tensorF,
                                                                                                   const Epetra_SerialDenseMatrix& cofactorF,
                                                                                                   const std::vector<Real>& invariants,
                                                                                                   const UInt marker)
{

    // It can be done using the evaluateNodal framework that has been shown to work
    // for the isotropic laws!

}

template <typename MeshType>
void AnisotropicMultimechanismMaterialNonLinear<MeshType>::computeCauchyStressTensor ( const vectorPtr_Type disp,
										       const QuadratureRule& evalQuad,
										       vectorPtr_Type sigma_1,
										       vectorPtr_Type sigma_2,
										       vectorPtr_Type sigma_3)
{

  ASSERT( 2 < 0 , "For the multimechanism law the computation of the Cauchy stress has to be defined." );

}



template <typename MeshType>
inline StructuralAnisotropicConstitutiveLaw<MeshType>* createAnisotropicMultimechanismMaterialNonLinear()
{
    return new AnisotropicMultimechanismMaterialNonLinear<MeshType >();
}

namespace
{
static bool registerAMM = StructuralAnisotropicConstitutiveLaw<LifeV::RegionMesh<LinearTetra> >::StructureAnisotropicMaterialFactory::instance().registerProduct ( "multimechanism", &createAnisotropicMultimechanismMaterialNonLinear<LifeV::RegionMesh<LinearTetra> > );
}

}

#endif
