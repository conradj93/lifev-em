//@HEADER
/*
*******************************************************************************

   Copyright (C) 2004, 2005, 2007 EPFL, Politecnico di Milano, INRIA
   Copyright (C) 2010 EPFL, Politecnico di Milano, Emory UNiversity

   This file is part of the LifeV library

   LifeV is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   LifeV is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>


*******************************************************************************
*/
//@HEADER

/*!
 *   @file
     @brief This file contains the definition of the IntegrateVectorElement class.

     @date 06/2011
     @author Samuel Quinodoz <samuel.quinodoz@epfl.ch>
 */

#ifndef EVALUATENODALEXPRESSION_VECTOR_ELEMENT_HPP
#define EVALUATENODALEXPRESSION_VECTOR_ELEMENT_HPP

#include <lifev/core/LifeV.hpp>

#include <lifev/core/fem/QuadratureRule.hpp>
#include <lifev/eta/fem/ETCurrentFE.hpp>
#include <lifev/eta/fem/MeshGeometricMap.hpp>
#include <lifev/eta/fem/QRAdapterBase.hpp>

#include <lifev/eta/expression/ExpressionToEvaluation.hpp>

#include <lifev/eta/array/ETVectorElemental.hpp>



namespace LifeV
{

namespace ExpressionAssembly
{

//! The class to actually perform the loop over the elements to assemble a vector
/*!
  @author Samuel Quinodoz <samuel.quinodoz@epfl.ch>

  This class is used to store the data required for the assembly of a vector and
  perform that assembly with a loop over the elements, and then, for each elements,
  using the Evaluation corresponding to the Expression (this convertion is done
  within a typedef).
 */
template < typename MeshType, typename SolutionSpaceType, typename ExpressionType, typename QRAdapterType>
class EvaluateNodalExpressionVectorElement
{
public:

    //! @name Public Types
    //@{

    //! Type of the Evaluation
    typedef typename ExpressionToEvaluation < ExpressionType,
            SolutionSpaceType::field_dim,
            0,
            MeshType::S_geoDimensions >::evaluation_Type evaluation_Type;

    //@}


    //! @name Constructors, destructor
    //@{

    //! Full data constructor
    EvaluateNodalExpressionVectorElement (const std::shared_ptr<MeshType>& mesh,
                                          const QRAdapterType& qrAdapter,
                                          const std::shared_ptr<SolutionSpaceType>& testSpace,
                                          const ExpressionType& expression);

    //! Copy constructor
    EvaluateNodalExpressionVectorElement ( const EvaluateNodalExpressionVectorElement < MeshType, SolutionSpaceType, ExpressionType, QRAdapterType>& integrator);

    //! Destructor
    ~EvaluateNodalExpressionVectorElement();

    //@}


    //! @name Operator
    //@{

    //! Operator wrapping the addTo method
    template <typename VectorType>
    inline void operator>> (VectorType& vector)
    {
        addTo (vector);
    }

    //! Operator wrapping the addTo method (for shared_ptr)
    template <typename VectorType>
    inline void operator>> (std::shared_ptr<VectorType> vector)
    {
        addTo (*vector);
    }


    //@}

    //! @name Methods
    //@{

    //! Ouput method
    void check (std::ostream& out = std::cout);

    //! Method that performs the assembly
    /*!
      The loop over the elements is located right
      in this method. Everything for the assembly is then
      performed: update the values, update the local vector,
      sum over the quadrature nodes, assemble in the global
      vector.
     */
    template <typename VectorType>
    void addTo (VectorType& vec);

    //@}

private:

    //! @name Private Methods
    //@{

    // No default constructor
    EvaluateNodalExpressionVectorElement();

    //@}

    // Pointer on the mesh
    std::shared_ptr<MeshType> M_mesh;

    // Quadrature to be used
    QRAdapterType M_qrAdapter;

    // Shared pointer on the Space
    std::shared_ptr<SolutionSpaceType> M_solutionSpace;

    // Tree to compute the values for the assembly
    evaluation_Type M_evaluation;

