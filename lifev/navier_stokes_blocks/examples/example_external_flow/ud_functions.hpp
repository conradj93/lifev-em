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
 *  @brief File containing the functions for BCs
 *
 *  @author Davide Forti <davide.forti@epfl.ch>
 *
 */

#ifndef UDFNS_HPP
#define UDFNS_HPP

// LifeV includes
#include <lifev/core/LifeV.hpp>

namespace LifeV
{

Real zeroFunction(const Real& /*t*/, const Real& /*x*/, const Real& /*y*/, const Real& /*z*/, const ID& /*i*/)
{
    return 0.0;
}

Real inflowFunction(const Real& t, const Real& /*x*/, const Real& /*y*/, const Real& /*z*/, const ID& i)
{
    if (i == 0)
    {
        Real ux = 22.0;
        Real T_ramp = 0.3;
        Real inflowVel;

        if ( t <= T_ramp )
        {
        	inflowVel =	ux/2.0*(1.0-std::cos(M_PI/T_ramp*t));
        }
        else
        {
        	inflowVel = ux;
        }
        return inflowVel;
    }
    else
    {
        return 0;
    }

}

Real oneFunctionX(const Real& /*t*/, const Real& /*x*/, const Real& /*y*/, const Real& /*z*/, const ID& i)
{
	if(i==0)
	{
		return 1.0;
	}
	else
	{
		return 0.0;
	}
}

Real oneFunctionY(const Real& /*t*/, const Real& /*x*/, const Real& /*y*/, const Real& /*z*/, const ID& i)
{
	if(i==1)
	{
		return 1.0;
	}
	else
	{
		return 0.0;
	}
}

}

#endif
