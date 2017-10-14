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

#ifndef _DISTRIBUTEDHOLZAPFELMATERIAL_H_
#define _DISTRIBUTEDHOLZAPFELMATERIAL_H_

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"


#include <lifev/structure/solver/anisotropic/StructuralAnisotropicConstitutiveLaw.hpp>

#define PI 3.14159265359

namespace LifeV
{
template <typename MeshType>
class DistributedHolzapfelMaterialNonLinear : public StructuralAnisotropicConstitutiveLaw<MeshType>
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

    //Vector for vector parameters
    typedef typename super::vectorsParameters_Type       vectorsParameters_Type;
    typedef typename super::vectorsParametersPtr_Type    vectorsParametersPtr_Type;

    typedef MatrixSmall<3, 3>                          matrixSmall_Type;

    typedef typename super::fiberFunction_Type                fiberFunction_Type;
    typedef typename super::fiberFunctionPtr_Type             fiberFunctionPtr_Type;

    typedef typename super::vectorFiberFunction_Type          vectorFiberFunction_Type;
    typedef typename super::vectorFiberFunctionPtr_Type          vectorFiberFunctionPtr_Type;


    // Typedefs for expression definitions
    typedef typename super::tensorF_Type               tensorF_Type;
    typedef typename super::determinantF_Type          determinantF_Type;
    typedef typename super::tensorC_Type               tensorC_Type;
    typedef typename super::minusT_Type                minusT_Type;
    typedef typename super::traceTensor_Type           traceTensor_Type;
    typedef typename super::powerExpression_Type       powerExpression_Type;
    typedef typename super::isochoricTrace_Type        isochoricTrace_Type;

    // Anisotropic typedefs
    typedef typename super::interpolatedValue_Type       interpolatedValue_Type;
    typedef typename super::outerProduct_Type            outerProduct_Type;
    typedef typename super::stretch_Type                 stretch_Type;
    typedef typename super::isochoricStretch_Type        isochoricStretch_Type;

    // typedefs specific for this model that are defined in ExpressionDefinitions.hpp
    typedef ExpressionDistributedModel::distributedStretch_Type      distributedStretch_Type;
    typedef ExpressionDistributedModel::tensorialPart_distrType      tensorialPart_distrType;
    typedef ExpressionDistributedModel::linearizationDistributedStretch_Type derivativeDistrStretch_Type;
    //@}



    //! @name Constructor &  Destructor
    //@{

    DistributedHolzapfelMaterialNonLinear();

    virtual  ~DistributedHolzapfelMaterialNonLinear();

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
					  const displayerPtr_Type& displayer ){};

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
    vectorPtr_Type  const selectionCriterion( const UInt /*i*/ ) const
    {
      vectorPtr_Type empty( new vector_Type( this->M_dispFESpace->map() ) );
      return empty;
    }


    vectorPtr_Type  const activationDisplacement( const UInt /*i*/ ) const
    {
      vectorPtr_Type empty( new vector_Type( this->M_dispFESpace->map() ) );
      return empty;
    }

    vectorPtr_Type  const activatedUnitFiber( const UInt /*i*/ ) const
    {
      vectorPtr_Type empty( new vector_Type( this->M_dispFESpace->map() ) );
      return empty;
    }

    vectorPtr_Type  const activatedDeterminant( const UInt /*i*/ ) const
    {
      vectorPtr_Type empty( new vector_Type( this->M_dispFESpace->map() ) );
      return empty;
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


    //! Vector: stiffness non-linear
    vectorPtr_Type                         M_stiff;

    //Create the indentity for F
    matrixSmall_Type                      M_identity;

};





template <typename MeshType>
DistributedHolzapfelMaterialNonLinear<MeshType>::DistributedHolzapfelMaterialNonLinear() :
    super            ( ),
    M_stiff          ( ),
    M_identity       ( )
{
}





template <typename MeshType>
DistributedHolzapfelMaterialNonLinear<MeshType>::~DistributedHolzapfelMaterialNonLinear()
{}