    ETCurrentFE<MeshType::S_geoDimensions, 1>* M_globalCFE_std;
    ETCurrentFE<MeshType::S_geoDimensions, 1>* M_globalCFE_adapted;

    ETCurrentFE<SolutionSpaceType::space_dim, SolutionSpaceType::field_dim>* M_testCFE_std;
    ETCurrentFE<SolutionSpaceType::space_dim, SolutionSpaceType::field_dim>* M_testCFE_adapted;

    ETVectorElemental M_elementalVector;
};


// ===================================================
// IMPLEMENTATION
// ===================================================

// ===================================================
// Constructors & Destructor
// ===================================================

template < typename MeshType, typename SolutionSpaceType, typename ExpressionType, typename QRAdapterType>
EvaluateNodalExpressionVectorElement < MeshType, SolutionSpaceType, ExpressionType, QRAdapterType>::
EvaluateNodalExpressionVectorElement (const std::shared_ptr<MeshType>& mesh,
                        const QRAdapterType& qrAdapter,
                        const std::shared_ptr<SolutionSpaceType>& solutionSpace,
                        const ExpressionType& expression)
    :   M_mesh (mesh),
        M_qrAdapter (qrAdapter),
        M_solutionSpace (solutionSpace),
        M_evaluation (expression),

        M_testCFE_std (new ETCurrentFE<SolutionSpaceType::space_dim, SolutionSpaceType::field_dim> (solutionSpace->refFE(), solutionSpace->geoMap(), qrAdapter.standardQR() ) ),
        M_testCFE_adapted (new ETCurrentFE<SolutionSpaceType::space_dim, SolutionSpaceType::field_dim> (solutionSpace->refFE(), solutionSpace->geoMap(), qrAdapter.standardQR() ) ),

        M_elementalVector (SolutionSpaceType::field_dim * solutionSpace->refFE().nbDof() )

{
    switch (MeshType::geoShape_Type::BasRefSha::S_shape)
    {
        case LINE:
            M_globalCFE_std = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feSegP0, geometricMapFromMesh<MeshType>(), qrAdapter.standardQR() );
            M_globalCFE_adapted = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feSegP0, geometricMapFromMesh<MeshType>(), qrAdapter.standardQR() );
            break;
        case TRIANGLE:
            M_globalCFE_std = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feTriaP0, geometricMapFromMesh<MeshType>(), qrAdapter.standardQR());
            M_globalCFE_adapted = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feTriaP0, geometricMapFromMesh<MeshType>(), qrAdapter.standardQR());
            break;
        case QUAD:
            M_globalCFE_std = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feQuadQ0, geometricMapFromMesh<MeshType>(), qrAdapter.standardQR());
            M_globalCFE_adapted = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feQuadQ0, geometricMapFromMesh<MeshType>(), qrAdapter.standardQR());
            break;
        case TETRA:
            M_globalCFE_std = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feTetraP0, geometricMapFromMesh<MeshType>(), qrAdapter.standardQR());
            M_globalCFE_adapted = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feTetraP0, geometricMapFromMesh<MeshType>(), qrAdapter.standardQR());
            break;
        case HEXA:
            M_globalCFE_std = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feHexaQ0, geometricMapFromMesh<MeshType>(), qrAdapter.standardQR());
            M_globalCFE_adapted = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feHexaQ0, geometricMapFromMesh<MeshType>(), qrAdapter.standardQR());
            break;
        default:
            ERROR_MSG ("Unrecognized element shape");
    }

    M_evaluation.setQuadrature (qrAdapter.standardQR() );
    M_evaluation.setGlobalCFE (M_globalCFE_std);
    M_evaluation.setTestCFE (M_testCFE_std);
}


