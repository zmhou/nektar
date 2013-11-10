///////////////////////////////////////////////////////////////////////////////
//
// File TriExp.cpp
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
// Description: Expasion for triangular elements.
//
///////////////////////////////////////////////////////////////////////////////

#include <LocalRegions/TriExp.h>
#include <LocalRegions/SegExp.h>
#include <LocalRegions/Expansion3D.h>
#include <StdRegions/StdNodalTriExp.h>
#include <LibUtilities/Foundations/Interp.h>


namespace Nektar
{
    namespace LocalRegions
    {
        TriExp::TriExp(const LibUtilities::BasisKey &Ba,
                       const LibUtilities::BasisKey &Bb,
                       const SpatialDomains::TriGeomSharedPtr &geom):
            StdExpansion  (LibUtilities::StdTriData::getNumberOfCoefficients(Ba.GetNumModes(),(Bb.GetNumModes())),2,Ba,Bb),
            StdExpansion2D(LibUtilities::StdTriData::getNumberOfCoefficients(Ba.GetNumModes(),(Bb.GetNumModes())),Ba,Bb),
            StdTriExp(Ba,Bb),
            Expansion     (geom),
            Expansion2D   (geom),
            m_matrixManager(
                    boost::bind(&TriExp::CreateMatrix, this, _1),
                    std::string("TriExpMatrix")),
            m_staticCondMatrixManager(
                    boost::bind(&TriExp::CreateStaticCondMatrix, this, _1),
                    std::string("TriExpStaticCondMatrix"))
        {
        }


        TriExp::TriExp(const TriExp &T):
            StdExpansion(T),
            StdExpansion2D(T),
            StdTriExp(T),
            Expansion(T),
            Expansion2D(T),
            m_matrixManager(T.m_matrixManager),
            m_staticCondMatrixManager(T.m_staticCondMatrixManager)
        {
        }


        TriExp::~TriExp()
        {
        }



        NekDouble TriExp::v_Integral(const Array<OneD, const NekDouble> &inarray)
        {
            int    nquad0 = m_base[0]->GetNumPoints();
            int    nquad1 = m_base[1]->GetNumPoints();
            Array<OneD, const NekDouble> jac = m_metricinfo->GetJac();
            NekDouble ival;
            Array<OneD,NekDouble> tmp(nquad0*nquad1);

            // multiply inarray with Jacobian
            if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
            {
                Vmath::Vmul(nquad0*nquad1, jac, 1, inarray, 1,tmp, 1);
            }
            else
            {
                Vmath::Smul(nquad0*nquad1, jac[0], inarray, 1, tmp, 1);
            }

            // call StdQuadExp version;
            ival = StdTriExp::v_Integral(tmp);
            return ival;
        }
		

        void TriExp::v_PhysDeriv(const Array<OneD, const NekDouble> & inarray,
                               Array<OneD,NekDouble> &out_d0,
                               Array<OneD,NekDouble> &out_d1,
                               Array<OneD,NekDouble> &out_d2)
        {
            int    nquad0 = m_base[0]->GetNumPoints();
            int    nquad1 = m_base[1]->GetNumPoints();
            int     nqtot = nquad0*nquad1;
            const Array<TwoD, const NekDouble>& df
                                        = m_metricinfo->GetDerivFactors();

            Array<OneD,NekDouble> diff0(2*nqtot);
            Array<OneD,NekDouble> diff1(diff0+nqtot);

            StdTriExp::v_PhysDeriv(inarray, diff0, diff1);

            if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
            {
                if(out_d0.num_elements())
                {
                    Vmath::Vmul  (nqtot,df[0],1,diff0,1, out_d0, 1);
                    Vmath::Vvtvp (nqtot,df[1],1,diff1,1, out_d0, 1, out_d0,1);
                }

                if(out_d1.num_elements())
                {
                    Vmath::Vmul  (nqtot,df[2],1,diff0,1, out_d1, 1);
                    Vmath::Vvtvp (nqtot,df[3],1,diff1,1, out_d1, 1, out_d1,1);
                }

                if(out_d2.num_elements())
                {
                    Vmath::Vmul  (nqtot,df[4],1,diff0,1, out_d2, 1);
                    Vmath::Vvtvp (nqtot,df[5],1,diff1,1, out_d2, 1, out_d2,1);
                }
            }
            else // regular geometry
            {
                if(out_d0.num_elements())
                {
                    Vmath::Smul (nqtot, df[0][0], diff0, 1, out_d0, 1);
                    Blas::Daxpy (nqtot, df[1][0], diff1, 1, out_d0, 1);
                }

                if(out_d1.num_elements())
                {
                    Vmath::Smul (nqtot, df[2][0], diff0, 1, out_d1, 1);
                    Blas::Daxpy (nqtot, df[3][0], diff1, 1, out_d1, 1);
                }

                if(out_d2.num_elements())
                {
                    Vmath::Smul (nqtot, df[4][0], diff0, 1, out_d2, 1);
                    Blas::Daxpy (nqtot, df[5][0], diff1, 1, out_d2, 1);
                }
            }
        }


        void TriExp::v_PhysDeriv(const int dir,
                               const Array<OneD, const NekDouble>& inarray,
                               Array<OneD, NekDouble> &outarray)
        {
            switch(dir)
            {
            case 0:
                {
                    PhysDeriv(inarray, outarray, NullNekDouble1DArray, NullNekDouble1DArray);
                }
                break;
            case 1:
                {
                    PhysDeriv(inarray, NullNekDouble1DArray, outarray, NullNekDouble1DArray);
                }
                break;
            case 2:
                {
                    PhysDeriv(inarray, NullNekDouble1DArray, NullNekDouble1DArray, outarray);
                }
                break;
            default:
                {
                    ASSERTL1(false,"input dir is out of range");
                }
                break;
            }
        }

        void TriExp::v_PhysDirectionalDeriv(
            const Array<OneD, const NekDouble> &inarray,
            const Array<OneD, const NekDouble> &direction,
                  Array<OneD,       NekDouble> &out)
        {
            if(! out.num_elements())
            {
                return;
            }

            int    nquad0 = m_base[0]->GetNumPoints();
            int    nquad1 = m_base[1]->GetNumPoints();
            int    nqtot = nquad0*nquad1;

            const Array<TwoD, const NekDouble>& df = m_metricinfo->GetDerivFactors();

            Array<OneD,NekDouble> diff0(2*nqtot);
            Array<OneD,NekDouble> diff1(diff0+nqtot);

            // diff0 = du/d_xi, diff1 = du/d_eta
            StdTriExp::v_PhysDeriv(inarray, diff0, diff1);

            if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
            {
                Array<OneD, Array<OneD, NekDouble> > tangmat(2);


                // D^v_xi = v_x*d_xi/dx + v_y*d_xi/dy + v_z*d_xi/dz
                // D^v_eta = v_x*d_eta/dx + v_y*d_eta/dy + v_z*d_eta/dz
                for (int i=0; i< 2; ++i)
                {
                    tangmat[i] = Array<OneD, NekDouble>(nqtot,0.0);
                    for (int k=0; k<(m_geom->GetCoordim()); ++k)
                    {
                        Vmath::Vvtvp(nqtot,&df[2*k+i][0],1,&direction[k*nqtot],1,&tangmat[i][0],1,&tangmat[i][0],1);
                    }
                }

                /// D_v = D^v_xi * du/d_xi + D^v_eta * du/d_eta
                Vmath::Vmul  (nqtot,&tangmat[0][0],1,&diff0[0],1, &out[0], 1);
                Vmath::Vvtvp (nqtot,&tangmat[1][0],1,&diff1[0],1, &out[0], 1, &out[0],1);
            }
            else
            {
                ASSERTL1(m_metricinfo->GetGtype() == SpatialDomains::eDeformed,"Wrong route");
            }
        }


        void TriExp::v_FwdTrans(const Array<OneD, const NekDouble> & inarray,
                              Array<OneD,NekDouble> &outarray)
        {
            IProductWRTBase(inarray,outarray);

            // get Mass matrix inverse
            MatrixKey             masskey(StdRegions::eInvMass,
                                          DetShapeType(),*this);
            DNekScalMatSharedPtr  matsys = m_matrixManager[masskey];

            // copy inarray in case inarray == outarray
            NekVector<NekDouble> in (m_ncoeffs,outarray,eCopy);
            NekVector<NekDouble> out(m_ncoeffs,outarray,eWrapper);

            out = (*matsys)*in;
        }