template <typename MeshType>
void
DistributedHolzapfelMaterialNonLinear<MeshType>::setup ( const FESpacePtr_Type&                       dFESpace,
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
    //this->setupVectorsParameters( );
}


// template <typename MeshType>
// void
// DistributedHolzapfelMaterialNonLinear<MeshType>::setupVectorsParameters ( void )
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
DistributedHolzapfelMaterialNonLinear<MeshType>::setupFiberDirections ( vectorFiberFunctionPtr_Type vectorOfFibers  )
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
void DistributedHolzapfelMaterialNonLinear<MeshType>::computeLinearStiff (dataPtr_Type& /*dataMaterial*/,
                                                                 const mapMarkerVolumesPtr_Type /*mapsMarkerVolumes*/,
                                                                 const mapMarkerIndexesPtr_Type /*mapsMarkerIndexes*/)
{
    //! Empty method for exponential material
}





template <typename MeshType>
void DistributedHolzapfelMaterialNonLinear<MeshType>::updateJacobianMatrix ( const vector_Type&       disp,
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
void DistributedHolzapfelMaterialNonLinear<MeshType>::updateNonLinearJacobianTerms ( matrixPtr_Type&         jacobian,
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

    // Definition of J^-(2/3) = det( C ) using the isochoric/volumetric decomposition
    powerExpression_Type  Jel = ExpressionDefinitions::powerExpression( J , (-2.0/3.0) );

    // Definition of F^-T
    minusT_Type  F_T = ExpressionDefinitions::minusT( F );

    //Definition of C
    tensorC_Type C = ExpressionDefinitions::tensorC( transpose(F), F );

    // Definition of tr( C )
    traceTensor_Type I_C = ExpressionDefinitions::traceTensor( C );

    // Definition of J^( -2.0/3.0 ) * tr( C )
    isochoricTrace_Type trCbar = ExpressionDefinitions::isochoricTrace( Jel, I_C );

    // Update the heaviside function for the stretch of the fibers
    * (jacobian) *= 0.0;

    displayer->leaderPrint ("   Non-Linear S - updating non linear terms in the Jacobian Matrix (Distributed Holzapfel): \n");

    for( UInt i(0); i < this->M_vectorInterpolated.size(); i++ )
    {

        displayer->leaderPrint ("                ", i + 1,"-th fiber family \n" );

        // Material parameters
        Real kappaI  = this->M_dataMaterial->ithDistributionFibers( i );
        Real alphaI = this->M_dataMaterial->ithStiffnessFibers( i );
        Real gammaI = this->M_dataMaterial->ithNonlinearityFibers( i );

        // Defining the expression for the i-th fiber
        // Definitions of the quantities which depend on the fiber directions e.g. I_4^i
        interpolatedValue_Type fiberIth = ExpressionDefinitions::interpolateFiber( this->M_dispETFESpace, *(this->M_vectorInterpolated[ i ] ) );

        // Definition of the tensor M = ithFiber \otimes ithFiber
        // At the moment, it's automatic that the method constructs the expression M = ithFiber \otimes ithFiber
        // For a more general case, the file ExpressionDefinitions.hpp should be changed
        outerProduct_Type Mith = ExpressionDefinitions::fiberTensor( fiberIth );

        // Definition of the fourth invariant : I_4^i = C:Mith
        stretch_Type IVith = ExpressionDefinitions::fiberStretch( C, Mith );

        // Definition of the fouth isochoric invariant : J^(-2.0/3.0) * I_4^i
        isochoricStretch_Type IVithBar = ExpressionDefinitions::isochoricFourthInvariant( Jel, IVith );

        // Definition of the quantity to measure stretch in the distributed case
        // \kappaI * \bar{I_C} + ( 1 - 3\kappaI ) * \bar{I_4^i} - 1.0
        distributedStretch_Type distrStretch = ExpressionDistributedModel::distributedStretch( trCbar, IVithBar, kappaI );

        // Definition of the matricial ( i.e. tensorial part ) of the Piola - Kirchhoff )
        tensorialPart_distrType tensorialPart = ExpressionDistributedModel::tensorialPartPiola( kappaI, I_C, IVith, F, F_T, Mith );

        // Definition of the derivative w.r.t to F of the distributed stretch
        derivativeDistrStretch_Type derivativeStretch = ExpressionDistributedModel::derivativeDistributedStretch( kappaI, trCbar, IVithBar,
                                                                                                                  Jel, F, F_T, Mith);

        // first term:
        integrate( elements ( this->M_dispETFESpace->mesh() ),
                   this->M_dispFESpace->qr(),
                   this->M_dispETFESpace,
                   this->M_dispETFESpace,
                   value( 2.0 * alphaI ) * exp( value( gammaI  ) * ( distrStretch ) * ( distrStretch ) ) *
                   Jel * distrStretch * derAtan( distrStretch , this->M_epsilon, ( 1.0 / PI ) ) *
                   derivativeStretch * dot( tensorialPart , grad(phi_i) )
                   ) >> jacobian;

        // second term:
        integrate( elements ( this->M_dispETFESpace->mesh() ),
                   this->M_dispFESpace->qr(),
                   this->M_dispETFESpace,
                   this->M_dispETFESpace,
                   atan(  distrStretch , this->M_epsilon, ( 1 / PI ), ( 1.0/2.0 )  ) *
                   value( 2.0 * alphaI ) * exp( value( gammaI  ) * ( distrStretch ) * ( distrStretch ) ) *
                   Jel * derivativeStretch * dot( tensorialPart , grad(phi_i) )
                   ) >> jacobian;

        // third term:
        integrate( elements ( this->M_dispETFESpace->mesh() ),
                   this->M_dispFESpace->qr(),
                   this->M_dispETFESpace,
                   this->M_dispETFESpace,
                   atan(  distrStretch , this->M_epsilon, ( 1 / PI ), ( 1.0/2.0 )  ) *
                   value( 2.0 * alphaI ) * Jel * distrStretch *
                   exp( value( gammaI  ) * ( distrStretch ) * ( distrStretch ) ) *
                   value( 2.0 * gammaI ) * distrStretch * derivativeStretch * dot( tensorialPart , grad(phi_i) )
                   ) >> jacobian;

        // fourth term:
        integrate( elements ( this->M_dispETFESpace->mesh() ),
                   this->M_dispFESpace->qr(),
                   this->M_dispETFESpace,
                   this->M_dispETFESpace,
                   atan(  distrStretch , this->M_epsilon, ( 1 / PI ), ( 1.0/2.0 )  ) *
                   value( 2.0 * alphaI ) *  distrStretch *
                   exp( value( gammaI  ) * ( distrStretch ) * ( distrStretch ) ) *
                   value( -2.0/3.0 ) * Jel * dot( F_T, grad(phi_j) ) * dot( tensorialPart , grad(phi_i) )
                   ) >> jacobian;

        // fifth term:
        integrate( elements ( this->M_dispETFESpace->mesh() ),
                   this->M_dispFESpace->qr(),
                   this->M_dispETFESpace,
                   this->M_dispETFESpace,
                   atan(  distrStretch , this->M_epsilon, ( 1 / PI ), ( 1.0/2.0 )  ) *
                   value( 2.0 * alphaI ) *  distrStretch *
                   exp( value( gammaI ) * ( distrStretch ) * ( distrStretch ) ) * Jel *
                   dot( value( kappaI ) * grad(phi_j) +
                        value( - ( 2.0 * kappaI )/ 3.0 ) * dot( F, grad(phi_j) ) * F_T  +
                        value( kappaI / 3.0 ) * I_C * F_T * transpose( grad(phi_j) ) * F_T +
                        value( 1.0 - 3.0 * kappaI ) * grad(phi_j) * Mith +
                        value( ( 1.0 - 3.0*kappaI ) / 3.0 ) * IVith *  F_T * transpose( grad(phi_j) ) * F_T +
                        value(-( 1.0 - 3.0*kappaI ) / 3.0 ) * ( dot( transpose(grad(phi_j)) * F , Mith ) + dot( transpose(F)*grad(phi_j), Mith ) ) * F_T , grad(phi_i) )
                   ) >> jacobian;



    } // closing loop on fibers

    jacobian->globalAssemble();
}





template <typename MeshType>
void DistributedHolzapfelMaterialNonLinear<MeshType>::computeStiffness ( const vector_Type& disp,
									 const UInt /*iter*/,
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
    displayer->leaderPrint (" Non-Linear S-  Computing the Distributed Holzapfel nonlinear stiffness vector (Distributed Holzapfel) ");
    displayer->leaderPrint (" \n*********************************\n  ");

    // For anisotropic part of the Piola-Kirchhoff is assemble summing up the parts of the
    // Piola-Kirchhoff using the fiber index

    // Here the fiber vector at the quadrature node is deduced using the method
    // Interpolate value of the expression template.

    // Defining quantities which depend of the displacement field and not on the fiber direction

    // Definition of F
    tensorF_Type F = ExpressionDefinitions::deformationGradient( this->M_dispETFESpace,  disp, this->M_offset, this->M_identity );

    // Definition of J
    determinantF_Type J = ExpressionDefinitions::determinantF( F );

    //Definition of C
    tensorC_Type C = ExpressionDefinitions::tensorC( transpose(F), F );

    // Definition I_C
    traceTensor_Type I_C = ExpressionDefinitions::traceTensor( C );

    // Definition of J^-(2/3) = det( C ) using the isochoric/volumetric decomposition
    powerExpression_Type  Jel = ExpressionDefinitions::powerExpression( J , (-2.0/3.0) );

    // Definition of the isochoric trace of C
    isochoricTrace_Type trCbar = ExpressionDefinitions::isochoricTrace( Jel, I_C );

    // Definition of F^-T
    minusT_Type  F_T = ExpressionDefinitions::minusT( F );

    for( UInt i(0); i < this->M_vectorInterpolated.size() ; i++ )
      {
          // As in other classes, the specialization of the MapType = MapEpetra makes this expression
          // not always usable. When other maps will be available in LifeV, the class should be re-templated.

          // Defining the expression for the i-th fiber
          // Definitions of the quantities which depend on the fiber directions e.g. I_4^i
          interpolatedValue_Type fiberIth = ExpressionDefinitions::interpolateFiber( this->M_dispETFESpace, *(this->M_vectorInterpolated[ i ] ) );

          // Definition of the tensor M = ithFiber \otimes ithFiber
          // At the moment, it's automatic that the method constructs the expression M = ithFiber \otimes ithFiber
          // For a more general case, the file ExpressionDefinitions.hpp should be changed
          outerProduct_Type Mith = ExpressionDefinitions::fiberTensor( fiberIth );

          // Definition of the fourth invariant : I_4^i = C:Mith
          stretch_Type IVith = ExpressionDefinitions::fiberStretch( C, Mith );

          // Definition of the fouth isochoric invariant : J^(-2.0/3.0) * I_4^i
          isochoricStretch_Type IVithBar = ExpressionDefinitions::isochoricFourthInvariant( Jel, IVith );

          // Material parameters
          Real kappaI = this->M_dataMaterial->ithDistributionFibers( i );
          Real alphaI = this->M_dataMaterial->ithStiffnessFibers( i );
          Real gammaI = this->M_dataMaterial->ithNonlinearityFibers( i );

          // Definition of the activation for distributed model
          distributedStretch_Type distrStretch = ExpressionDistributedModel::distributedStretch( trCbar, IVithBar, kappaI );

          // For this model, the activation in front of the stress tensor is of the form
          // atan( k * \bar{I_C} + ( 1 - 3*k ) * \bar(I_4) - 1 )

          // First term:
          // 2 alpha_i J^(-2.0/3.0) ( k \bar{I_1}  + ( 1 - 3*k) \bar{I_4} - 1 ) ( k )
          // exp( gamma_i * ( k \bar{I_1} + ( 1 - 3*k) \bar{I_4} - 1)^2 ) * ( F  - 1.3 * I_1 * F^-T) : \grad phi_i
          // where alpha_i and gamma_i are the fiber parameters and M is the 2nd order tensor of type f_i \otimes \ f_i

          // Second term:
          // 2 alpha_i J^(-2.0/3.0) ( k \bar{I_1}  + ( 1 - 3*k) \bar{I_4} - 1 ) ( 1 - 3k )
          // exp( gamma_i * ( k \bar{I_1} + ( 1 - 3*k) \bar{I_4} - 1)^2 ) * ( FM  - 1.3 * I_4 * F^-T) : \grad phi_i
          // where alpha_i and gamma_i are the fiber parameters and M is the 2nd order tensor of type f_i \otimes \ f_i

          // Definition of the matricial ( i.e. tensorial part ) of the Piola - Kirchhoff )
          tensorialPart_distrType tensorialPart = ExpressionDistributedModel::tensorialPartPiola( kappaI, I_C, IVith, F, F_T, Mith );

          integrate ( elements ( this->M_dispETFESpace->mesh() ),
                      this->M_dispFESpace->qr(),
                      this->M_dispETFESpace,
                      atan(  distrStretch , this->M_epsilon, ( 1 / PI ), ( 1.0/2.0 )  ) *
                      value( 2.0 * alphaI ) * Jel * ( distrStretch ) *
                      exp( value( gammaI  ) * ( distrStretch ) * ( distrStretch ) ) *
                      dot( tensorialPart, grad( phi_i ) )
                      ) >> this->M_stiff;

      }

    this->M_stiff->globalAssemble();
}


template <typename MeshType>
void DistributedHolzapfelMaterialNonLinear<MeshType>::showMe ( std::string const& fileNameStiff,
                                                      std::string const& fileNameJacobian )
{
    this->M_stiff->spy (fileNameStiff);
    this->M_jacobian->spy (fileNameJacobian);
}


template <typename MeshType>
void DistributedHolzapfelMaterialNonLinear<MeshType>::apply ( const vector_Type& sol,
                                                   vector_Type& res, const mapMarkerVolumesPtr_Type mapsMarkerVolumes,
                                                   const mapMarkerIndexesPtr_Type mapsMarkerIndexes,
                                                   const displayerPtr_Type displayer)
{
    computeStiffness (sol, 0, 0, this->M_dataMaterial, mapsMarkerVolumes, mapsMarkerIndexes, displayer);
    res += *M_stiff;
}


template <typename MeshType>
void DistributedHolzapfelMaterialNonLinear<MeshType>::computeLocalFirstPiolaKirchhoffTensor ( Epetra_SerialDenseMatrix& firstPiola,
                                                                                     const Epetra_SerialDenseMatrix& tensorF,
                                                                                     const Epetra_SerialDenseMatrix& cofactorF,
                                                                                     const std::vector<Real>& invariants,
                                                                                     const UInt marker)
{
    // It can be done using the evaluateNodal framework that has been shown to work
    // for the isotropic laws!
}

template <typename MeshType>
void DistributedHolzapfelMaterialNonLinear<MeshType>::computeCauchyStressTensor ( const vectorPtr_Type disp,
										  const QuadratureRule& evalQuad,
										  vectorPtr_Type sigma_1,
										  vectorPtr_Type sigma_2,
										  vectorPtr_Type sigma_3)
{

  ASSERT( 2 < 0 , "For the distributed anisotropic law the computation of the Cauchy stress has to be defined." );

}


template <typename MeshType>
inline StructuralAnisotropicConstitutiveLaw<MeshType>* createDistributedHolzapfelMaterialNonLinear()
{
    return new DistributedHolzapfelMaterialNonLinear<MeshType >();
}

namespace
{
static bool registerDHL = StructuralAnisotropicConstitutiveLaw<LifeV::RegionMesh<LinearTetra> >::StructureAnisotropicMaterialFactory::instance().registerProduct ( "distributedHolzapfel", &createDistributedHolzapfelMaterialNonLinear<LifeV::RegionMesh<LinearTetra> > );
}

} //Namespace LifeV

#endif /* __EXPONENTIALMATERIAL_H */
