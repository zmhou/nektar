///////////////////////////////////////////////////////////////////////////////
//
// File EquationSystem.h
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
// Description: Base class for individual solvers.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_SOLVERS_ADRSOLVER_EQUATIONSYSTEM_H
#define NEKTAR_SOLVERS_ADRSOLVER_EQUATIONSYSTEM_H

#include <LibUtilities/BasicUtils/NekFactory.hpp>
#include <ADRSolver/SessionReader.h>
#include <Auxiliary/ADRBase.h>
#include <LibUtilities/BasicUtils/SharedArray.hpp>
#include <SpatialDomains/SpatialData.h>
#include <MultiRegions/ExpList.h>

namespace Nektar
{
    class EquationSystem;

    /// A shared pointer to an EquationSystem object
    typedef boost::shared_ptr<EquationSystem> EquationSystemSharedPtr;
    /// Datatype of the NekFactory used to instantiate classes derived from
    /// the EquationSystem class.
    typedef LibUtilities::NekFactory< std::string, EquationSystem,
        SessionReaderSharedPtr&> EquationSystemFactory;

    /// A base class for describing how to solve specific equations.
    class EquationSystem : public ADRBase
    {
    public:
        /// Destructor
        virtual ~EquationSystem();

        /// Perform any initialisation necessary before solving the problem.
        void DoInitialise();

        /// Solve the problem.
        void DoSolve();
		
        /// Print a summary of parameters and solver characteristics.
        void PrintSummary(std::ostream &out);

    protected:
        /// The session reader
        SessionReaderSharedPtr                  m_session;

        /// Initialises EquationSystem class members.
        EquationSystem(SessionReaderSharedPtr& pSession);

        /// Evaluates a function as specified in the session file.
        void EvaluateFunction(Array<OneD, NekDouble>& pArray,
                SpatialDomains::ConstUserDefinedEqnShPtr pEqn);

        /// Evaluates the boundary conditions at the given time.
        void SetBoundaryConditions(NekDouble time);

        /// Virtual function for initialisation implementation.
        virtual void v_DoInitialise();

        /// Virtual function for solve implementation.
        virtual void v_DoSolve();

        /// Virtual function for printing summary information.
        virtual void v_PrintSummary(std::ostream &out);
		
    private:

    };
}

#endif
