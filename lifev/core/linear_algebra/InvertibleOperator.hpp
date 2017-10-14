/*
 * InvertibleOperator.hpp
 *
 *  Created on: Sep 3, 2010
 *      Author: uvilla
 */

#ifndef INVERTIBLEOPERATOR_HPP_
#define INVERTIBLEOPERATOR_HPP_

#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCPDecl.hpp>

#include <lifev/core/linear_algebra/LinearOperatorAlgebra.hpp>
#include <lifev/core/util/FactorySingleton.hpp>
#include <lifev/core/util/Factory.hpp>
#include <lifev/core/array/VectorEpetra.hpp>

namespace LifeV
{
namespace Operators
{
//! @class InvertibleOperator
/*! @brief Abstract class which defines the interface of an Invertible Linear Operator Algebra.
 *
 */
class InvertibleOperator : public LinearOperatorAlgebra
{
public:

    InvertibleOperator();

    //! @name Attribute set methods
    //@{

    //! If set true, transpose of this operator will be applied.
    virtual int SetUseTranspose(bool useTranspose);

    void setOperator(const operatorPtr_Type & _oper);

    void setPreconditioner(const operatorPtr_Type & _prec);

    void setParameterList(const Teuchos::ParameterList & _pList);

    //@}

    //! @name Mathematical functions
    //@{

    //! Returns the result of a Epetra_Operator applied to a vector_Type X in Y.
    virtual int Apply(const vector_Type& X, vector_Type& Y) const;

    //! Returns the result of a Epetra_Operator inverse applied to an vector_Type X in Y.
    virtual int ApplyInverse(const vector_Type& X, vector_Type& Y) const;

    int NumIter() const {return M_numIterations;}
    
    double TimeSolver() const {return M_solutionTime;}

    //! Returns the infinity norm of the global matrix.
    double NormInf() const {return M_oper->NormInf();}

    //@}

    //! @name Attribute access functions
    //@{

    //! Returns a character string describing the operator
    virtual const char * Label() const {return M_name.c_str();}

    //! Returns the current UseTranspose setting.
    virtual bool UseTranspose() const {return M_useTranspose;}

    //! Returns true if the \e this object can provide an approximate Inf-norm, false otherwise.
    virtual bool HasNormInf() const {return M_oper->HasNormInf();}

    //! Returns a pointer to the Epetra_Comm communicator associated with this operator.
    virtual const comm_Type & Comm() const {return M_oper->Comm();}

    //! Returns the Epetra_Map object associated with the domain of this operator.
    virtual const map_Type & OperatorDomainMap() const {return M_oper->OperatorDomainMap();}

    //! Returns the Epetra_Map object associated with the range of this operator.
    virtual const map_Type & OperatorRangeMap() const {return M_oper->OperatorRangeMap();}

    //@}
protected:

    virtual int doApplyInverse(const vector_Type& X, vector_Type& Y) const = 0;
    virtual void doSetOperator() = 0;
    virtual void doSetPreconditioner() = 0;
    virtual void doSetParameterList() = 0;

    //! The name of the Operator
    std::string M_name;
    //! The list of Parameter to feed the linear solver
    Teuchos::RCP<Teuchos::ParameterList> M_pList;

    //! The preconditioner operator (the boost copy makes sure that the preconditioner is still alive; see .cpp file for details)
    Teuchos::RCP<Epetra_Operator> M_prec;
    std::shared_ptr<Epetra_Operator> M_precBoost;

    //! The operator to be solved (the boost copy makes sure that the operator is still alive; see .cpp file for details)
    Teuchos::RCP<Epetra_Operator> M_oper;
    std::shared_ptr<Epetra_Operator> M_operBoost;
    //! Whenever to use the transpose
    bool M_useTranspose;
    
    //! Number of iterations performed by the solver
    mutable int M_numIterations;

    //! Time spent to solve the linear system
    mutable double M_solutionTime;
};

typedef FactorySingleton<Factory<InvertibleOperator, std::string> > InvertibleOperatorFactory;

} // Namespace Operators

} // Namespace LifeV

#endif // INVERTIBLEOPERATOR_HPP_
