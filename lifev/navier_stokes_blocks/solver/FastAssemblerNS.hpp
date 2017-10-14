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
     @brief This file contains the definition of the FastAssembler class.

     This function is used to perform an efficient assembly of FE matrices
     where the test and trial functions are the same. This class is experimental
     and must be used with a lot of care.

     \warning All the routines contained here are supposted to be used/expanded by experts of LifeV
     since they contain a much faster way of assembling matrices stemming from the discretized
     NS equations (either in the fully-implicit or semi-implicit case).

     Preliminary tests
     showed that the assemblers coded here are roughly 20 times faster then
     ETA based ones. The assembly is done "by hands" and the code is quite
     hard to be understood.

     To use this class one has first to allocate the memory required and then
     just call the routines required for the assembly.

     @date 06/2016
     @author Davide Forti <davide.forti@epfl.ch>
 */

#ifndef FASTASSEMBLERNS_HPP
#define FASTASSEMBLERNS_HPP

#include <lifev/core/LifeV.hpp>
#include <lifev/core/mesh/RegionMesh.hpp>
#include <lifev/core/array/VectorEpetra.hpp>
#include <lifev/core/array/MatrixEpetra.hpp>
#include <lifev/core/fem/QuadratureRule.hpp>
#include <lifev/core/fem/ReferenceFE.hpp>
#include <lifev/core/fem/CurrentFE.hpp>
#include <lifev/core/fem/FESpace.hpp>

namespace LifeV
{

class FastAssemblerNS
{
public:

	typedef RegionMesh< LinearTetra > mesh_Type;
    typedef std::shared_ptr<mesh_Type>  meshPtr_Type;

    typedef VectorEpetra vector_Type;
    typedef std::shared_ptr<vector_Type> vectorPtr_Type;

    typedef MatrixEpetra<Real> matrix_Type;
    typedef std::shared_ptr<matrix_Type> matrixPtr_Type;

    typedef Epetra_Comm comm_Type;
    typedef std::shared_ptr< comm_Type > commPtr_Type;

    typedef QuadratureRule qr_Type;
    typedef std::shared_ptr< qr_Type > qrPtr_Type;

    typedef FESpace<mesh_Type, MapEpetra> fespace_Type;
    typedef std::shared_ptr<fespace_Type> fespacePtr_Type;


    //! Constructor
    /*!
     * @param mesh - input mesh
     * @param comm - communicator
     * @param refFE_velocity - reference FE space velocity
     * @param refFE_pressure - reference FE space pressure
     * @param fespace_velocity -
     * @param fespace_pressure -
     * @param qr - quadrature rule to be used for the integration
     */
    FastAssemblerNS( const meshPtr_Type& mesh, const commPtr_Type& comm,
    		         const ReferenceFE* refFE_velocity, const ReferenceFE* refFE_pressure,
    		         const fespacePtr_Type& fespace_velocity, const fespacePtr_Type& fespace_pressure,
    		         const qr_Type* qr );

    //! Destructor
	~FastAssemblerNS();

	//! @name Methods
	//@{

	//! Set physical parameters for NS
	/*!
	 * @param density - density of the fluid
	 * @param viscosity - viscosity of the fluid
	 * @param timestep - timestep for the simulation
	 * @param orderBDF - order time integrator BDF
	 * @param C_I - is 30 for P1 and 60 for P2
	 */
	void setConstants_NavierStokes( const Real& density, const Real& viscosity, const Real& timestep, const Real& orderBDF, const Real& C_I );
    
    //! Set physical parameters for NS
    /*!
     * @param density - density of the fluid
     * @param viscosity - viscosity of the fluid
     * @param timestep - timestep for the simulation
     * @param orderBDF - order time integrator BDF
     * @param C_I - is 30 for P1 and 60 for P2
     * @param alpha - coefficient BDF in front of u_{n+1}
     */
    void setConstants_NavierStokes( const Real& density, const Real& viscosity, const Real& timestep, const Real& orderBDF, const Real& C_I, const Real& alpha );

	//! Allocate space for members before the assembly
	/*!
	 * @param current_fe_velocity - current FE space velocity
	 */
	void allocateSpace( CurrentFE* current_fe_velocity, const bool& use_supg );

    //! Assemble constant terms NS
    /*!
     * @param mass - mass matrix
     * @param stiffness - stiffness matrix
     * @param grad - block01
     * @param div - block10
     */
    void assemble_constant_terms( matrixPtr_Type& mass, matrixPtr_Type& stiffness, matrixPtr_Type& grad, matrixPtr_Type& div );
    
    //! FE Assembly of NS constant terms (no scaling by coefficients like viscosity)
    /*!
     * @param matrix - global matrix
     * @param u_h - velocity vector
     */
    void assembleConvective( matrixPtr_Type& matrix, const vector_Type& u_h );
    
    //! FE Assembly of NS nonlinear term
    /*!
     * @param matrix - matrix
     * @param u_h - velocity vector previous Newton step
     */
    void jacobianNS( matrixPtr_Type& matrix, const vector_Type& u_h );
    