template < typename MeshType, typename SolutionSpaceType, typename ExpressionType, typename QRAdapterType>
EvaluateNodalExpressionVectorElement < MeshType, SolutionSpaceType, ExpressionType, QRAdapterType>::
EvaluateNodalExpressionVectorElement ( const EvaluateNodalExpressionVectorElement < MeshType, SolutionSpaceType, ExpressionType, QRAdapterType>& integrator)
    :   M_mesh (integrator.M_mesh),
        M_qrAdapter (integrator.M_qrAdapter),
        M_solutionSpace (integrator.M_solutionSpace),
        M_evaluation (integrator.M_evaluation),

        M_testCFE_std (new ETCurrentFE<SolutionSpaceType::space_dim, SolutionSpaceType::field_dim> (M_solutionSpace->refFE(), M_solutionSpace->geoMap(), integrator.M_qrAdapter.standardQR() ) ),
        M_testCFE_adapted (new ETCurrentFE<SolutionSpaceType::space_dim, SolutionSpaceType::field_dim> (M_solutionSpace->refFE(), M_solutionSpace->geoMap(), integrator.M_qrAdapter.standardQR() ) ),

        M_elementalVector (integrator.M_elementalVector)
{
    switch (MeshType::geoShape_Type::BasRefSha::S_shape)
    {
        case LINE:
            M_globalCFE_std = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feSegP0, geometricMapFromMesh<MeshType>(), integrator.M_qrAdapter.standardQR() );
            M_globalCFE_adapted = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feSegP0, geometricMapFromMesh<MeshType>(), integrator.M_qrAdapter.standardQR() );
            break;
        case TRIANGLE:
            M_globalCFE_std = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feTriaP0, geometricMapFromMesh<MeshType>(), integrator.M_qrAdapter.standardQR());
            M_globalCFE_adapted = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feTriaP0, geometricMapFromMesh<MeshType>(), integrator.M_qrAdapter.standardQR());
            break;
        case QUAD:
            M_globalCFE_std = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feQuadQ0, geometricMapFromMesh<MeshType>(), integrator.M_qrAdapter.standardQR());
            M_globalCFE_adapted = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feQuadQ0, geometricMapFromMesh<MeshType>(), integrator.M_qrAdapter.standardQR());
            break;
        case TETRA:
            M_globalCFE_std = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feTetraP0, geometricMapFromMesh<MeshType>(), integrator.M_qrAdapter.standardQR());
            M_globalCFE_adapted = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feTetraP0, geometricMapFromMesh<MeshType>(), integrator.M_qrAdapter.standardQR());
            break;
        case HEXA:
            M_globalCFE_std = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feHexaQ0, geometricMapFromMesh<MeshType>(), integrator.M_qrAdapter.standardQR());
            M_globalCFE_adapted = new ETCurrentFE<MeshType::S_geoDimensions, 1> (feHexaQ0, geometricMapFromMesh<MeshType>(), integrator.M_qrAdapter.standardQR());
            break;
        default:
            ERROR_MSG ("Unrecognized element shape");
    }
    M_evaluation.setQuadrature (integrator.M_qrAdapter.standardQR() );
    M_evaluation.setGlobalCFE (M_globalCFE_std);
    M_evaluation.setTestCFE (M_testCFE_std);
}


template < typename MeshType, typename SolutionSpaceType, typename ExpressionType, typename QRAdapterType>
EvaluateNodalExpressionVectorElement < MeshType, SolutionSpaceType, ExpressionType, QRAdapterType>::
~EvaluateNodalExpressionVectorElement()
{
    delete M_globalCFE_std;
    delete M_globalCFE_adapted;
    delete M_testCFE_std;
    delete M_testCFE_adapted;
}

// ===================================================
// Methods
// ===================================================

template < typename MeshType, typename SolutionSpaceType, typename ExpressionType, typename QRAdapterType>
void
EvaluateNodalExpressionVectorElement < MeshType, SolutionSpaceType, ExpressionType, QRAdapterType>::
check (std::ostream& out)
{
    out << " Checking the integration : " << std::endl;
    M_evaluation.display (out);
    out << std::endl;
    out << " Elemental vector : " << std::endl;
    M_elementalVector.showMe (out);
    out << std::endl;
}