        void TriExp::v_FwdTrans_BndConstrained(const Array<OneD, const NekDouble>& inarray,
                                             Array<OneD, NekDouble> &outarray)
        {
            int i,j;
            int npoints[2] = {m_base[0]->GetNumPoints(),
                              m_base[1]->GetNumPoints()};
            int nmodes[2]  = {m_base[0]->GetNumModes(),
                              m_base[1]->GetNumModes()};

            fill(outarray.get(), outarray.get()+m_ncoeffs, 0.0 );

            Array<OneD, NekDouble> physEdge[3];
            Array<OneD, NekDouble> coeffEdge[3];
            StdRegions::Orientation orient[3];
            for(i = 0; i < 3; i++)
            {
                physEdge[i]  = Array<OneD, NekDouble>(npoints[i!=0]);
                coeffEdge[i] = Array<OneD, NekDouble>(nmodes[i!=0]);
                orient[i]    = GetEorient(i);
            }

            for(i = 0; i < npoints[0]; i++)
            {
                physEdge[0][i] = inarray[i];
            }

            for(i = 0; i < npoints[1]; i++)
            {
                physEdge[1][i] = inarray[npoints[0]-1+i*npoints[0]];
                physEdge[2][i] = inarray[(npoints[1]-1)*npoints[0]-i*npoints[0]];
            }

            for(i = 0; i < 3; i++)
            {
                if( orient[i] == StdRegions::eBackwards )
                {
                    reverse( (physEdge[i]).get() , (physEdge[i]).get() + npoints[i!=0] );
                }
            }

            SegExpSharedPtr segexp[3];
            for(i = 0; i < 3; i++)
            {
                segexp[i] = MemoryManager<LocalRegions::SegExp>::AllocateSharedPtr(m_base[i!=0]->GetBasisKey(),GetGeom2D()->GetEdge(i));
            }

            Array<OneD, unsigned int> mapArray;
            Array<OneD, int>          signArray;
            NekDouble sign;

            for(i = 0; i < 3; i++)
            {
                segexp[i!=0]->FwdTrans_BndConstrained(physEdge[i],coeffEdge[i]);

                GetEdgeToElementMap(i,orient[i],mapArray,signArray);
                for(j=0; j < nmodes[i!=0]; j++)
                {
                    sign = (NekDouble) signArray[j];
                    outarray[ mapArray[j] ] = sign * coeffEdge[i][j];
                }
            }

            int nBoundaryDofs = NumBndryCoeffs();
            int nInteriorDofs = m_ncoeffs - nBoundaryDofs;

            if (nInteriorDofs > 0) {
                Array<OneD, NekDouble> tmp0(m_ncoeffs);
                Array<OneD, NekDouble> tmp1(m_ncoeffs);

                StdRegions::StdMatrixKey  stdmasskey(StdRegions::eMass,DetShapeType(),*this);
                MassMatrixOp(outarray,tmp0,stdmasskey);
                IProductWRTBase(inarray,tmp1);

                Vmath::Vsub(m_ncoeffs, tmp1, 1, tmp0, 1, tmp1, 1);

                // get Mass matrix inverse (only of interior DOF)
                // use block (1,1) of the static condensed system
                // note: this block alreay contains the inverse matrix
                MatrixKey             masskey(StdRegions::eMass,DetShapeType(),*this);
                DNekScalMatSharedPtr  matsys = (m_staticCondMatrixManager[masskey])->GetBlock(1,1);

                Array<OneD, NekDouble> rhs(nInteriorDofs);
                Array<OneD, NekDouble> result(nInteriorDofs);

                GetInteriorMap(mapArray);

                for(i = 0; i < nInteriorDofs; i++)
                {
                    rhs[i] = tmp1[ mapArray[i] ];
                }

                Blas::Dgemv('N', nInteriorDofs, nInteriorDofs, matsys->Scale(), &((matsys->GetOwnedMatrix())->GetPtr())[0],
                            nInteriorDofs,rhs.get(),1,0.0,result.get(),1);

                for(i = 0; i < nInteriorDofs; i++)
                {
                    outarray[ mapArray[i] ] = result[i];
                }
            }
        }


        void TriExp::v_IProductWRTBase(const Array<OneD, const NekDouble>& inarray,
                             Array<OneD, NekDouble> &outarray)
        {
            IProductWRTBase_SumFac(inarray,outarray);
        }


        void TriExp::v_IProductWRTDerivBase(const int dir,
                                  const Array<OneD, const NekDouble>& inarray,
                                  Array<OneD, NekDouble> & outarray)
        {
            IProductWRTDerivBase_SumFac(dir,inarray,outarray);
        }


        void TriExp::v_IProductWRTBase_SumFac(const Array<OneD, const NekDouble>& inarray,
                                            Array<OneD, NekDouble> &outarray)
        {
            int    nquad0 = m_base[0]->GetNumPoints();
            int    nquad1 = m_base[1]->GetNumPoints();
            int    order0 = m_base[0]->GetNumModes();

            Array<OneD,NekDouble> tmp(nquad0*nquad1+nquad1*order0);
            Array<OneD,NekDouble> wsp(tmp+nquad0*nquad1);

            MultiplyByQuadratureMetric(inarray,tmp);
            IProductWRTBase_SumFacKernel(m_base[0]->GetBdata(),m_base[1]->GetBdata(),tmp,outarray,wsp);
        }


        void TriExp::v_IProductWRTBase_MatOp(const Array<OneD, const NekDouble>& inarray,
                                           Array<OneD, NekDouble> &outarray)
        {
            int nq = GetTotPoints();
            MatrixKey      iprodmatkey(StdRegions::eIProductWRTBase,DetShapeType(),*this);
            DNekScalMatSharedPtr iprodmat = m_matrixManager[iprodmatkey];

            Blas::Dgemv('N',m_ncoeffs,nq,iprodmat->Scale(),(iprodmat->GetOwnedMatrix())->GetPtr().get(),
                        m_ncoeffs, inarray.get(), 1, 0.0, outarray.get(), 1);

        }


        void TriExp::v_IProductWRTDerivBase_SumFac(const int dir,
                                                 const Array<OneD, const NekDouble>& inarray,
                                                 Array<OneD, NekDouble> & outarray)
        {
            ASSERTL1((dir==0)||(dir==1)||(dir==2),"Invalid direction.");
            ASSERTL1((dir==2)?(m_geom->GetCoordim()==3):true,"Invalid direction.");

            int    i;
            int    nquad0 = m_base[0]->GetNumPoints();
            int    nquad1 = m_base[1]->GetNumPoints();
            int    nqtot  = nquad0*nquad1;
            int    nmodes0 = m_base[0]->GetNumModes();
            int    wspsize = max(max(nqtot,m_ncoeffs),nquad1*nmodes0);

            const Array<TwoD, const NekDouble>& df = m_metricinfo->GetDerivFactors();

            Array<OneD, NekDouble> tmp0 (6*wspsize);
            Array<OneD, NekDouble> tmp1 (tmp0 +   wspsize);
            Array<OneD, NekDouble> tmp2 (tmp0 + 2*wspsize);
            Array<OneD, NekDouble> tmp3 (tmp0 + 3*wspsize);
            Array<OneD, NekDouble> gfac0(tmp0 + 4*wspsize);
            Array<OneD, NekDouble> gfac1(tmp0 + 5*wspsize);

            const Array<OneD, const NekDouble>& z0 = m_base[0]->GetZ();
            const Array<OneD, const NekDouble>& z1 = m_base[1]->GetZ();

            // set up geometric factor: 2/(1-z1)
            for(i = 0; i < nquad1; ++i)
            {
                gfac0[i] = 2.0/(1-z1[i]);
            }
            for(i = 0; i < nquad0; ++i)
            {
                gfac1[i] = 0.5*(1+z0[i]);
            }

            for(i = 0; i < nquad1; ++i)
            {
                Vmath::Smul(nquad0,gfac0[i],&inarray[0]+i*nquad0,1,&tmp0[0]+i*nquad0,1);
            }

            for(i = 0; i < nquad1; ++i)
            {
                Vmath::Vmul(nquad0,&gfac1[0],1,&tmp0[0]+i*nquad0,1,&tmp1[0]+i*nquad0,1);
            }

            if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
            {
                Vmath::Vmul(nqtot,&df[2*dir][0],  1,&tmp0[0],   1,&tmp0[0],1);
                Vmath::Vmul(nqtot,&df[2*dir+1][0],1,&tmp1[0],   1,&tmp1[0],1);
                Vmath::Vmul(nqtot,&df[2*dir+1][0],1,&inarray[0],1,&tmp2[0],1);
            }
            else
            {
                Vmath::Smul(nqtot, df[2*dir][0],   tmp0,    1, tmp0, 1);
                Vmath::Smul(nqtot, df[2*dir+1][0], tmp1,    1, tmp1, 1);
                Vmath::Smul(nqtot, df[2*dir+1][0], inarray, 1, tmp2, 1);
            }
            Vmath::Vadd(nqtot, tmp0, 1, tmp1, 1, tmp1, 1);

            MultiplyByQuadratureMetric(tmp1,tmp1);
            MultiplyByQuadratureMetric(tmp2,tmp2);

            IProductWRTBase_SumFacKernel(m_base[0]->GetDbdata(),m_base[1]->GetBdata() ,tmp1,tmp3    ,tmp0);
            IProductWRTBase_SumFacKernel(m_base[0]->GetBdata() ,m_base[1]->GetDbdata(),tmp2,outarray,tmp0);
            Vmath::Vadd(m_ncoeffs, tmp3, 1, outarray, 1, outarray, 1);
        }