	//! Assemble SUPG terms
	/*!
	 * @param block00 - block00 stabilization
	 * @param block01 - block01 stabilization
	 * @param block10 - block10 stabilization
	 * @param block11 - block11 stabilization
	 */
	void assemble_supg_terms( matrixPtr_Type& block00, matrixPtr_Type& block01, matrixPtr_Type& block10, matrixPtr_Type& block11, const vector_Type& u_h  );
    
    //! Assemble SUPG terms fully implicit for FSI
    /*!
     * @param block00 - block00 stabilization
     * @param block01 - block01 stabilization
     * @param block10 - block10 stabilization
     * @param block11 - block11 stabilization
     * @param beta_km1 - vector: fluid_velocity - ale_velocity
     * @param u_km1 - velocity previous Newton step
     * @param p_km1 - pressure previous Newton step
     * @param u_bdf - vector time discretization fluid velocity
     */
    void supg_FI_FSI_terms ( matrixPtr_Type& block00,
                             matrixPtr_Type& block01,
                             matrixPtr_Type& block10,
                             matrixPtr_Type& block11,
                             const vector_Type& beta_km1,
                             const vector_Type& u_km1,
                             const vector_Type& p_km1,
                             const vector_Type& u_bdf);
    
    //! Assemble VMSLES terms
    /*!
     * @param block00 - block00 stabilization
     * @param block01 - block01 stabilization
     * @param block10 - block10 stabilization
     * @param block11 - block11 stabilization
     * @param fine_scale - fine scale vector
     * @param u_extr - velocity extrapolated
     */
    void vmsles_semi_implicit_terms ( matrixPtr_Type& block00,
                                      matrixPtr_Type& block01,
                                      matrixPtr_Type& block10,
                                      matrixPtr_Type& block11,
                                      const std::vector<std::vector<VectorSmall<3>>>& fine_scale,
                                      const vector_Type& u_extr);
    
    //! Assemble VMSLES terms when using P1-P1 for the fluid
    /*!
     * @param block00 - block00 stabilization
     * @param block01 - block01 stabilization
     * @param block10 - block10 stabilization
     * @param block11 - block11 stabilization
     * @param fine_scale - fine scale vector
     * @param u_extr - velocity extrapolated
     */
    void vmsles_semi_implicit_termsP1P1 ( matrixPtr_Type& block00,
    									  matrixPtr_Type& block01,
    									  matrixPtr_Type& block10,
    									  matrixPtr_Type& block11,
    									  const std::vector<std::vector<VectorSmall<3>>>& fine_scale,
    									  const vector_Type& u_extr);

    //! Assemble SUPG terms fully implicit for FSI
    /*!
     * @param current_fe_velocity - current FE needed to update geom quantities (for FSI when moving mesh)
     */

    void updateGeoQuantities ( CurrentFE* current_fe );

    //! Set alpha
    /*!
     * @param alpha - time advance coefficient
     */

    void setAlpha ( const Real& alpha ) { M_alpha = alpha; };

    //! Set time step
    /*!
     * @param timestep - time step of the simulation
     */

    void setTimeStep ( const Real& timestep ) { M_timestep = timestep; };

	//@}

private:

	meshPtr_Type M_mesh;
	commPtr_Type M_comm;

	int M_numElements;
	int M_numScalarDofs;

	double * M_detJacobian;
	double *** M_invJacobian;

	double ** M_phi_velocity;
	double ** M_phi_pressure;
	double *** M_dphi_velocity;
	double *** M_dphi_pressure;
	double **** M_d2phi_velocity;
    double ** M_elements_velocity;
    double ** M_elements_pressure;

	const qr_Type* M_qr;
	const ReferenceFE* M_referenceFE_velocity;
	const ReferenceFE* M_referenceFE_pressure;
	const std::shared_ptr<FESpace<mesh_Type, MapEpetra>> M_fespace_velocity;
	const std::shared_ptr<FESpace<mesh_Type, MapEpetra>> M_fespace_pressure;

	double*** M_vals_00;
	double*** M_vals_01;
	double*** M_vals_10;
	double*** M_vals_11;
	double***** M_vals_supg;
	double**** M_vals_supg_01;
	double**** M_vals_supg_10;
	int** M_rows_velocity;
	int** M_rows_pressure;
	int** M_cols_velocity;
	int** M_cols_pressure;
    int** M_rows_tmp;
    int** M_cols_tmp;

    bool M_useSUPG;
    double *** M_G; // metric tensor
    double ** M_g; // metric vector
    double ** M_Tau_M; // coefficient Tau_M
    double ** M_Tau_C; // coefficient Tau_C
    double ** M_Tau_M_hat; // coefficient Tau_M_hat for VMSLES at quadrature
    
    double M_density;
    double M_viscosity;
    double M_timestep;
    double M_orderBDF;
    double M_C_I;
    double M_alpha;
};

} // Namespace LifeV

#endif // FASTASSEMBLERNS_HPP
