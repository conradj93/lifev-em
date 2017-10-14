/* -*- mode: c++ -*-
 
 This file is part of the LifeV library.
 Copyright (C) 2010 EPFL
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 USA
 */
/**
 \file main.cpp
 \author Davide Forti <davide.forti@epfl.ch>
 \date 2014-02-06
 */


#include <Epetra_ConfigDefs.h>
#ifdef EPETRA_MPI
#include <mpi.h>
#include <Epetra_MpiComm.h>
#else
#include <Epetra_SerialComm.h>
#endif


#include <lifev/core/LifeV.hpp>

#include <lifev/core/algorithm/PreconditionerIfpack.hpp>
#include <lifev/core/algorithm/PreconditionerML.hpp>

#include "navier_stokes.hpp"

using namespace LifeV;

int
main ( int argc, char** argv )
{
    
    bool verbose (false);
#ifdef HAVE_MPI
    MPI_Init (&argc, &argv);
    boost::shared_ptr<Epetra_Comm> Comm (new Epetra_MpiComm (MPI_COMM_WORLD) );
    if ( Comm->MyPID() == 0 )
    {
        verbose = true;
    }
#else
    boost::shared_ptr<Epetra_Comm> Comm (new Epetra_SerialComm);
    verbose = true;
#endif
    
    NavierStokes ns ( argc, argv, Comm, "data", "result");
    
    ns.run();
    
#ifdef HAVE_MPI
    if (verbose)
    {
        std::cout << "MPI Finalization" << std::endl;
    }
    MPI_Finalize();
#endif
    return ( EXIT_SUCCESS );
}


