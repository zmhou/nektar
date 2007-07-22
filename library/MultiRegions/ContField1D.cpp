///////////////////////////////////////////////////////////////////////////////
//
// File ContField1D.cpp
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
// Description: Field definition for 1D domain with boundary conditions
//
///////////////////////////////////////////////////////////////////////////////

#include <MultiRegions/ContField1D.h>

namespace Nektar
{
    namespace MultiRegions
    {
	
	ContField1D::ContField1D(void)
	{
	}

        ContField1D::ContField1D(const ContField1D &In):
            ContExpList1D(In)
        {
        }

        ContField1D::ContField1D(const LibUtilities::BasisKey &Ba, 
                                 const SpatialDomains::Composite &cmps,
                                 SpatialDomains::BoundaryConditions &bcs):
            ContExpList1D(Ba,cmps)
        {
	    int i,nbnd;
            LocalRegions::PointExpSharedPtr  p_exp;
            int NumDirichlet = 0;

            SpatialDomains::BoundaryRegionCollectionType    &bregions = bcs.GetBoundaryRegions();
            SpatialDomains::BoundaryConditionCollectionType &bconditions = bcs.GetBoundaryConditions();

            nbnd = bregions.size();

            // list Dirichlet boundaries first
            for(i = 0; i < nbnd; ++i)
            {
                if(  ((*(bconditions[i]))["u"])->GetBoundaryConditionType() == SpatialDomains::eDirichlet)
                {
                    SpatialDomains::VertexComponentSharedPtr vert;
                    
                    if(vert = boost::dynamic_pointer_cast<SpatialDomains::VertexComponent>((*(*bregions[i])[0])[0]))
                    {
                        p_exp = MemoryManager<LocalRegions::PointExp>::AllocateSharedPtr(vert);
                        m_bndConstraint.push_back(p_exp);
                        m_bndTypes.push_back(SpatialDomains::eDirichlet);
                        NumDirichlet++;
                    }
                    else
                    {
                        ASSERTL0(false,"dynamic cast to a vertex failed");
                    }
                }
            }

            for(i = 0; i < nbnd; ++i)
            {
                if( ((*(bconditions[i]))["u"])->GetBoundaryConditionType() != SpatialDomains::eDirichlet)
                {
                   SpatialDomains:: VertexComponentSharedPtr vert;

                    if(vert = dynamic_pointer_cast<SpatialDomains::VertexComponent>((*(*bregions[i])[0])[0]))
                    {
                        p_exp = MemoryManager<LocalRegions::PointExp>::AllocateSharedPtr(vert);
                        m_bndConstraint.push_back(p_exp);
                        m_bndTypes.push_back(((*(bconditions[i]))["u"])->GetBoundaryConditionType());
                    }
                    else
                    {
                        ASSERTL0(false,"dynamic cast to a vertex failed");
                    }
                }
            }
            

            // Need to reset numbering according to Dirichlet Bondary Condition
            m_locToGloMap->ResetMapping(NumDirichlet,bcs);
            // Need to know how to get global vertex id 
            // I note that boundary Regions is a composite vector and so belive we can use this to get the desired quantities. 

        }

        ContField1D::~ContField1D()
	{
	}

	void ContField1D::FwdTrans(const ExpList &In)
	{
            GlobalLinSysKey key(StdRegions::eMass);
            GlobalSolve(key,In);

	    m_transState = eContinuous;
	    m_physState = false;
	}

        // Solve the helmholtz problem assuming that m_contCoeff vector 
        // contains an intial estimate for solution
	void ContField1D::HelmSolve(const ExpList &In, NekDouble lambda)
        {
            GlobalLinSysKey key(StdRegions::eHelmholtz,lambda);
            GlobalSolve(key,In);
        }


	void ContField1D::GlobalSolve(const GlobalLinSysKey &key, const ExpList &Rhs)
	{
            int i;
            int NumDirBcs = m_locToGloMap->GetNumDirichletBCs();
            Array<OneD,NekDouble> sln;
            Array<OneD,NekDouble> init    = Array<OneD,NekDouble>(m_contNcoeffs);
            Array<OneD,NekDouble> Dir_fce = Array<OneD,NekDouble>(m_contNcoeffs);
            //assume m_contCoeffs contains initial estimate
            // Set BCs in m_contCoeffs
            Blas::Dcopy(m_contNcoeffs,&m_contCoeffs[0],1,&init[0],1);
            for(i = 0; i < NumDirBcs; ++i)
            {
                init[i] = m_bndConstraint[i]->GetValue();
            }

            GeneralMatrixOp(key.GetLinSysType(), init,Dir_fce,
                            key.GetScaleFactor());

            // Set up forcing function
	    IProductWRTBase(Rhs);
            Vmath::Neg(m_contNcoeffs,&m_contCoeffs[0],1);

            // Forcing function with Dirichlet conditions 
            Vmath::Vsub(m_contNcoeffs,&m_contCoeffs[0],1,
                        &Dir_fce[0],1,&m_contCoeffs[0],1);

            if(m_contNcoeffs - NumDirBcs > 0)
            {
                GlobalLinSysSharedPtr LinSys = GetGlobalLinSys(key);

                sln = m_contCoeffs+NumDirBcs;
                DNekVec v(m_contNcoeffs-NumDirBcs,sln,eWrapper);
                LinSys->GetLinSys()->Solve(v,v);
            }

            // Recover solution by addinig intial conditons
            Vmath::Zero(NumDirBcs,&m_contCoeffs[0],1);
            Vmath::Vadd(m_contNcoeffs,&init[0],1,&m_contCoeffs[0],1,
                        &m_contCoeffs[0],1);

	    m_transState = eContinuous;
	    m_physState = false;
	}


        // could replace with a map
        GlobalLinSysSharedPtr ContField1D::GetGlobalLinSys(const GlobalLinSysKey &mkey) 
       {
            switch (mkey.GetLinSysType())
            {
            case StdRegions::eMass:
                if(!m_mass.get())
                {
                    m_mass = GenGlobalLinSys(mkey,
                             m_locToGloMap->GetNumDirichletBCs());
                }
                return m_mass;
                break;
            case StdRegions::eHelmholtz:
                if(!m_helm.get())
                {
                    m_helm = GenGlobalLinSys(mkey,
                             m_locToGloMap->GetNumDirichletBCs());
                }
                return m_helm;
                break;
            default:
                ASSERTL0(false,"This matrix type is not set up");
                    break;
            }
        }

    } // end of namespace
} //end of namespace
