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
 @file
 @brief Navier-Stokes operator equations.
 
 @author Davide Forti <davide.forti@epfl.ch>
 @contributor Umberto Villa
 @date 08-12-2014
 
 @maintainer Davide Forti <davide.forti@epfl.ch>
 */

#ifndef NSOPERATOR_HPP_
#define NSOPERATOR_HPP_

#include <Epetra_Import.h>
#include <boost/numeric/ublas/matrix.hpp>

#include <lifev/core/linear_algebra/BlockEpetra_Map.hpp>
#include <lifev/core/linear_algebra/BlockEpetra_MultiVector.hpp>
#include <lifev/core/linear_algebra/LinearOperatorAlgebra.hpp>

namespace LifeV
{
    
namespace Operators
{
//! @class NavierStokesOperator
/*! @brief A abstract class for handling n-by-m block operators
 * This class inherits from LifeV::LinearOperator.
 *
 * The Transpose is not supported yet.
 */

class NavierStokesOperator: public LinearOperatorAlgebra
{
public:
    
    //! @name Public Typedefs
    //@{
    typedef LinearOperatorAlgebra super;
    typedef super::comm_Type comm_Type;
    typedef super::commPtr_Type commPtr_Type;
    typedef super::map_Type map_Type;
    typedef super::mapPtr_Type mapPtr_Type;
    typedef super::operator_Type operator_Type;
    typedef super::operatorPtr_Type operatorPtr_Type;
    typedef super::vector_Type vector_Type;
    typedef super::vectorPtr_Type vectorPtr_Type;
    
    typedef boost::numeric::ublas::matrix<operatorPtr_Type> operatorPtrContainer_Type;
    typedef std::vector<vectorPtr_Type> vectorPtrContainer_Type;
    typedef std::vector<mapPtr_Type > mapPtrContainer_Type;

    //@}
    
    //! Empty Constructor
    NavierStokesOperator();
    
    //! @name Set Methods
    //@{
    //! SetUp for a "square operator"
    /*!
     * @param domainMap: the map of a vector in the domain of \e this
     *                   is obtained by concatenating the block maps in
     *                   domainMap.
     *                   rangeMap is assumed to be the same of domainMap.
     * @param comm:      the communicator.
     */
    void setUp (const std::shared_ptr<BlockEpetra_Map> & map, const commPtr_Type & comm);
    
    //! SetUp for a "rectangular operator"
    /*!
     * @param domainMap: the map of a vector in the domain of \e this
     *                   is obtained by concatenating the block maps in
     *                   domainMap.
     * @param rangeMap:  the map of a vector in the range of \e this
     *                   is obtained by concatenating the block maps in
     *                   rangeMap.
     * @param comm:      the communicator.
     */
    void setUp (const std::shared_ptr<BlockEpetra_Map> & domainMap,
                const std::shared_ptr<BlockEpetra_Map> & rangeMap,
                const commPtr_Type & comm);
    
    //! SetUp when the operator is given like a std::matrix
    /*!
     * @param blockOper: a dense matrix to describe the block operator
     * @param comm:      the communicator
     */
    void setUp (const operatorPtrContainer_Type & blockOper, const commPtr_Type & comm);
    
    //! set a component of the block operator
    /*!
     * @param iblock, jblock: The position of the block is (iblock, jblock).
     * @param operBlock     : an operator_ptr representing the block
     */
    void setBlock (UInt iblock, UInt jblock, const operatorPtr_Type & operBlock);
    
    //! Complete the block matrix with null operators
    void fillComplete ();
    
    //! If true the transpose of the operator will be computed.
    /*
     * Not Supported yet
     */
    int SetUseTranspose(bool useTranspose);
    
    //! Compute Y = Op*X;
    virtual int Apply(const vector_Type & X, vector_Type & Y) const;
    //! Compute Y = Op\X;
    /*!
     * ApplyInverse is implemented for the matrices with block diagonal, lowerTriangular, upperTriangular form
     */
    virtual int ApplyInverse(const vector_Type & X, vector_Type & Y) const;
    
    //! Compute the Inf norm of the operator
    /*!
     * Not implemented yet.
     */
    double NormInf() const {return -1;}
    
    //! Returns a character string describing the operator
    virtual const char * Label() const {return M_name.c_str();}
    
    //! Returns the current UseTranspose setting.
    bool UseTranspose() const {return M_useTranspose;}
    
    //! Returns true if the \e this object can provide an approximate Inf-norm, false otherwise.
    bool HasNormInf() const {return false;}
    
    //! Returns a pointer to the Epetra_Comm communicator associated with this operator.
    const comm_Type & Comm() const {return *M_comm;}
    
    //! Returns a const pointer to the (i,j) block
    const operatorPtr_Type& block (UInt iblock, UInt jblock) const;
    
    //! Returns the Epetra_Map object associated with the domain of this operator.
    const map_Type & OperatorDomainMap() const {return *(M_domainMap->monolithicMap());}
    //! Returns the Epetra_Map object associated with the domain of this operator as a pointer
    const mapPtr_Type & OperatorDomainMap_ptr() const {return M_domainMap->monolithicMap();}
    const std::shared_ptr<BlockEpetra_Map> & OperatorDomainBlockMapPtr() const {return M_domainMap;}
    
    //! Returns the Epetra_Map object associated with the range of this operator.
    const map_Type & OperatorRangeMap() const {return *(M_rangeMap->monolithicMap());}
    //! Returns the Epetra_Map object associated with the range of this operator as a pointer
    const mapPtr_Type & OperatorRangeMap_ptr() const {return M_rangeMap->monolithicMap();}
    const std::shared_ptr<BlockEpetra_Map> & OperatorRangeBlockMapPtr() const {return M_rangeMap;}
    
protected:
    //! Compute Y = Op*X;
    int applyNoTranspose(const vector_Type & X, vector_Type & Y) const;
    //! Compute Y = Op'*X;
    int applyTranspose(const vector_Type & X, vector_Type & Y) const;
    //! Y = diag(block(i,i)^-1)*X
    int blockJacobi(const vector_Type & X, vector_Type & Y) const;
    //! Y = backwardsubstitution(X)
    int blockUpperTriangularSolve(const vector_Type & X, vector_Type & Y) const;
    //! Y = forwardsubstitution(X)
    int blockLowerTriangularSolve(const vector_Type & X, vector_Type & Y) const;
    //! Change the name of the operator, (available for derivate classes).
    void setName(const std::string & name){M_name =name;}
private:
    
    //! Number of blocks in each row
    UInt M_nBlockRows;
    //! Number of blocks in each column
    UInt M_nBlockCols;
    
    //! Name of the object
    std::string M_name;
    //! Communicator
    commPtr_Type M_comm;
    
    //! @name Maps
    //@{
    //! Domain Map
    std::shared_ptr<BlockEpetra_Map> M_domainMap;
    //! Range Map
    std::shared_ptr<BlockEpetra_Map> M_rangeMap;
    //@}
    
    //! block operator represented like a dense matrix of pointers to Operators
    operatorPtrContainer_Type M_oper;
    
    //! whenever transpose should be used
    bool M_useTranspose;
};

} /*end namespace Operators*/
    
} /*end namespace */
#endif /* BLOCKOPERATOR_HPP_ */
