///////////////////////////////////////////////////////////////////////////////
//
// File Points1D.cpp
//
// For more information, please see: http://www.nektar.info
//
// The MIT License
//
// Copyright (c) 2006 Division of Applied Mathematics, Brown University (USA),
// Department of Aeronautics, Imperial College London (UK), and Scientific
// Computing and Imaging Institute, University of Utah (USA).
//
// License for the specific language governing rights and limitations under
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// 
// Description: C functions to provide access to managers. 
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <LibUtilities/Foundations/Points.h>
#include <LibUtilities/Foundations/Basis.h>
#include <LibUtilities/Foundations/Foundations.hpp>
#include <LibUtilities/BasicUtils/ErrorUtil.hpp>

#include <LibUtilities/Foundations/ManagerAccess.h>

namespace Nektar
{
    namespace LibUtilities 
    {
        PointsManagerT &PointsManager(void)
        {
            return Loki::SingletonHolder<PointsManagerT>::Instance();
        }

        BasisManagerT &BasisManager(void)
        {
            return Loki::SingletonHolder<BasisManagerT>::Instance();
        }

    } // end of namespace LibUtilities
} // end of namespace Nektar


/**
$Log: ManagerAccess.cpp,v $
Revision 1.1  2007/01/19 18:02:26  jfrazier
Initial checkin.

**/