        void TriExp::v_IProductWRTDerivBase_MatOp(const int dir,
                                                const Array<OneD, const NekDouble>& inarray,
                                                Array<OneD, NekDouble> &outarray)
        {
            int nq = GetTotPoints();
            StdRegions::MatrixType mtype = StdRegions::eIProductWRTDerivBase0;

            switch(dir)
            {
            case 0:
                {
                    mtype = StdRegions::eIProductWRTDerivBase0;
                }
                break;
            case 1:
                {
                    mtype = StdRegions::eIProductWRTDerivBase1;
                }
                break;
            case 2:
                {
                    mtype = StdRegions::eIProductWRTDerivBase2;
                }
                break;
            default:
                {
                    ASSERTL1(false,"input dir is out of range");
                }
                break;
            }

            MatrixKey      iprodmatkey(mtype,DetShapeType(),*this);
            DNekScalMatSharedPtr iprodmat = m_matrixManager[iprodmatkey];

            Blas::Dgemv('N',m_ncoeffs,nq,iprodmat->Scale(),(iprodmat->GetOwnedMatrix())->GetPtr().get(),
                        m_ncoeffs, inarray.get(), 1, 0.0, outarray.get(), 1);

        }
        
        void TriExp::v_NormVectorIProductWRTBase(
            const Array<OneD, const NekDouble> &Fx,
            const Array<OneD, const NekDouble> &Fy,
            const Array<OneD, const NekDouble> &Fz,
                  Array<OneD,       NekDouble> &outarray)
        {
            int nq = m_base[0]->GetNumPoints()*m_base[1]->GetNumPoints();
            Array<OneD, NekDouble > Fn(nq);

            const Array<OneD, const Array<OneD, NekDouble> > &normals = 
                GetLeftAdjacentElementExp()->GetFaceNormal(
                    GetLeftAdjacentElementFace());
            
            if (m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
            {
                Vmath::Vvtvvtp(nq,&normals[0][0],1,&Fx[0],1,
                                  &normals[1][0],1,&Fy[0],1,&Fn[0],1);
                Vmath::Vvtvp  (nq,&normals[2][0],1,&Fz[0],1,&Fn[0],1,&Fn[0],1);
            }
            else
            {
                Vmath::Svtsvtp(nq,normals[0][0],&Fx[0],1,
                                  normals[1][0],&Fy[0],1,&Fn[0],1);
                Vmath::Svtvp  (nq,normals[2][0],&Fz[0],1,&Fn[0],1,&Fn[0],1);
            }

            IProductWRTBase(Fn,outarray);
        }

        void TriExp::v_GetCoords(Array<OneD,NekDouble> &coords_0,
                               Array<OneD,NekDouble> &coords_1,
                               Array<OneD,NekDouble> &coords_2)
        {
            LibUtilities::BasisSharedPtr CBasis0;
            LibUtilities::BasisSharedPtr CBasis1;
            Array<OneD,NekDouble>  x;

            ASSERTL0(m_geom, "m_geom not define");

            // get physical points defined in Geom
            m_geom->FillGeom();

            switch(m_geom->GetCoordim())
            {
            case 3:
                ASSERTL0(coords_2.num_elements() != 0,"output coords_2 is not defined");

                CBasis0 = m_geom->GetBasis(2,0);
                CBasis1 = m_geom->GetBasis(2,1);

                if((m_base[0]->GetBasisKey().SamePoints(CBasis0->GetBasisKey()))&&
                   (m_base[1]->GetBasisKey().SamePoints(CBasis1->GetBasisKey())))
                {
                    x = m_geom->UpdatePhys(2);
                    Blas::Dcopy(m_base[0]->GetNumPoints()*m_base[1]->GetNumPoints(),
                                x, 1, coords_2, 1);
                }
                else // Interpolate to Expansion point distribution
                {
                    LibUtilities::Interp2D(CBasis0->GetPointsKey(), CBasis1->GetPointsKey(),&(m_geom->UpdatePhys(2))[0],
                                           m_base[0]->GetPointsKey(),m_base[1]->GetPointsKey(),&coords_2[0]);
                }
            case 2:
                ASSERTL0(coords_1.num_elements(),
                         "output coords_1 is not defined");

                CBasis0 = m_geom->GetBasis(1,0);
                CBasis1 = m_geom->GetBasis(1,1);

                if((m_base[0]->GetBasisKey().SamePoints(CBasis0->GetBasisKey()))&&
                   (m_base[1]->GetBasisKey().SamePoints(CBasis1->GetBasisKey())))
                {
                    x = m_geom->UpdatePhys(1);
                    Blas::Dcopy(m_base[0]->GetNumPoints()*m_base[1]->GetNumPoints(),
                                x, 1, coords_1, 1);
                }
                else // LibUtilities::Interpolate to Expansion point distribution
                {
                    LibUtilities::Interp2D(CBasis0->GetPointsKey(), CBasis1->GetPointsKey(), &(m_geom->UpdatePhys(1))[0],
                                           m_base[0]->GetPointsKey(),m_base[1]->GetPointsKey(),&coords_1[0]);
                }
            case 1:
                ASSERTL0(coords_0.num_elements(),
                         "output coords_0 is not defined");

                CBasis0 = m_geom->GetBasis(0,0);
                CBasis1 = m_geom->GetBasis(0,1);

                if((m_base[0]->GetBasisKey().SamePoints(CBasis0->GetBasisKey()))&&
                   (m_base[1]->GetBasisKey().SamePoints(CBasis1->GetBasisKey())))
                {
                    x = m_geom->UpdatePhys(0);
                    Blas::Dcopy(m_base[0]->GetNumPoints()*m_base[1]->GetNumPoints(),
                                x, 1, coords_0, 1);
                }
                else // Interpolate to Expansion point distribution
                {
                    LibUtilities::Interp2D(CBasis0->GetPointsKey(), CBasis1->GetPointsKey(), &(m_geom->UpdatePhys(0))[0],
                                           m_base[0]->GetPointsKey(),m_base[1]->GetPointsKey(),&coords_0[0]);
                }
                break;
            default:
                ASSERTL0(false,"Number of dimensions are greater than 2");
                break;
            }
        }


        void TriExp::v_GetCoord(const Array<OneD, const NekDouble> &Lcoords,
                              Array<OneD,NekDouble> &coords)
        {
            int  i;

            ASSERTL1(Lcoords[0] >= -1.0 && Lcoords[1] <= 1.0 &&
                     Lcoords[1] >= -1.0 && Lcoords[1]  <=1.0,
                     "Local coordinates are not in region [-1,1]");

            m_geom->FillGeom();

            for(i = 0; i < m_geom->GetCoordim(); ++i)
            {
                coords[i] = m_geom->GetCoord(i,Lcoords);
            }
        }


        /** 
         * Given the local cartesian coordinate \a Lcoord evaluate the
         * value of physvals at this point by calling through to the
         * StdExpansion method
         */
        NekDouble TriExp::v_StdPhysEvaluate(
            const Array<OneD, const NekDouble> &Lcoord,
            const Array<OneD, const NekDouble> &physvals)
        {
            // Evaluate point in local (eta) coordinates.
            return StdTriExp::v_PhysEvaluate(Lcoord,physvals);
        }

        NekDouble TriExp::v_PhysEvaluate(const Array<OneD, const NekDouble> &coord)
        {
            return PhysEvaluate(coord,m_phys);
        }


        NekDouble TriExp::v_PhysEvaluate(const Array<OneD, const NekDouble> &coord, const Array<OneD, const NekDouble> & physvals)
        {
            Array<OneD,NekDouble> Lcoord = Array<OneD,NekDouble>(2);

            ASSERTL0(m_geom,"m_geom not defined");
            m_geom->GetLocCoords(coord,Lcoord);

            return StdTriExp::v_PhysEvaluate(Lcoord, physvals);
        }


        void TriExp::v_GetTracePhysVals(
                const int edge,
                const StdRegions::StdExpansionSharedPtr &EdgeExp,
                const Array<OneD, const NekDouble> &inarray,
                      Array<OneD,NekDouble> &outarray,
                      StdRegions::Orientation  orient)
        {
            v_GetEdgePhysVals(edge,EdgeExp,inarray,outarray);
        }

        void TriExp::v_GetEdgePhysVals(const int edge, const StdRegions::StdExpansionSharedPtr &EdgeExp,
                                     const Array<OneD, const NekDouble> &inarray,
                                     Array<OneD,NekDouble> &outarray)
        {
            int nquad0 = m_base[0]->GetNumPoints();
            int nquad1 = m_base[1]->GetNumPoints();

            // get points in Cartesian orientation
            switch(edge)
            {
            case 0:
                Vmath::Vcopy(nquad0,&(inarray[0]),1,&(outarray[0]),1);
                break;
            case 1:
                Vmath::Vcopy(nquad1,&(inarray[0])+(nquad0-1),nquad0,&(outarray[0]),1);
                break;
            case 2:
                Vmath::Vcopy(nquad1,&(inarray[0]),nquad0,&(outarray[0]),1);
                break;
            default:
                ASSERTL0(false,"edge value (< 3) is out of range");
                break;
            }

            // Interpolate if required
			if(m_base[edge?1:0]->GetPointsKey() != EdgeExp->GetBasis(0)->GetPointsKey())
			{
				Array<OneD,NekDouble> outtmp(max(nquad0,nquad1));
				
				outtmp = outarray;
				
				LibUtilities::Interp1D(m_base[edge?1:0]->GetPointsKey(),outtmp,
                     EdgeExp->GetBasis(0)->GetPointsKey(),outarray);
			}

            //Reverse data if necessary
            if(GetCartesianEorient(edge) == StdRegions::eBackwards)
            {
                Vmath::Reverse(EdgeExp->GetNumPoints(0),&outarray[0],1,
                               &outarray[0],1);
            }

        }
        
        
        void TriExp::v_GetEdgeInterpVals(
                const int edge,const Array<OneD, const NekDouble> &inarray,
                Array<OneD, NekDouble> &outarray)
        {
            ASSERTL0(false,
                     "Routine not implemented for triangular elements");
        }
        
        void TriExp::v_GetEdgeQFactors(
                const int edge, 
                Array<OneD, NekDouble> &outarray)
        {
            ASSERTL0(false, 
                     "Routine not implemented for triangular elements");
        }


        void TriExp::v_ComputeEdgeNormal(const int edge)
        {
            int i;
            const SpatialDomains::GeomFactorsSharedPtr & geomFactors = GetGeom()->GetMetricInfo();
            const SpatialDomains::GeomType type = geomFactors->GetGtype();
            const Array<TwoD, const NekDouble> & df = geomFactors->GetDerivFactors();
            const Array<OneD, const NekDouble> & jac  = geomFactors->GetJac();
            int nqe = m_base[0]->GetNumPoints();
            int dim = GetCoordim();

            m_edgeNormals[edge] = Array<OneD, Array<OneD, NekDouble> >(dim);
            Array<OneD, Array<OneD, NekDouble> > &normal = m_edgeNormals[edge];
            for (i = 0; i < dim; ++i)
            {
                normal[i] = Array<OneD, NekDouble>(nqe);
            }

            // Regular geometry case
            if((type == SpatialDomains::eRegular)||(type == SpatialDomains::eMovingRegular))
            {
                NekDouble fac;
                // Set up normals
                switch(edge)
                {
                case 0:
                    for(i = 0; i < GetCoordim(); ++i)
                    {
                        Vmath::Fill(nqe,-df[2*i+1][0],normal[i],1);
                    }
                    break;
                case 1:
                    for(i = 0; i < GetCoordim(); ++i)
                    {
                        Vmath::Fill(nqe,df[2*i+1][0] + df[2*i][0],normal[i],1);
                    }
                        break;
                case 2:
                    for(i = 0; i < GetCoordim(); ++i)
                    {
                        Vmath::Fill(nqe,-df[2*i][0],normal[i],1);
                    }
                    break;
                default:
                    ASSERTL0(false,"Edge is out of range (edge < 3)");
                }

                // normalise
                fac = 0.0;
                for(i =0 ; i < GetCoordim(); ++i)
                {
                    fac += normal[i][0]*normal[i][0];
                }
                fac = 1.0/sqrt(fac);
                for (i = 0; i < GetCoordim(); ++i)
                {
                    Vmath::Smul(nqe,fac,normal[i],1,normal[i],1);
                }
            }
            else   // Set up deformed normals
            {
                int j;

                int nquad0 = geomFactors->GetPointsKey(0).GetNumPoints();
                int nquad1 = geomFactors->GetPointsKey(1).GetNumPoints();

                LibUtilities::PointsKey from_key;

                Array<OneD,NekDouble> normals(GetCoordim()*max(nquad0,nquad1),0.0);
                Array<OneD,NekDouble> edgejac(GetCoordim()*max(nquad0,nquad1),0.0);

                // Extract Jacobian along edges and recover local
                // derivates (dx/dr) for polynomial interpolation by
                // multiplying m_gmat by jacobian
                switch(edge)
                {
                case 0:
                    for(j = 0; j < nquad0; ++j)
                    {
                        edgejac[j] = jac[j];
                        for(i = 0; i < GetCoordim(); ++i)
                        {
                            normals[i*nquad0+j] = -df[2*i+1][j]*edgejac[j];
                        }
                    }
                    from_key = geomFactors->GetPointsKey(0);
                    break;
                case 1:
                    for(j = 0; j < nquad1; ++j)
                    {
                        edgejac[j] = jac[nquad0*j+nquad0-1];
                        for(i = 0; i < GetCoordim(); ++i)
                        {
                            normals[i*nquad1+j] = (df[2*i][nquad0*j + nquad0-1] +  df[2*i+1][nquad0*j + nquad0-1])*edgejac[j];
                        }
                    }
                    from_key = geomFactors->GetPointsKey(1);
                    break;
                case 2:
                    for(j = 0; j < nquad1; ++j)
                    {
                        edgejac[j] = jac[nquad0*j];
                        for(i = 0; i < GetCoordim(); ++i)
                        {
                            normals[i*nquad1+j] = -df[2*i][nquad0*j]*edgejac[j];
                        }
                    }
                    from_key = geomFactors->GetPointsKey(1);
                    break;
                default:
                    ASSERTL0(false,"edge is out of range (edge < 3)");

                }

                int nq  = from_key.GetNumPoints();
                Array<OneD,NekDouble> work(nqe,0.0);

                // interpolate Jacobian and invert
                LibUtilities::Interp1D(from_key,jac,m_base[0]->GetPointsKey(),work);
                Vmath::Sdiv(nq,1.0,&work[0],1,&work[0],1);

                // interpolate
                for(i = 0; i < GetCoordim(); ++i)
                {
                    LibUtilities::Interp1D(from_key,&normals[i*nq],m_base[0]->GetPointsKey(),&normal[i][0]);
                    Vmath::Vmul(nqe,work,1,normal[i],1,normal[i],1);
                }

                //normalise normal vectors
                Vmath::Zero(nqe,work,1);
                for(i = 0; i < GetCoordim(); ++i)
                {
                    Vmath::Vvtvp(nqe,normal[i],1, normal[i],1,work,1,work,1);
                }

                Vmath::Vsqrt(nqe,work,1,work,1);
                Vmath::Sdiv(nqe,1.0,work,1,work,1);

                for(i = 0; i < GetCoordim(); ++i)
                {
                    Vmath::Vmul(nqe,normal[i],1,work,1,normal[i],1);
                }

                // Reverse direction so that points are in
                // anticlockwise direction if edge >=2
                if(edge >= 2)
                {
                    for(i = 0; i < GetCoordim(); ++i)
                    {
                        Vmath::Reverse(nqe,normal[i],1, normal[i],1);
                    }
                }
            }
            if(GetGeom()->GetEorient(edge) == StdRegions::eBackwards)
            {
                for(i = 0; i < GetCoordim(); ++i)
                {
                    if(geomFactors->GetGtype() == SpatialDomains::eDeformed)
                    {
                        Vmath::Reverse(nqe, normal[i], 1, normal[i],1);
                    }
                }
            }
        }


        void TriExp::v_WriteToFile(std::ofstream &outfile, OutputFormat format, const bool dumpVar, std::string var)
        {
            if(format==eTecplot)
            {
                int i,j;
                int nquad0 = m_base[0]->GetNumPoints();
                int nquad1 = m_base[1]->GetNumPoints();
                Array<OneD,NekDouble> coords[3];

                ASSERTL0(m_geom,"m_geom not defined");

                int     coordim  = m_geom->GetCoordim();

                coords[0] = Array<OneD,NekDouble>(nquad0*nquad1);
                coords[1] = Array<OneD,NekDouble>(nquad0*nquad1);
                coords[2] = Array<OneD,NekDouble>(nquad0*nquad1);

                GetCoords(coords[0],coords[1],coords[2]);

                if(dumpVar)
                {
                    outfile << "Variables = x";

                    if(coordim == 2)
                    {
                        outfile << ", y";
                    }
                    else if (coordim == 3)
                    {
                        outfile << ", y, z";
                    }
                    outfile << ", "<< var << std::endl << std::endl;
                }

                outfile << "Zone, I=" << nquad0 << ", J=" <<
                    nquad1 <<", F=Point" << std::endl;

                for(i = 0; i < nquad0*nquad1; ++i)
                {
                    for(j = 0; j < coordim; ++j)
                    {
                        outfile << coords[j][i] << " ";
                    }
                    outfile << m_phys[i] << std::endl;
                }
            }
            else if(format==eGmsh)
            {
                if(dumpVar)
                {
                    outfile<<"View.MaxRecursionLevel = 4;"<<endl;
                    outfile<<"View.TargetError = 0.00;"<<endl;
                    outfile<<"View.AdaptVisualizationGrid = 1;"<<endl;
                    outfile<<"View \" \" {"<<endl;
                }

                outfile<<"ST("<<endl;
                // write the coordinates of the vertices of the quadrilateral
                unsigned int vCoordDim = m_geom->GetCoordim();
                unsigned int nVertices = GetNverts();
                Array<OneD, NekDouble> coordVert(vCoordDim);
                for (unsigned int i = 0; i < nVertices; ++i)
                {
                    GetGeom2D()->GetVertex(i)->GetCoords(coordVert);
                    for (unsigned int j = 0; j < vCoordDim; ++j)
                    {
                        outfile << coordVert[j];
                        outfile << (j < 2 ? ", " : "");
                    }
                    for (unsigned int j = vCoordDim; j < 3; ++j)
                    {
                        outfile << " 0";
                        outfile << (j < 2 ? ", " : "");
                    }
                    outfile << (i < nVertices - 1 ? "," : "") << endl;
                }
                outfile<<")"<<endl;

                // calculate the coefficients (monomial format)
                int i,j;
                int maxnummodes = max(m_base[0]->GetNumModes(),m_base[1]->GetNumModes());

                const LibUtilities::PointsKey Pkey1Gmsh(maxnummodes,LibUtilities::eGaussGaussLegendre);
                const LibUtilities::PointsKey Pkey2Gmsh(maxnummodes,LibUtilities::eGaussGaussLegendre);
                const LibUtilities::BasisKey  Bkey1Gmsh(m_base[0]->GetBasisType(),maxnummodes,Pkey1Gmsh);
                const LibUtilities::BasisKey  Bkey2Gmsh(m_base[1]->GetBasisType(),maxnummodes,Pkey2Gmsh);
                LibUtilities::PointsType ptype = LibUtilities::eNodalTriElec;

                StdRegions::StdNodalTriExpSharedPtr EGmsh;
                EGmsh = MemoryManager<StdRegions::StdNodalTriExp>::
                    AllocateSharedPtr(Bkey1Gmsh,Bkey2Gmsh,ptype);

                Array<OneD,NekDouble> xi1(EGmsh->GetNcoeffs());
                Array<OneD,NekDouble> xi2(EGmsh->GetNcoeffs());
                EGmsh->GetNodalPoints(xi1,xi2);

                Array<OneD,NekDouble> x(EGmsh->GetNcoeffs());
                Array<OneD,NekDouble> y(EGmsh->GetNcoeffs());

                for(i=0;i<EGmsh->GetNcoeffs();i++)
                {
                    x[i] = 0.5*(1.0+xi1[i]);
                    y[i] = 0.5*(1.0+xi2[i]);
                }

                int cnt  = 0;
                int cnt2 = 0;
                int nDumpCoeffs = maxnummodes*maxnummodes;
                Array<TwoD, int> dumpExponentMap(nDumpCoeffs,3,0);
                Array<OneD, int> indexMap(EGmsh->GetNcoeffs(),0);
                Array<TwoD, int> exponentMap(EGmsh->GetNcoeffs(),3,0);
                for(i = 0; i < maxnummodes; i++)
                {
                    for(j = 0; j < maxnummodes; j++)
                    {
                        if(j<maxnummodes-i)
                        {
                            exponentMap[cnt][0] = j;
                            exponentMap[cnt][1] = i;
                            indexMap[cnt++]  = cnt2;
                        }

                        dumpExponentMap[cnt2][0]   = j;
                        dumpExponentMap[cnt2++][1] = i;
                    }
                }

                NekMatrix<NekDouble> vdm(EGmsh->GetNcoeffs(),EGmsh->GetNcoeffs());
                for(i = 0 ; i < EGmsh->GetNcoeffs(); i++)
                {
                    for(j = 0 ; j < EGmsh->GetNcoeffs(); j++)
                    {
                        vdm(i,j) = pow(x[i],exponentMap[j][0])*pow(y[i],exponentMap[j][1]);
                    }
                }

                vdm.Invert();

                Array<OneD, NekDouble> tmp2(EGmsh->GetNcoeffs());
                EGmsh->ModalToNodal(m_coeffs,tmp2);

                NekVector<NekDouble> in(EGmsh->GetNcoeffs(),tmp2,eWrapper);
                NekVector<NekDouble> out(EGmsh->GetNcoeffs());
                out = vdm*in;

                Array<OneD,NekDouble> dumpOut(nDumpCoeffs,0.0);
                for(i = 0 ; i < EGmsh->GetNcoeffs(); i++)
                {
                    dumpOut[ indexMap[i]  ] = out[i];
                }

                //write the coefficients
                outfile<<"{";
                for(i = 0; i < nDumpCoeffs; i++)
                {
                    outfile<<dumpOut[i];
                    if(i < nDumpCoeffs - 1)
                    {
                        outfile<<", ";
                    }
                }
                outfile<<"};"<<endl;

                if(dumpVar)
                {
                    outfile<<"INTERPOLATION_SCHEME"<<endl;
                    outfile<<"{"<<endl;
                    for(i=0; i < nDumpCoeffs; i++)
                    {
                        outfile<<"{";
                        for(j = 0; j < nDumpCoeffs; j++)
                        {
                            if(i==j)
                            {
                                outfile<<"1.00";
                            }
                            else
                            {
                                outfile<<"0.00";
                            }
                            if(j < nDumpCoeffs - 1)
                            {
                                outfile<<", ";
                            }
                        }
                        if(i < nDumpCoeffs - 1)
                        {
                            outfile<<"},"<<endl;
                        }
                        else
                        {
                            outfile<<"}"<<endl<<"}"<<endl;
                        }
                    }

                    outfile<<"{"<<endl;
                    for(i=0; i < nDumpCoeffs; i++)
                    {
                        outfile<<"{";
                        for(j = 0; j < 3; j++)
                        {
                            outfile<<dumpExponentMap[i][j];
                            if(j < 2)
                            {
                                outfile<<", ";
                            }
                        }
                        if(i < nDumpCoeffs  - 1)
                        {
                            outfile<<"},"<<endl;
                        }
                        else
                        {
                            outfile<<"}"<<endl<<"};"<<endl;
                        }
                    }
                    outfile<<"};"<<endl;
                }
            }
            else
            {
                ASSERTL0(false, "Output routine not implemented for requested type of output");
            }
        }


        int TriExp::v_GetCoordim()
        {
            return m_geom->GetCoordim();
        }


        void TriExp::v_ExtractDataToCoeffs(const NekDouble *data,
                                           const std::vector<unsigned int > &nummodes,  const int mode_offset,   NekDouble * coeffs)
        {
            int data_order0 = nummodes[mode_offset];
            int fillorder0  = min(m_base[0]->GetNumModes(),data_order0);
            int data_order1 = nummodes[mode_offset+1];
            int order1      = m_base[1]->GetNumModes();
            int fillorder1  = min(order1,data_order1);

            switch(m_base[0]->GetBasisType())
            {
            case LibUtilities::eModified_A:
                {
                    int i;
                    int cnt  = 0;
                    int cnt1 = 0;

                    ASSERTL1(m_base[1]->GetBasisType() == LibUtilities::eModified_B,
                             "Extraction routine not set up for this basis");

                    Vmath::Zero(m_ncoeffs,coeffs,1);
                    for(i = 0; i < fillorder0; ++i)
                    {
                        Vmath::Vcopy(fillorder1-i,&data[cnt],1,&coeffs[cnt1],1);
                        cnt  += data_order1-i;
                        cnt1 += order1-i;
                    }
                }
                break;
            default:
                ASSERTL0(false,"basis is either not set up or not hierarchicial");
            }
        }


        StdRegions::Orientation TriExp::v_GetEorient(int edge)
        {
            return GetGeom2D()->GetEorient(edge);
        }


        StdRegions::Orientation TriExp::v_GetCartesianEorient(int edge)
        {
            return GetGeom2D()->GetCartesianEorient(edge);
        }


        const LibUtilities::BasisSharedPtr& TriExp::v_GetBasis(int dir) const
            {
          ASSERTL1(dir >= 0 &&dir <= 1,"input dir is out of range");
          return m_base[dir];
        }


        int TriExp::v_GetNumPoints(const int dir) const
        {
            return GetNumPoints(dir);
        }


        DNekMatSharedPtr TriExp::v_GenMatrix(const StdRegions::StdMatrixKey &mkey)
        {
            DNekMatSharedPtr returnval;
            switch(mkey.GetMatrixType())
            {
            case StdRegions::eHybridDGHelmholtz:
            case StdRegions::eHybridDGLamToU:
            case StdRegions::eHybridDGLamToQ0:
            case StdRegions::eHybridDGLamToQ1:
            case StdRegions::eHybridDGLamToQ2:
            case StdRegions::eHybridDGHelmBndLam:
                returnval = Expansion2D::v_GenMatrix(mkey);
                break;
            default:
                returnval = StdTriExp::v_GenMatrix(mkey);
                break;
            }

            return returnval;
        }


        DNekMatSharedPtr TriExp::v_CreateStdMatrix(const StdRegions::StdMatrixKey &mkey)
        {
            LibUtilities::BasisKey bkey0 = m_base[0]->GetBasisKey();
            LibUtilities::BasisKey bkey1 = m_base[1]->GetBasisKey();
            StdRegions::StdTriExpSharedPtr tmp = MemoryManager<StdTriExp>::
                AllocateSharedPtr(bkey0,bkey1);

            return tmp->GetStdMatrix(mkey);
        }


        DNekScalMatSharedPtr TriExp::CreateMatrix(const MatrixKey &mkey)
        {
            DNekScalMatSharedPtr returnval;

            ASSERTL2(m_metricinfo->GetGtype() != SpatialDomains::eNoGeomType,"Geometric information is not set up");

            switch(mkey.GetMatrixType())
            {
            case StdRegions::eMass:
                {
                    if((m_metricinfo->GetGtype() == SpatialDomains::eDeformed)||
                       (mkey.GetNVarCoeff()))
                    {
                        NekDouble one = 1.0;
                        DNekMatSharedPtr mat = GenMatrix(mkey);
                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,mat);
                    }
                    else
                    {
                        NekDouble jac = (m_metricinfo->GetJac())[0];
                        DNekMatSharedPtr mat = GetStdMatrix(mkey);
                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(jac,mat);
                    }
                }
                break;
            case StdRegions::eInvMass:
                {
                    if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
                    {
                        NekDouble one = 1.0;
                        StdRegions::StdMatrixKey masskey(StdRegions::eMass,DetShapeType(),
                                                         *this);
                        DNekMatSharedPtr mat = GenMatrix(masskey);
                        mat->Invert();

                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,mat);
                    }
                    else
                    {
                        NekDouble fac = 1.0/(m_metricinfo->GetJac())[0];
                        DNekMatSharedPtr mat = GetStdMatrix(mkey);
                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(fac,mat);

                    }
                }
                break;
            case StdRegions::eWeakDeriv0:
            case StdRegions::eWeakDeriv1:
            case StdRegions::eWeakDeriv2:
                {
                    if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed || mkey.GetNVarCoeff())
                    {
                        NekDouble one = 1.0;
                        DNekMatSharedPtr mat = GenMatrix(mkey);

                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,mat);
                    }
                    else
                    {
                        NekDouble jac = (m_metricinfo->GetJac())[0];
                        Array<TwoD, const NekDouble> df = m_metricinfo->GetDerivFactors();
                        int dir = 0;
                        switch(mkey.GetMatrixType())
                        {
                            case StdRegions::eWeakDeriv0:
                                dir = 0;
                                break;
                            case StdRegions::eWeakDeriv1:
                                dir = 1;
                                break;
                            case StdRegions::eWeakDeriv2:
                                dir = 2;
                                break;
                            default:
                                break;
                        }

                        MatrixKey deriv0key(StdRegions::eWeakDeriv0,
                                            mkey.GetShapeType(), *this);
                        MatrixKey deriv1key(StdRegions::eWeakDeriv1,
                                            mkey.GetShapeType(), *this);

                        DNekMat &deriv0 = *GetStdMatrix(deriv0key);
                        DNekMat &deriv1 = *GetStdMatrix(deriv1key);

                        int rows = deriv0.GetRows();
                        int cols = deriv1.GetColumns();

                        DNekMatSharedPtr WeakDeriv = MemoryManager<DNekMat>::AllocateSharedPtr(rows,cols);
                        (*WeakDeriv) = df[2*dir][0]*deriv0 + df[2*dir+1][0]*deriv1;

                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(jac,WeakDeriv);
                    }
                }
                break;
            case StdRegions::eLaplacian:
                {
                    if( (m_metricinfo->GetGtype() == SpatialDomains::eDeformed) ||
                        (mkey.GetNVarCoeff() > 0)||(mkey.ConstFactorExists(StdRegions::eFactorSVVCutoffRatio)))
                    {
                        NekDouble one = 1.0;
                        DNekMatSharedPtr mat = GenMatrix(mkey);

                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,mat);
                    }
                    else
                    {
                        MatrixKey lap00key(StdRegions::eLaplacian00,
                                           mkey.GetShapeType(), *this);
                        MatrixKey lap01key(StdRegions::eLaplacian01,
                                           mkey.GetShapeType(), *this);
                        MatrixKey lap11key(StdRegions::eLaplacian11,
                                           mkey.GetShapeType(), *this);

                        DNekMat &lap00 = *GetStdMatrix(lap00key);
                        DNekMat &lap01 = *GetStdMatrix(lap01key);
                        DNekMat &lap11 = *GetStdMatrix(lap11key);

                        NekDouble jac = (m_metricinfo->GetJac())[0];
                        Array<TwoD, const NekDouble> gmat = m_metricinfo->GetGmat();

                        int rows = lap00.GetRows();
                        int cols = lap00.GetColumns();

                        DNekMatSharedPtr lap = MemoryManager<DNekMat>::AllocateSharedPtr(rows,cols);

                        (*lap) = gmat[0][0] * lap00 +
                                 gmat[1][0] * (lap01 + Transpose(lap01)) +
                                 gmat[3][0] * lap11;

                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(jac,lap);
                    }
                }
                break;
            case StdRegions::eInvLaplacianWithUnityMean:
                {
                    NekDouble one = 1.0;
                    MatrixKey lapkey(StdRegions::eLaplacian,mkey.GetShapeType(), *this);
                    DNekMatSharedPtr lmat = GenMatrix(lapkey);

                    // replace first column with inner product wrt 1
                    int nq = GetTotPoints();
                    Array<OneD, NekDouble> tmp(nq);
                    Array<OneD, NekDouble> outarray(m_ncoeffs);
                    Vmath::Fill(nq,one,tmp,1);
                    v_IProductWRTBase(tmp, outarray);

                    Vmath::Vcopy(m_ncoeffs,&outarray[0],1,
                                 &(lmat->GetPtr())[0],m_ncoeffs);

                    lmat->Invert();
                    returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,lmat); //Populate  matrix.
                }
                break;
            case StdRegions::eHelmholtz:
                {
                    NekDouble factor = mkey.GetConstFactor(StdRegions::eFactorLambda);

                    MatrixKey masskey(mkey, StdRegions::eMass);
                    DNekScalMat &MassMat = *(this->m_matrixManager[masskey]);

                    MatrixKey lapkey(mkey, StdRegions::eLaplacian);
                    DNekScalMat &LapMat = *(this->m_matrixManager[lapkey]);

                    int rows = LapMat.GetRows();
                    int cols = LapMat.GetColumns();

                    DNekMatSharedPtr helm = MemoryManager<DNekMat>::AllocateSharedPtr(rows,cols);

                    NekDouble one = 1.0;
                    (*helm) = LapMat + factor*MassMat;

                    returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,helm);
                }
                break;
            case StdRegions::eIProductWRTBase:
                {
                    if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
                    {
                        NekDouble one = 1.0;
                        DNekMatSharedPtr mat = GenMatrix(mkey);
                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,mat);
                    }
                    else
                    {
                        NekDouble jac = (m_metricinfo->GetJac())[0];
                        DNekMatSharedPtr mat = GetStdMatrix(mkey);
                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(jac,mat);
                    }
                }
                break;
            case StdRegions::eIProductWRTDerivBase0:
            case StdRegions::eIProductWRTDerivBase1:
            case StdRegions::eIProductWRTDerivBase2:
                {
                    if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
                    {
                        NekDouble one = 1.0;
                        DNekMatSharedPtr mat = GenMatrix(mkey);
                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,mat);
                    }
                    else
                    {
                        NekDouble jac = (m_metricinfo->GetJac())[0];

                        const Array<TwoD, const NekDouble>& df = m_metricinfo->GetDerivFactors();
                        int dir = 0;

                        switch(mkey.GetMatrixType())
                        {
                            case StdRegions::eIProductWRTDerivBase0:
                                dir = 0;
                                break;
                            case StdRegions::eIProductWRTDerivBase1:
                                dir = 1;
                                break;
                            case StdRegions::eIProductWRTDerivBase2:
                                dir = 2;
                                break;
                            default:
                                break;
                        }

                        MatrixKey iProdDeriv0Key(StdRegions::eIProductWRTDerivBase0,
                                                 mkey.GetShapeType(), *this);
                        MatrixKey iProdDeriv1Key(StdRegions::eIProductWRTDerivBase1,
                                                 mkey.GetShapeType(), *this);

                        DNekMat &stdiprod0 = *GetStdMatrix(iProdDeriv0Key);
                        DNekMat &stdiprod1 = *GetStdMatrix(iProdDeriv0Key);

                        int rows = stdiprod0.GetRows();
                        int cols = stdiprod1.GetColumns();

                        DNekMatSharedPtr mat = MemoryManager<DNekMat>::AllocateSharedPtr(rows,cols);
                        (*mat) = df[2*dir][0]*stdiprod0 + df[2*dir+1][0]*stdiprod1;

                        returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(jac,mat);
                    }
                }
                break;

            case StdRegions::eInvHybridDGHelmholtz:
                {
                    NekDouble one = 1.0;

                    MatrixKey hkey(StdRegions::eHybridDGHelmholtz, DetShapeType(), *this, mkey.GetConstFactors(), mkey.GetVarCoeffs());

                    DNekMatSharedPtr mat = GenMatrix(hkey);

                    mat->Invert();
                    returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,mat);
                }
                break;
            case StdRegions::ePreconLinearSpace:
                {
                    NekDouble one = 1.0;
                    MatrixKey helmkey(StdRegions::eHelmholtz, mkey.GetShapeType(), *this, mkey.GetConstFactors(), mkey.GetVarCoeffs());
                    DNekScalBlkMatSharedPtr helmStatCond = GetLocStaticCondMatrix(helmkey);
                    DNekScalMatSharedPtr A =helmStatCond->GetBlock(0,0);
                    DNekMatSharedPtr R=BuildVertexMatrix(A);

                    returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,R);
                }
                break;
            default:
                {
                    NekDouble        one = 1.0;
                    DNekMatSharedPtr mat = GenMatrix(mkey);

                    returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,mat);
                }
                break;
            }

            return returnval;
        }


        DNekScalBlkMatSharedPtr TriExp::CreateStaticCondMatrix(const MatrixKey &mkey)
        {
            DNekScalBlkMatSharedPtr returnval;

            ASSERTL2(m_metricinfo->GetGtype() != SpatialDomains::eNoGeomType,"Geometric information is not set up");

            // set up block matrix system
            unsigned int nbdry = NumBndryCoeffs();
            unsigned int nint = (unsigned int)(m_ncoeffs - nbdry);
            unsigned int exp_size[] = {nbdry,nint};
            unsigned int nblks = 2;
            returnval = MemoryManager<DNekScalBlkMat>::AllocateSharedPtr(nblks,nblks,exp_size,exp_size); //Really need a constructor which takes Arrays
            NekDouble factor = 1.0;

            switch(mkey.GetMatrixType())
            {
                // this can only use stdregions statically condensed system for mass matrix
            case StdRegions::eMass:
                if((m_metricinfo->GetGtype() == SpatialDomains::eDeformed)||(mkey.GetNVarCoeff()))
                {
                    factor = 1.0;
                    goto UseLocRegionsMatrix;
                }
                else
                {
                    factor = (m_metricinfo->GetJac())[0];
                    goto UseStdRegionsMatrix;
                }
                break;
            default:     // use Deformed case for both regular and deformed geometries
                factor = 1.0;
                goto UseLocRegionsMatrix;
                break;
            UseStdRegionsMatrix:
                {
                    NekDouble            invfactor = 1.0/factor;
                    NekDouble            one = 1.0;
                    DNekBlkMatSharedPtr  mat = GetStdStaticCondMatrix(mkey);
                    DNekScalMatSharedPtr Atmp;
                    DNekMatSharedPtr     Asubmat;

                    returnval->SetBlock(0,0,Atmp = MemoryManager<DNekScalMat>::AllocateSharedPtr(factor,Asubmat = mat->GetBlock(0,0)));
                    returnval->SetBlock(0,1,Atmp = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,Asubmat = mat->GetBlock(0,1)));
                    returnval->SetBlock(1,0,Atmp = MemoryManager<DNekScalMat>::AllocateSharedPtr(factor,Asubmat = mat->GetBlock(1,0)));
                    returnval->SetBlock(1,1,Atmp = MemoryManager<DNekScalMat>::AllocateSharedPtr(invfactor,Asubmat = mat->GetBlock(1,1)));
                }
                break;

                UseLocRegionsMatrix:
                {
                    int i,j;
                    NekDouble            invfactor = 1.0/factor;
                    NekDouble            one = 1.0;

                    DNekScalMat &mat = *GetLocMatrix(mkey);

                    DNekMatSharedPtr A = MemoryManager<DNekMat>::AllocateSharedPtr(nbdry,nbdry);
                    DNekMatSharedPtr B = MemoryManager<DNekMat>::AllocateSharedPtr(nbdry,nint);
                    DNekMatSharedPtr C = MemoryManager<DNekMat>::AllocateSharedPtr(nint,nbdry);
                    DNekMatSharedPtr D = MemoryManager<DNekMat>::AllocateSharedPtr(nint,nint);

                    Array<OneD,unsigned int> bmap(nbdry);
                    Array<OneD,unsigned int> imap(nint);
                    GetBoundaryMap(bmap);
                    GetInteriorMap(imap);

                    for(i = 0; i < nbdry; ++i)
                    {
                        for(j = 0; j < nbdry; ++j)
                        {
                            (*A)(i,j) = mat(bmap[i],bmap[j]);
                        }

                        for(j = 0; j < nint; ++j)
                        {
                            (*B)(i,j) = mat(bmap[i],imap[j]);
                        }
                    }

                    for(i = 0; i < nint; ++i)
                    {
                        for(j = 0; j < nbdry; ++j)
                        {
                            (*C)(i,j) = mat(imap[i],bmap[j]);
                        }

                        for(j = 0; j < nint; ++j)
                        {
                            (*D)(i,j) = mat(imap[i],imap[j]);
                        }
                    }

                    // Calculate static condensed system
                    if(nint)
                    {
                        D->Invert();
                        (*B) = (*B)*(*D);
                        (*A) = (*A) - (*B)*(*C);
                    }

                    DNekScalMatSharedPtr     Atmp;

                    returnval->SetBlock(0,0,Atmp = MemoryManager<DNekScalMat>::AllocateSharedPtr(factor,A));
                    returnval->SetBlock(0,1,Atmp = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,B));
                    returnval->SetBlock(1,0,Atmp = MemoryManager<DNekScalMat>::AllocateSharedPtr(factor,C));
                    returnval->SetBlock(1,1,Atmp = MemoryManager<DNekScalMat>::AllocateSharedPtr(invfactor,D));

                }
            }

            return returnval;
        }


        DNekScalMatSharedPtr TriExp::v_GetLocMatrix(const MatrixKey &mkey)
        {
            return m_matrixManager[mkey];
        }


        DNekScalBlkMatSharedPtr TriExp::v_GetLocStaticCondMatrix(const MatrixKey &mkey)
        {
            return m_staticCondMatrixManager[mkey];
        }

        void TriExp::v_DropLocStaticCondMatrix(const MatrixKey &mkey)
        {
            m_staticCondMatrixManager.DeleteObject(mkey);
        }



        void TriExp::v_MassMatrixOp(const Array<OneD, const NekDouble> &inarray,
                          Array<OneD,NekDouble> &outarray,
                          const StdRegions::StdMatrixKey &mkey)
        {
            StdExpansion::MassMatrixOp_MatFree(inarray,outarray,mkey);
        }


        void TriExp::v_LaplacianMatrixOp(const Array<OneD, const NekDouble> &inarray,
                               Array<OneD,NekDouble> &outarray,
                               const StdRegions::StdMatrixKey &mkey)
        {
                TriExp::LaplacianMatrixOp_MatFree(inarray,outarray,mkey);
        }


        void TriExp::v_LaplacianMatrixOp(const int k1, const int k2,
                               const Array<OneD, const NekDouble> &inarray,
                               Array<OneD,NekDouble> &outarray,
                               const StdRegions::StdMatrixKey &mkey)
        {
            StdExpansion::LaplacianMatrixOp_MatFree(k1,k2,inarray,outarray,mkey);
        }


        void TriExp::v_WeakDerivMatrixOp(const int i,
                               const Array<OneD, const NekDouble> &inarray,
                               Array<OneD,NekDouble> &outarray,
                               const StdRegions::StdMatrixKey &mkey)
        {
            StdExpansion::WeakDerivMatrixOp_MatFree(i,inarray,outarray,mkey);
        }


        void TriExp::v_WeakDirectionalDerivMatrixOp(const Array<OneD, const NekDouble> &inarray,
                                          Array<OneD,NekDouble> &outarray,
                                          const StdRegions::StdMatrixKey &mkey)
        {
            StdExpansion::WeakDirectionalDerivMatrixOp_MatFree(inarray,outarray,mkey);
        }


        void TriExp::v_MassLevelCurvatureMatrixOp(const Array<OneD, const NekDouble> &inarray,
                                          Array<OneD,NekDouble> &outarray,
                                          const StdRegions::StdMatrixKey &mkey)
        {
            StdExpansion::MassLevelCurvatureMatrixOp_MatFree(inarray,outarray,mkey);
        }


        void TriExp::v_HelmholtzMatrixOp(const Array<OneD, const NekDouble> &inarray,
                               Array<OneD,NekDouble> &outarray,
                               const StdRegions::StdMatrixKey &mkey)
        {
            TriExp::HelmholtzMatrixOp_MatFree(inarray,outarray,mkey);
        }


        void TriExp::v_GeneralMatrixOp_MatOp(const Array<OneD, const NekDouble> &inarray,
                                           Array<OneD,NekDouble> &outarray,
                                           const StdRegions::StdMatrixKey &mkey)
        {
            DNekScalMatSharedPtr   mat = GetLocMatrix(mkey);

            if(inarray.get() == outarray.get())
            {
                Array<OneD,NekDouble> tmp(m_ncoeffs);
                Vmath::Vcopy(m_ncoeffs,inarray.get(),1,tmp.get(),1);

                Blas::Dgemv('N',m_ncoeffs,m_ncoeffs,mat->Scale(),(mat->GetOwnedMatrix())->GetPtr().get(),
                            m_ncoeffs, tmp.get(), 1, 0.0, outarray.get(), 1);
            }
            else
            {
                Blas::Dgemv('N',m_ncoeffs,m_ncoeffs,mat->Scale(),(mat->GetOwnedMatrix())->GetPtr().get(),
                            m_ncoeffs, inarray.get(), 1, 0.0, outarray.get(), 1);
            }
        }


        void TriExp::v_LaplacianMatrixOp_MatFree_Kernel(
                    const Array<OneD, const NekDouble> &inarray,
                          Array<OneD,       NekDouble> &outarray,
                          Array<OneD,       NekDouble> &wsp)
        {
            if (m_metrics.count(MetricLaplacian00) == 0)
            {
                ComputeLaplacianMetric();
            }

            int       nquad0  = m_base[0]->GetNumPoints();
            int       nquad1  = m_base[1]->GetNumPoints();
            int       nqtot   = nquad0*nquad1;
            int       nmodes0 = m_base[0]->GetNumModes();
            int       nmodes1 = m_base[1]->GetNumModes();
            int       wspsize = max(max(max(nqtot,m_ncoeffs),nquad1*nmodes0),nquad0*nmodes1);

            ASSERTL1(wsp.num_elements() >= 3*wspsize,
                     "Workspace is of insufficient size.");

            const Array<OneD, const NekDouble>& base0  = m_base[0]->GetBdata();
            const Array<OneD, const NekDouble>& base1  = m_base[1]->GetBdata();
            const Array<OneD, const NekDouble>& dbase0 = m_base[0]->GetDbdata();
            const Array<OneD, const NekDouble>& dbase1 = m_base[1]->GetDbdata();
            const Array<OneD, const NekDouble>& metric00 = m_metrics[MetricLaplacian00];
            const Array<OneD, const NekDouble>& metric01 = m_metrics[MetricLaplacian01];
            const Array<OneD, const NekDouble>& metric11 = m_metrics[MetricLaplacian11];

            // Allocate temporary storage
            Array<OneD,NekDouble> wsp0(wsp);
            Array<OneD,NekDouble> wsp1(wsp+wspsize);
            Array<OneD,NekDouble> wsp2(wsp+2*wspsize);

            StdExpansion2D::PhysTensorDeriv(inarray,wsp1,wsp2);

            // wsp0 = k = g0 * wsp1 + g1 * wsp2 = g0 * du_dxi1 + g1 * du_dxi2
            // wsp2 = l = g1 * wsp1 + g2 * wsp2 = g0 * du_dxi1 + g1 * du_dxi2
            // where g0, g1 and g2 are the metric terms set up in the GeomFactors class
            // especially for this purpose
            Vmath::Vvtvvtp(nqtot,&metric00[0],1,&wsp1[0],1,&metric01[0],1,&wsp2[0],1,&wsp0[0],1);
            Vmath::Vvtvvtp(nqtot,&metric01[0],1,&wsp1[0],1,&metric11[0],1,&wsp2[0],1,&wsp2[0],1);

            // outarray = m = (D_xi1 * B)^T * k
            // wsp1     = n = (D_xi2 * B)^T * l
            IProductWRTBase_SumFacKernel(dbase0,base1,wsp0,outarray,wsp1);
            IProductWRTBase_SumFacKernel(base0,dbase1,wsp2,wsp1,    wsp0);

            // outarray = outarray + wsp1
            //          = L * u_hat
            Vmath::Vadd(m_ncoeffs,wsp1.get(),1,outarray.get(),1,outarray.get(),1);
        }


        void TriExp::v_ComputeLaplacianMetric()
        {
            if (m_metrics.count(MetricQuadrature) == 0)
            {
                ComputeQuadratureMetric();
            }

            unsigned int i, j;
            const SpatialDomains::GeomType type = m_metricinfo->GetGtype();
            const unsigned int nqtot = GetTotPoints();
            const unsigned int dim = 2;
            const MetricType m[3][3] = { {MetricLaplacian00, MetricLaplacian01, MetricLaplacian02},
                                       {MetricLaplacian01, MetricLaplacian11, MetricLaplacian12},
                                       {MetricLaplacian02, MetricLaplacian12, MetricLaplacian22}
            };

            Array<OneD, NekDouble> dEta_dXi[2] = {Array<OneD, NekDouble>(nqtot,1.0),
                                                  Array<OneD, NekDouble>(nqtot,1.0)};

            for (i = 0; i < dim; ++i)
            {
                for (j = i; j < dim; ++j)
                {
                    m_metrics[m[i][j]] = Array<OneD, NekDouble>(nqtot);
                }
            }

            const Array<OneD, const NekDouble>& z0 = m_base[0]->GetZ();
            const Array<OneD, const NekDouble>& z1 = m_base[1]->GetZ();
            const unsigned int nquad0 = m_base[0]->GetNumPoints();
            const unsigned int nquad1 = m_base[1]->GetNumPoints();
            const Array<TwoD, const NekDouble>& df   = m_metricinfo->GetDerivFactors();

            for(i = 0; i < nquad1; i++)
            {
                Blas::Dscal(nquad0,2.0/(1-z1[i]),&dEta_dXi[0][0]+i*nquad0,1);
                Blas::Dscal(nquad0,2.0/(1-z1[i]),&dEta_dXi[1][0]+i*nquad0,1);
            }
            for(i = 0; i < nquad0; i++)
            {
                Blas::Dscal(nquad1,0.5*(1+z0[i]),&dEta_dXi[1][0]+i,nquad0);
            }

            Array<OneD, NekDouble> tmp(nqtot);
            if((type == SpatialDomains::eRegular ||
                type == SpatialDomains::eMovingRegular))
            {
                Vmath::Smul (nqtot,df[0][0],&dEta_dXi[0][0],1,&tmp[0],1);
                Vmath::Svtvp(nqtot,df[1][0],&dEta_dXi[1][0],1,&tmp[0],1,&tmp[0],1);

                Vmath::Vmul (nqtot,&tmp[0],1,   &tmp[0],1,&m_metrics[MetricLaplacian00][0],1);
                Vmath::Smul (nqtot,df[1][0],&tmp[0],1,&m_metrics[MetricLaplacian01][0],1);


                Vmath::Smul (nqtot,df[2][0],&dEta_dXi[0][0],1,&tmp[0],1);
                Vmath::Svtvp(nqtot,df[3][0],&dEta_dXi[1][0],1,&tmp[0],1,&tmp[0],1);

                Vmath::Vvtvp(nqtot,&tmp[0],1,   &tmp[0],1,&m_metrics[MetricLaplacian00][0],1,&m_metrics[MetricLaplacian00][0],1);
                Vmath::Svtvp(nqtot,df[3][0],&tmp[0],1,&m_metrics[MetricLaplacian01][0],1,&m_metrics[MetricLaplacian01][0],1);

                if(GetCoordim() == 3)
                {
                    Vmath::Smul (nqtot,df[4][0],&dEta_dXi[0][0],1,&tmp[0],1);
                    Vmath::Svtvp(nqtot,df[5][0],&dEta_dXi[1][0],1,&tmp[0],1,&tmp[0],1);

                    Vmath::Vvtvp(nqtot,&tmp[0],1,   &tmp[0],1,&m_metrics[MetricLaplacian00][0],1,&m_metrics[MetricLaplacian00][0],1);
                    Vmath::Svtvp(nqtot,df[5][0],&tmp[0],1,&m_metrics[MetricLaplacian01][0],1,&m_metrics[MetricLaplacian01][0],1);
                }

                NekDouble g2 = df[1][0]*df[1][0] + df[3][0]*df[3][0];
                if(GetCoordim() == 3)
                {
                    g2 += df[5][0]*df[5][0];
                }
                Vmath::Fill(nqtot,g2,&m_metrics[MetricLaplacian11][0],1);
            }
            else
            {

                Vmath::Vmul (nqtot,&df[0][0],1,&dEta_dXi[0][0],1,&tmp[0],1);
                Vmath::Vvtvp(nqtot,&df[1][0],1,&dEta_dXi[1][0],1,&tmp[0],1,&tmp[0],1);

                Vmath::Vmul (nqtot,&tmp[0],  1,&tmp[0],  1,&m_metrics[MetricLaplacian00][0],1);
                Vmath::Vmul (nqtot,&df[1][0],1,&tmp[0],  1,&m_metrics[MetricLaplacian01][0],1);
                Vmath::Vmul (nqtot,&df[1][0],1,&df[1][0],1,&m_metrics[MetricLaplacian11][0],1);


                Vmath::Vmul (nqtot,&df[2][0],1,&dEta_dXi[0][0],1,&tmp[0],1);
                Vmath::Vvtvp(nqtot,&df[3][0],1,&dEta_dXi[1][0],1,&tmp[0],1,&tmp[0],1);

                Vmath::Vvtvp(nqtot,&tmp[0],  1,&tmp[0],  1,&m_metrics[MetricLaplacian00][0],1,&m_metrics[MetricLaplacian00][0],1);
                Vmath::Vvtvp(nqtot,&df[3][0],1,&tmp[0],  1,&m_metrics[MetricLaplacian01][0],1,&m_metrics[MetricLaplacian01][0],1);
                Vmath::Vvtvp(nqtot,&df[3][0],1,&df[3][0],1,&m_metrics[MetricLaplacian11][0],1,&m_metrics[MetricLaplacian11][0],1);

                if(GetCoordim() == 3)
                {
                    Vmath::Vmul (nqtot,&df[4][0],1,&dEta_dXi[0][0],1,&tmp[0],1);
                    Vmath::Vvtvp(nqtot,&df[5][0],1,&dEta_dXi[1][0],1,&tmp[0],1,&tmp[0],1);

                    Vmath::Vvtvp(nqtot,&tmp[0],  1,&tmp[0],  1,&m_metrics[MetricLaplacian00][0],1,&m_metrics[MetricLaplacian00][0],1);
                    Vmath::Vvtvp(nqtot,&df[5][0],1,&tmp[0],  1,&m_metrics[MetricLaplacian01][0],1,&m_metrics[MetricLaplacian01][0],1);
                    Vmath::Vvtvp(nqtot,&df[5][0],1,&df[5][0],1,&m_metrics[MetricLaplacian11][0],1,&m_metrics[MetricLaplacian11][0],1);
                }
            }

            for (unsigned int i = 0; i < dim; ++i)
            {
                for (unsigned int j = i; j < dim; ++j)
                {
                    MultiplyByQuadratureMetric(m_metrics[m[i][j]],
                                               m_metrics[m[i][j]]);

                }
            }

        }

    }
}