template < typename MeshType, typename SolutionSpaceType, typename ExpressionType, typename QRAdapterType>
template <typename VectorType>
void
EvaluateNodalExpressionVectorElement < MeshType, SolutionSpaceType, ExpressionType, QRAdapterType>::
addTo (VectorType& vec)
{
    UInt nbElements (M_mesh->numElements() );
    UInt nbQuadPt_std (M_qrAdapter.standardQR().nbQuadPt() );
    UInt nbSolDof (M_solutionSpace->refFE().nbDof() );

    // Defaulted to true for security
    bool isPreviousAdapted (true);

    for (UInt iElement (0); iElement < nbElements; ++iElement)
    {
        // Zeros out the elemental vector
        M_elementalVector.zero();

        // Update the quadrature rule adapter
        M_qrAdapter.update (iElement);


        if (M_qrAdapter.isAdaptedElement() )
        {
            // Reset the quadrature in the different structures
            M_evaluation.setQuadrature ( M_qrAdapter.adaptedQR() );
            M_globalCFE_adapted -> setQuadratureRule ( M_qrAdapter.adaptedQR() );
            M_testCFE_adapted -> setQuadratureRule ( M_qrAdapter.adaptedQR() );

            // Reset the CurrentFEs in the evaluation
            M_evaluation.setGlobalCFE ( M_globalCFE_adapted );
            M_evaluation.setTestCFE ( M_testCFE_adapted );

            // Update with the correct element
            M_evaluation.update (iElement);

            // Update the currentFEs
            M_globalCFE_adapted->update (M_mesh->element (iElement), evaluation_Type::S_globalUpdateFlag | ET_UPDATE_WDET);
            M_testCFE_adapted->update (M_mesh->element (iElement), evaluation_Type::S_testUpdateFlag);


            // Assembly
            for (UInt iblock (0); iblock < SolutionSpaceType::field_dim; ++iblock)
            {
                // Set the row global indices in the local vector
                for (UInt i (0); i < nbSolDof; ++i)
                {
                    M_elementalVector.setRowIndex
                    (i + iblock * nbSolDof,
                     M_solutionSpace->dof().localToGlobalMap (iElement, i) + iblock * M_solutionSpace->dof().numTotalDof() );
                }

                // Make the assembly
                for (UInt iQuadPt (0); iQuadPt < M_qrAdapter.adaptedQR().nbQuadPt(); ++iQuadPt)
                {
                    for (UInt i (0); i < nbSolDof; ++i)
                    {
                        M_elementalVector.element (i + iblock * nbSolDof) +=
                            M_evaluation.value_qi (iQuadPt, i + iblock * nbSolDof);

                    }
                }
            }

            // Finally, set the flag
            isPreviousAdapted = true;
        }
        else
        {
            // Check if the last one was adapted
            if (isPreviousAdapted)
            {
                M_evaluation.setQuadrature ( M_qrAdapter.standardQR() );
                M_evaluation.setGlobalCFE ( M_globalCFE_std );
                M_evaluation.setTestCFE ( M_testCFE_std );

                isPreviousAdapted = false;
            }


            // Update the currentFEs
            M_globalCFE_std->update (M_mesh->element (iElement), evaluation_Type::S_globalUpdateFlag | ET_UPDATE_WDET);
            M_testCFE_std->update (M_mesh->element (iElement), evaluation_Type::S_testUpdateFlag );

            // Update the evaluation
            M_evaluation.update (iElement);

            // Loop on the blocks
            for (UInt iblock (0); iblock < SolutionSpaceType::field_dim; ++iblock)
            {
                // Set the row global indices in the local vector
                for (UInt i (0); i < nbSolDof; ++i)
                {
                    M_elementalVector.setRowIndex
                    (i + iblock * nbSolDof,
                     M_solutionSpace->dof().localToGlobalMap (iElement, i) + iblock * M_solutionSpace->dof().numTotalDof() );
                }

                // Make the assembly
                for (UInt iQuadPt (0); iQuadPt < nbQuadPt_std; ++iQuadPt)
                {
                    for (UInt i (0); i < nbSolDof; ++i)
                    {
                        M_elementalVector.element (i + iblock * nbSolDof) +=
                            M_evaluation.value_qi (iQuadPt, i + iblock * nbSolDof);

                    }
                }
            }

        }

        M_elementalVector.pushToGlobal (vec);
    }
}


} // Namespace ExpressionAssembly

} // Namespace LifeV

#endif
