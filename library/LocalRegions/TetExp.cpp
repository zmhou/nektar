///////////////////////////////////////////////////////////////////////////////
//
// File TetExp.cpp
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
// Description:
//
///////////////////////////////////////////////////////////////////////////////

#include <LocalRegions/TetExp.h>
#include <SpatialDomains/SegGeom.h>

#include <LibUtilities/Foundations/Interp.h>

namespace Nektar
{
    namespace LocalRegions
    {
        /**
         * @class TetExp
         * Defines a Tetrahedral local expansion.
         */

        /**
	 * \brief Constructor using BasisKey class for quadrature points and 
	 * order definition 
	 *
         * @param   Ba          Basis key for first coordinate.
         * @param   Bb          Basis key for second coordinate.
         * @param   Bc          Basis key for third coordinate.
         */
        TetExp::TetExp( const LibUtilities::BasisKey &Ba,
                        const LibUtilities::BasisKey &Bb,
                        const LibUtilities::BasisKey &Bc,
                        const SpatialDomains::TetGeomSharedPtr &geom
                        ):
            StdExpansion  (LibUtilities::StdTetData::getNumberOfCoefficients(Ba.GetNumModes(),Bb.GetNumModes(),Bc.GetNumModes()),3,Ba,Bb,Bc),
            StdExpansion3D(LibUtilities::StdTetData::getNumberOfCoefficients(Ba.GetNumModes(),Bb.GetNumModes(),Bc.GetNumModes()),Ba,Bb,Bc),
            StdRegions::StdTetExp(Ba,Bb,Bc),
            Expansion     (geom),
            Expansion3D   (geom),
            m_matrixManager(
                    boost::bind(&TetExp::CreateMatrix, this, _1),
                    std::string("TetExpMatrix")),
            m_staticCondMatrixManager(
                    boost::bind(&TetExp::CreateStaticCondMatrix, this, _1),
                    std::string("TetExpStaticCondMatrix"))
        {
        }


        /**
	 * \brief Copy Constructor
	 */
        TetExp::TetExp(const TetExp &T):
            StdExpansion(T),
            StdExpansion3D(T),
            StdRegions::StdTetExp(T),
            Expansion(T),
            Expansion3D(T),
            m_matrixManager(T.m_matrixManager),
            m_staticCondMatrixManager(T.m_staticCondMatrixManager)
        {
        }

        /**
	 * \brief Destructor
	 */
        TetExp::~TetExp()
        {
        }

        
        //-----------------------------
        // Integration Methods
        //-----------------------------
        /**
         * \brief Integrate the physical point list \a inarray over region
         *
         * @param   inarray     Definition of function to be returned at
         *                      quadrature point of expansion.
         * @returns \f$\int^1_{-1}\int^1_{-1} \int^1_{-1}
         *   u(\eta_1, \eta_2, \eta_3) J[i,j,k] d \eta_1 d \eta_2 d \eta_3 \f$
         * where \f$inarray[i,j,k] = u(\eta_{1i},\eta_{2j},\eta_{3k})
         * \f$ and \f$ J[i,j,k] \f$ is the Jacobian evaluated at the quadrature
         * point.
         */
        NekDouble TetExp::v_Integral(
                  const Array<OneD, const NekDouble> &inarray)
        {
            int    nquad0 = m_base[0]->GetNumPoints();
            int    nquad1 = m_base[1]->GetNumPoints();
            int    nquad2 = m_base[2]->GetNumPoints();
            Array<OneD, const NekDouble> jac = m_metricinfo->GetJac();
            NekDouble retrunVal;
            Array<OneD,NekDouble> tmp(nquad0*nquad1*nquad2);

            // multiply inarray with Jacobian
            if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
            {
                Vmath::Vmul(nquad0*nquad1*nquad2,&jac[0],1,
                            (NekDouble*)&inarray[0],1, &tmp[0],1);
            }
            else
            {
                Vmath::Smul(nquad0*nquad1*nquad2,(NekDouble) jac[0],
                            (NekDouble*)&inarray[0],1,&tmp[0],1);
            }

            // call StdTetExp version;
            retrunVal = StdTetExp::v_Integral(tmp);

            return retrunVal;
        }
        

        //-----------------------------
        // Differentiation Methods
        //-----------------------------
        /**
	 * \brief Differentiate \a inarray in the three coordinate directions.
	 * 
         * @param   inarray     Input array of values at quadrature points to
         *                      be differentiated.
         * @param   out_d0      Derivative in first coordinate direction.
         * @param   out_d1      Derivative in second coordinate direction.
         * @param   out_d2      Derivative in third coordinate direction.
         */
        void TetExp::v_PhysDeriv(
            const Array<OneD, const NekDouble> &inarray,
                  Array<OneD,       NekDouble> &out_d0,
                  Array<OneD,       NekDouble> &out_d1,
                  Array<OneD,       NekDouble> &out_d2)
        {
            int  TotPts = m_base[0]->GetNumPoints()*m_base[1]->GetNumPoints()*
                m_base[2]->GetNumPoints();
            
            Array<TwoD, const NekDouble> df = m_metricinfo->GetDerivFactors();
            Array<OneD,NekDouble> Diff0 = Array<OneD,NekDouble>(3*TotPts);
            Array<OneD,NekDouble> Diff1 = Diff0 + TotPts;
            Array<OneD,NekDouble> Diff2 = Diff1 + TotPts;
            
            StdTetExp::v_PhysDeriv(inarray, Diff0, Diff1, Diff2);
            
            if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
            {
                if(out_d0.num_elements())
                {
                    Vmath::Vmul  (TotPts,&df[0][0],1,&Diff0[0],1, &out_d0[0], 1);
                    Vmath::Vvtvp (TotPts,&df[1][0],1,&Diff1[0],1, &out_d0[0], 1,&out_d0[0],1);
                    Vmath::Vvtvp (TotPts,&df[2][0],1,&Diff2[0],1, &out_d0[0], 1,&out_d0[0],1);
                }
                
                if(out_d1.num_elements())
                {
                    Vmath::Vmul  (TotPts,&df[3][0],1,&Diff0[0],1, &out_d1[0], 1);
                    Vmath::Vvtvp (TotPts,&df[4][0],1,&Diff1[0],1, &out_d1[0], 1,&out_d1[0],1);
                    Vmath::Vvtvp (TotPts,&df[5][0],1,&Diff2[0],1, &out_d1[0], 1,&out_d1[0],1);
                }

                if(out_d2.num_elements())
                {
                    Vmath::Vmul  (TotPts,&df[6][0],1,&Diff0[0],1, &out_d2[0], 1);
                    Vmath::Vvtvp (TotPts,&df[7][0],1,&Diff1[0],1, &out_d2[0], 1, &out_d2[0],1);
                    Vmath::Vvtvp (TotPts,&df[8][0],1,&Diff2[0],1, &out_d2[0], 1, &out_d2[0],1);
                }
            }
            else // regular geometry
            {
                if(out_d0.num_elements())
                {
                    Vmath::Smul (TotPts,df[0][0],&Diff0[0],1, &out_d0[0], 1);
                    Blas::Daxpy (TotPts,df[1][0],&Diff1[0],1, &out_d0[0], 1);
                    Blas::Daxpy (TotPts,df[2][0],&Diff2[0],1, &out_d0[0], 1);
                }

                if(out_d1.num_elements())
                {
                    Vmath::Smul (TotPts,df[3][0],&Diff0[0],1, &out_d1[0], 1);
                    Blas::Daxpy (TotPts,df[4][0],&Diff1[0],1, &out_d1[0], 1);
                    Blas::Daxpy (TotPts,df[5][0],&Diff2[0],1, &out_d1[0], 1);
                }

                if(out_d2.num_elements())
                {
                    Vmath::Smul (TotPts,df[6][0],&Diff0[0],1, &out_d2[0], 1);
                    Blas::Daxpy (TotPts,df[7][0],&Diff1[0],1, &out_d2[0], 1);
                    Blas::Daxpy (TotPts,df[8][0],&Diff2[0],1, &out_d2[0], 1);
                }
            }
        }


        //-----------------------------
        // Transforms
        //-----------------------------
        /**
	 * \brief Forward transform from physical quadrature space stored in
	 * \a inarray and evaluate the expansion coefficients and store
	 * in \a (this)->_coeffs
	 *
         * @param   inarray     Array of physical quadrature points to be
         *                      transformed.
         * @param   outarray    Array of coefficients to update.
         */
        void TetExp::v_FwdTrans( 
                 const Array<OneD, const NekDouble> & inarray,
                       Array<OneD,NekDouble> &outarray)
        {
            if((m_base[0]->Collocation())&&(m_base[1]->Collocation())&&(m_base[2]->Collocation()))
            {
                Vmath::Vcopy(GetNcoeffs(),&inarray[0],1,&m_coeffs[0],1);
            }
            else
            {
                IProductWRTBase(inarray,outarray);
                
                // get Mass matrix inverse
                MatrixKey             masskey(StdRegions::eInvMass,
                                              DetShapeType(),*this);
                DNekScalMatSharedPtr  matsys = m_matrixManager[masskey];

                // copy inarray in case inarray == outarray
                DNekVec in (m_ncoeffs,outarray);
                DNekVec out(m_ncoeffs,outarray,eWrapper);

                out = (*matsys)*in;
            }
        }

        //-----------------------------
        // Inner product functions
        //-----------------------------
        /**
	 * \brief Calculate the inner product of inarray with respect to the
	 * basis B=m_base0*m_base1*m_base2 and put into outarray:
	 *
         * \f$ \begin{array}{rcl} I_{pqr} = (\phi_{pqr}, u)_{\delta}
         *   & = & \sum_{i=0}^{nq_0} \sum_{j=0}^{nq_1} \sum_{k=0}^{nq_2}
         *     \psi_{p}^{a} (\eta_{1i}) \psi_{pq}^{b} (\eta_{2j}) \psi_{pqr}^{c}
         *     (\eta_{3k}) w_i w_j w_k u(\eta_{1,i} \eta_{2,j} \eta_{3,k})
         * J_{i,j,k}\\ & = & \sum_{i=0}^{nq_0} \psi_p^a(\eta_{1,i})
         *   \sum_{j=0}^{nq_1} \psi_{pq}^b(\eta_{2,j}) \sum_{k=0}^{nq_2}
         *   \psi_{pqr}^c u(\eta_{1i},\eta_{2j},\eta_{3k}) J_{i,j,k}
         * \end{array} \f$ \n
         * where
         * \f$ \phi_{pqr} (\xi_1 , \xi_2 , \xi_3)
         *   = \psi_p^a (\eta_1) \psi_{pq}^b (\eta_2) \psi_{pqr}^c (\eta_3) \f$
         * which can be implemented as \n
         * \f$f_{pqr} (\xi_{3k})
         *   = \sum_{k=0}^{nq_3} \psi_{pqr}^c u(\eta_{1i},\eta_{2j},\eta_{3k})
         * J_{i,j,k} = {\bf B_3 U}   \f$ \n
         * \f$ g_{pq} (\xi_{3k})
         *   = \sum_{j=0}^{nq_1} \psi_{pq}^b (\xi_{2j}) f_{pqr} (\xi_{3k})
         *   = {\bf B_2 F}  \f$ \n
         * \f$ (\phi_{pqr}, u)_{\delta}
         *   = \sum_{k=0}^{nq_0} \psi_{p}^a (\xi_{3k}) g_{pq} (\xi_{3k})
         *   = {\bf B_1 G} \f$
         */
        void TetExp::v_IProductWRTBase(
            const Array<OneD, const NekDouble> &inarray,
                  Array<OneD,       NekDouble> &outarray)
        {
            v_IProductWRTBase_SumFac(inarray, outarray);
        }

        void TetExp::v_IProductWRTBase_SumFac(
            const Array<OneD, const NekDouble> &inarray,
                  Array<OneD,       NekDouble> &outarray)
        {
            const int nquad0 = m_base[0]->GetNumPoints();
            const int nquad1 = m_base[1]->GetNumPoints();
            const int nquad2 = m_base[2]->GetNumPoints();
            const int order0 = m_base[0]->GetNumModes();
            const int order1 = m_base[1]->GetNumModes();
            Array<OneD, NekDouble> wsp(nquad1*nquad2*order0 +
                                       nquad2*order0*(order1+1)/2);
            Array<OneD, NekDouble> tmp(nquad0*nquad1*nquad2);

            MultiplyByQuadratureMetric(inarray, tmp);
            IProductWRTBase_SumFacKernel(m_base[0]->GetBdata(),
                                         m_base[1]->GetBdata(),
                                         m_base[2]->GetBdata(),
                                         tmp,outarray,wsp,
                                         true,true,true);
        }

        /**
         * @brief Calculates the inner product \f$ I_{pqr} = (u,
         * \partial_{x_i} \phi_{pqr}) \f$.
         * 
         * The derivative of the basis functions is performed using the chain
         * rule in order to incorporate the geometric factors. Assuming that
         * the basis functions are a tensor product
         * \f$\phi_{pqr}(\eta_1,\eta_2,\eta_3) =
         * \phi_1(\eta_1)\phi_2(\eta_2)\phi_3(\eta_3)\f$, this yields the
         * result
         * 
         * \f[
         * I_{pqr} = \sum_{j=1}^3 \left(u, \frac{\partial u}{\partial \eta_j}
         * \frac{\partial \eta_j}{\partial x_i}\right)
         * \f]
         * 
         * In the prismatic element, we must also incorporate a second set of
         * geometric factors which incorporate the collapsed co-ordinate
         * system, so that
         * 
         * \f[ \frac{\partial\eta_j}{\partial x_i} = \sum_{k=1}^3
         * \frac{\partial\eta_j}{\partial\xi_k}\frac{\partial\xi_k}{\partial
         * x_i} \f]
         * 
         * These derivatives can be found on p152 of Sherwin & Karniadakis.
         * 
         * @param dir       Direction in which to take the derivative.
         * @param inarray   The function \f$ u \f$.
         * @param outarray  Value of the inner product.
         */
        void TetExp::v_IProductWRTDerivBase(
            const int                           dir, 
            const Array<OneD, const NekDouble> &inarray, 
                  Array<OneD,       NekDouble> &outarray)
        {
            const int nquad0 = m_base[0]->GetNumPoints();
            const int nquad1 = m_base[1]->GetNumPoints();
            const int nquad2 = m_base[2]->GetNumPoints();
            const int order0 = m_base[0]->GetNumModes ();
            const int order1 = m_base[1]->GetNumModes ();
            const int nqtot  = nquad0*nquad1*nquad2;
            int i, j;

            const Array<OneD, const NekDouble> &z0 = m_base[0]->GetZ();
            const Array<OneD, const NekDouble> &z1 = m_base[1]->GetZ();
            const Array<OneD, const NekDouble> &z2 = m_base[2]->GetZ();
            
            Array<OneD, NekDouble> h0   (nqtot);
            Array<OneD, NekDouble> h1   (nqtot);
            Array<OneD, NekDouble> h2   (nqtot);
            Array<OneD, NekDouble> h3   (nqtot);
            Array<OneD, NekDouble> tmp1 (nqtot);
            Array<OneD, NekDouble> tmp2 (nqtot);
            Array<OneD, NekDouble> tmp3 (nqtot);
            Array<OneD, NekDouble> tmp4 (nqtot);
            Array<OneD, NekDouble> tmp5 (nqtot);
            Array<OneD, NekDouble> tmp6 (m_ncoeffs);
            Array<OneD, NekDouble> wsp  (nquad1*nquad2*order0 +
                                         nquad2*order0*(order1+1)/2);
            
            const Array<TwoD, const NekDouble>& df = m_metricinfo->GetDerivFactors();

            MultiplyByQuadratureMetric(inarray,tmp1);
            
            if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
            {
                Vmath::Vmul(nqtot,&df[3*dir][0],  1,tmp1.get(),1,tmp2.get(),1);
                Vmath::Vmul(nqtot,&df[3*dir+1][0],1,tmp1.get(),1,tmp3.get(),1);
                Vmath::Vmul(nqtot,&df[3*dir+2][0],1,tmp1.get(),1,tmp4.get(),1);
            }
            else
            {
                Vmath::Smul(nqtot, df[3*dir  ][0],tmp1.get(),1,tmp2.get(), 1);
                Vmath::Smul(nqtot, df[3*dir+1][0],tmp1.get(),1,tmp3.get(), 1);
                Vmath::Smul(nqtot, df[3*dir+2][0],tmp1.get(),1,tmp4.get(), 1);
            }
            
            const int nq01 = nquad0*nquad1;
            const int nq12 = nquad1*nquad2;

            for(j = 0; j < nquad2; ++j)
            {
                for(i = 0; i < nquad1; ++i)
                {
                    Vmath::Fill(nquad0, 4.0/(1.0-z1[i])/(1.0-z2[j]),
                                &h0[0]+i*nquad0 + j*nq01,1);
                    Vmath::Fill(nquad0, 2.0/(1.0-z1[i])/(1.0-z2[j]),
                                &h1[0]+i*nquad0 + j*nq01,1);
                    Vmath::Fill(nquad0, 2.0/(1.0-z2[j]),
                                &h2[0]+i*nquad0 + j*nq01,1);
                    Vmath::Fill(nquad0, (1.0+z1[i])/(1.0-z2[j]),
                                &h3[0]+i*nquad0 + j*nq01,1);
                }
            }

            for(i = 0; i < nquad0; i++)
            {
                Blas::Dscal(nq12, 1+z0[i], &h1[0]+i, nquad0);
            }
            
            // Assemble terms for first IP.
            Vmath::Vvtvvtp(nqtot, &tmp2[0], 1, &h0[0], 1,
                                  &tmp3[0], 1, &h1[0], 1,
                                  &tmp5[0], 1);
            Vmath::Vvtvp  (nqtot, &tmp4[0], 1, &h1[0], 1,
                                  &tmp5[0], 1, &tmp5[0], 1);

            IProductWRTBase_SumFacKernel(m_base[0]->GetDbdata(),
                                         m_base[1]->GetBdata (),
                                         m_base[2]->GetBdata (),
                                         tmp5,outarray,wsp,
                                         true,true,true);

            // Assemble terms for second IP.
            Vmath::Vvtvvtp(nqtot, &tmp3[0], 1, &h2[0], 1,
                                  &tmp4[0], 1, &h3[0], 1,
                                  &tmp5[0], 1);
            
            IProductWRTBase_SumFacKernel(m_base[0]->GetBdata (),
                                         m_base[1]->GetDbdata(),
                                         m_base[2]->GetBdata (),
                                         tmp5,tmp6,wsp,
                                         true,true,true);
            Vmath::Vadd(m_ncoeffs, tmp6, 1, outarray, 1, outarray, 1);

            // Do third IP.
            IProductWRTBase_SumFacKernel(m_base[0]->GetBdata (),
                                         m_base[1]->GetBdata (),
                                         m_base[2]->GetDbdata(),
                                         tmp4,tmp6,wsp,
                                         true,true,true);

            // Sum contributions.
            Vmath::Vadd(m_ncoeffs, tmp6, 1, outarray, 1, outarray, 1);
        }


        //-----------------------------
        // Evaluation functions
        //-----------------------------

        /** 
         * Given the local cartesian coordinate \a Lcoord evaluate the
         * value of physvals at this point by calling through to the
         * StdExpansion method
         */
        NekDouble TetExp::v_StdPhysEvaluate(
            const Array<OneD, const NekDouble> &Lcoord,
            const Array<OneD, const NekDouble> &physvals)
        {
            // Evaluate point in local (eta) coordinates.
            return StdTetExp::v_PhysEvaluate(Lcoord,physvals);
        }



        /**
         * @param   coord       Physical space coordinate
         * @returns Evaluation of expansion at given coordinate.
         */
        NekDouble TetExp::v_PhysEvaluate(
                  const Array<OneD, const NekDouble> &coord)
        {
            return PhysEvaluate(coord,m_phys);
        }


        /**
         * @param   coord       Physical space coordinate
         * @returns Evaluation of expansion at given coordinate.
         */
        NekDouble TetExp::v_PhysEvaluate(
                  const Array<OneD, const NekDouble> &coord,
                  const Array<OneD, const NekDouble> & physvals)
        {
            ASSERTL0(m_geom,"m_geom not defined");

            Array<OneD,NekDouble> Lcoord = Array<OneD,NekDouble>(3);

            // Get the local (eta) coordinates of the point
            m_geom->GetLocCoords(coord,Lcoord);

            // Evaluate point in local (eta) coordinates.
            return StdTetExp::v_PhysEvaluate(Lcoord,physvals);
        }


        /** 
	 * \brief Get the x,y,z coordinates of each quadrature point.
	 */
        void TetExp::v_GetCoords(
                  Array<OneD,NekDouble> &coords_0,
                  Array<OneD,NekDouble> &coords_1,
                  Array<OneD,NekDouble> &coords_2)
        {
            LibUtilities::BasisSharedPtr CBasis0;
            LibUtilities::BasisSharedPtr CBasis1;
            LibUtilities::BasisSharedPtr CBasis2;
            Array<OneD,NekDouble>  x;

            ASSERTL0(m_geom, "m_geom not define");

            // get physical points defined in Geom
            m_geom->FillGeom();  //TODO: implement

            switch(m_geom->GetCoordim())
            {
            case 3:
                ASSERTL0(coords_2.num_elements(), "output coords_2 is not defined");
                CBasis0 = m_geom->GetBasis(2,0);
                CBasis1 = m_geom->GetBasis(2,1);
                CBasis2 = m_geom->GetBasis(2,2);

                if((m_base[0]->GetBasisKey().SamePoints(CBasis0->GetBasisKey()))&&
                   (m_base[1]->GetBasisKey().SamePoints(CBasis1->GetBasisKey()))&&
                   (m_base[2]->GetBasisKey().SamePoints(CBasis2->GetBasisKey())))
                {
                    x = m_geom->UpdatePhys(2);
                    Blas::Dcopy(m_base[0]->GetNumPoints()*m_base[1]->GetNumPoints()*m_base[2]->GetNumPoints(),
                                x, 1, coords_2, 1);
                }
                else // Interpolate to Expansion point distribution
                {
                    LibUtilities::Interp3D(CBasis0->GetPointsKey(), CBasis1->GetPointsKey(), CBasis2->GetPointsKey(), &(m_geom->UpdatePhys(2))[0],
                             m_base[0]->GetPointsKey(), m_base[1]->GetPointsKey(), m_base[2]->GetPointsKey(), &coords_2[0]);
                }
            case 2:
                ASSERTL0(coords_1.num_elements(), "output coords_1 is not defined");

                CBasis0 = m_geom->GetBasis(1,0);
                CBasis1 = m_geom->GetBasis(1,1);
                CBasis2 = m_geom->GetBasis(1,2);

                if((m_base[0]->GetBasisKey().SamePoints(CBasis0->GetBasisKey()))&&
                   (m_base[1]->GetBasisKey().SamePoints(CBasis1->GetBasisKey()))&&
                   (m_base[2]->GetBasisKey().SamePoints(CBasis2->GetBasisKey())))
                {
                    x = m_geom->UpdatePhys(1);
                    Blas::Dcopy(m_base[0]->GetNumPoints()*m_base[1]->GetNumPoints()*m_base[2]->GetNumPoints(),
                                x, 1, coords_1, 1);
                }
                else // Interpolate to Expansion point distribution
                {
                    LibUtilities::Interp3D(CBasis0->GetPointsKey(), CBasis1->GetPointsKey(), CBasis2->GetPointsKey(), &(m_geom->UpdatePhys(1))[0],
                             m_base[0]->GetPointsKey(), m_base[1]->GetPointsKey(), m_base[2]->GetPointsKey(), &coords_1[0]);
                }
            case 1:
                ASSERTL0(coords_0.num_elements(), "output coords_0 is not defined");

                CBasis0 = m_geom->GetBasis(0,0);
                CBasis1 = m_geom->GetBasis(0,1);
                CBasis2 = m_geom->GetBasis(0,2);

                if((m_base[0]->GetBasisKey().SamePoints(CBasis0->GetBasisKey()))&&
                   (m_base[1]->GetBasisKey().SamePoints(CBasis1->GetBasisKey()))&&
                   (m_base[2]->GetBasisKey().SamePoints(CBasis2->GetBasisKey())))
                {
                    x = m_geom->UpdatePhys(0);
                    Blas::Dcopy(m_base[0]->GetNumPoints()*m_base[1]->GetNumPoints()*m_base[2]->GetNumPoints(),
                                x, 1, coords_0, 1);
                }
                else // Interpolate to Expansion point distribution
                {
                    LibUtilities::Interp3D(CBasis0->GetPointsKey(), CBasis1->GetPointsKey(), CBasis2->GetPointsKey(), &(m_geom->UpdatePhys(0))[0],
                             m_base[0]->GetPointsKey(),m_base[1]->GetPointsKey(),m_base[2]->GetPointsKey(),&coords_0[0]);
                }
                break;
            default:
                ASSERTL0(false,"Number of dimensions are greater than 3");
                break;
            }
        }


        /**
	 * \brief Get the coordinates "coords" at the local coordinates "Lcoords"
	 */
        void TetExp::v_GetCoord(
                  const Array<OneD, const NekDouble> &Lcoords, 
                        Array<OneD,NekDouble> &coords)
        {
            int  i;

            ASSERTL1(Lcoords[0] <= -1.0 && Lcoords[0] >= 1.0 &&
                     Lcoords[1] <= -1.0 && Lcoords[1] >= 1.0 &&
                     Lcoords[2] <= -1.0 && Lcoords[2] >= 1.0,
                     "Local coordinates are not in region [-1,1]");

            // m_geom->FillGeom(); // TODO: implement FillGeom()

            for(i = 0; i < m_geom->GetCoordim(); ++i)
            {
                coords[i] = m_geom->GetCoord(i,Lcoords);
            }
        }


        //-----------------------------
        // Helper functions
        //-----------------------------
        void TetExp::v_WriteToFile(
                  std::ofstream &outfile, 
                  OutputFormat format, 
                  const bool dumpVar, 
                  std::string var)
        {
            int i,j;
            int nquad0 = m_base[0]->GetNumPoints();
            int nquad1 = m_base[1]->GetNumPoints();
            int nquad2 = m_base[2]->GetNumPoints();
            Array<OneD,NekDouble> coords[3];

            ASSERTL0(m_geom,"m_geom not defined");

            int     coordim  = m_geom->GetCoordim();

            coords[0] = Array<OneD,NekDouble>(nquad0*nquad1*nquad2);
            coords[1] = Array<OneD,NekDouble>(nquad0*nquad1*nquad2);
            coords[2] = Array<OneD,NekDouble>(nquad0*nquad1*nquad2);

            GetCoords(coords[0],coords[1],coords[2]);

            if(format==eTecplot)
            {
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
                    outfile << std::endl << std::endl;
                }

                outfile << "Zone, I=" << nquad0 << ", J=" << nquad1 << ", K=" << nquad2 << ", F=Point" << std::endl;

                for(i = 0; i < nquad0*nquad1*nquad2; ++i)
                {
                    for(j = 0; j < coordim; ++j)
                    {
                        outfile << coords[j][i] << " ";
                    }
                    outfile << std::endl;
                }
            }
            else if (format==eGnuplot)
            {
                for(int k = 0; k < nquad2; ++k)
                {
                    for(int j = 0; j < nquad1; ++j)
                    {
                        for(int i = 0; i < nquad0; ++i)
                        {
                            int n = (k*nquad1 + j)*nquad0 + i;
                            outfile <<  coords[0][n] <<  " " << coords[1][n] << " "
                                    << coords[2][n] << " "
                                    << m_phys[i + nquad0*(j + nquad1*k)] << endl;
                        }
                        outfile << endl;
                    }
                    outfile << endl;
                }
            }
            else
            {
                ASSERTL0(false, "Output routine not implemented for requested type of output");
            }
        }


       /**
        * \brief Return Shape of region, using  ShapeType enum list.
	*/
        LibUtilities::ShapeType TetExp::v_DetShapeType() const
        {
            return LibUtilities::eTetrahedron;
        }

        int TetExp::v_GetCoordim()
        {
            return m_geom->GetCoordim();
        }



        void TetExp::v_ExtractDataToCoeffs(
                const NekDouble *data,
                const std::vector<unsigned int > &nummodes,
                const int mode_offset,
                NekDouble * coeffs)
        {
            int data_order0 = nummodes[mode_offset];
            int fillorder0  = min(m_base[0]->GetNumModes(),data_order0);
            int data_order1 = nummodes[mode_offset+1];
            int order1      = m_base[1]->GetNumModes();
            int fillorder1  = min(order1,data_order1);
            int data_order2 = nummodes[mode_offset+2];
            int order2      = m_base[2]->GetNumModes();
            int fillorder2  = min(order2,data_order2);

            switch(m_base[0]->GetBasisType())
            {
            case LibUtilities::eModified_A:
                {
                    int i,j;
                    int cnt  = 0;
                    int cnt1 = 0;

                    ASSERTL1(m_base[1]->GetBasisType() ==
                             LibUtilities::eModified_B,
                             "Extraction routine not set up for this basis");
                    ASSERTL1(m_base[2]->GetBasisType() ==
                             LibUtilities::eModified_C,
                             "Extraction routine not set up for this basis");

                    Vmath::Zero(m_ncoeffs,coeffs,1);
                    for(j = 0; j < fillorder0; ++j)
                    {
                        for(i = 0; i < fillorder1-j; ++i)
                        {
                            Vmath::Vcopy(fillorder2-j-i, &data[cnt],    1,
                                                         &coeffs[cnt1], 1);
                            cnt  += data_order2-j-i;
                            cnt1 += order2-j-i;
                        }

                        // count out data for j iteration
                        for(i = fillorder1-j; i < data_order1; ++i)
                        {
                            cnt += data_order2-j-i;
                        }

                        for(i = fillorder1-j; i < order1; ++i)
                        {
                            cnt1 += order2-j-i;
                        }

                    }
                }
                break;
            default:
                ASSERTL0(false, "basis is either not set up or not "
                                "hierarchicial");
            }
        }


        StdRegions::Orientation TetExp::v_GetFaceOrient(int face)
        {
            return GetGeom3D()->GetFaceOrient(face);
        }

      
        /**
         * \brief Returns the physical values at the quadrature points of a face
         * Wrapper function to v_GetFacePhysVals
         */
        void TetExp::v_GetTracePhysVals(
            const int                                face,
            const StdRegions::StdExpansionSharedPtr &FaceExp,
            const Array<OneD, const NekDouble>      &inarray,
                  Array<OneD,       NekDouble>      &outarray,
            StdRegions::Orientation                  orient)
        {
            v_GetFacePhysVals(face,FaceExp,inarray,outarray,orient);
        }

        /**
         * \brief Returns the physical values at the quadrature points of a face
         */
        void TetExp::v_GetFacePhysVals(
            const int                                face,
            const StdRegions::StdExpansionSharedPtr &FaceExp,
            const Array<OneD, const NekDouble>      &inarray,
                  Array<OneD,       NekDouble>      &outarray,
            StdRegions::Orientation                  orient)
        {
            int nquad0 = m_base[0]->GetNumPoints();
            int nquad1 = m_base[1]->GetNumPoints();
            int nquad2 = m_base[2]->GetNumPoints();

            Array<OneD,NekDouble> o_tmp (GetFaceNumPoints(face));
            Array<OneD,NekDouble> o_tmp2(FaceExp->GetTotPoints());
            Array<OneD,NekDouble> o_tmp3;
            
            if (orient == StdRegions::eNoOrientation)
            {
                orient = GetFaceOrient(face);
            }
            
            switch(face)
            {
                case 0:
                {
                    //Directions A and B positive
                    Vmath::Vcopy(nquad0*nquad1,inarray.get(),1,o_tmp.get(),1);
                    //interpolate
                    LibUtilities::Interp2D(m_base[0]->GetPointsKey(), m_base[1]->GetPointsKey(), o_tmp.get(),
                                           FaceExp->GetBasis(0)->GetPointsKey(),FaceExp->GetBasis(1)->GetPointsKey(),o_tmp2.get());
                    break;
                }
                case 1:
                {
                    //Direction A and B positive
                    for (int k=0; k<nquad2; k++)
                    {
                        Vmath::Vcopy(nquad0,inarray.get()+(nquad0*nquad1*k),1,o_tmp.get()+(k*nquad0),1);
                    }
                    //interpolate
                    LibUtilities::Interp2D(m_base[0]->GetPointsKey(), m_base[2]->GetPointsKey(), o_tmp.get(),
                                           FaceExp->GetBasis(0)->GetPointsKey(),FaceExp->GetBasis(1)->GetPointsKey(),o_tmp2.get());
                    break;
                }
                case 2:
                {
                    //Directions A and B positive
                    Vmath::Vcopy(nquad1*nquad2,inarray.get()+(nquad0-1),nquad0,o_tmp.get(),1);
                    //interpolate
                    LibUtilities::Interp2D(m_base[1]->GetPointsKey(), m_base[2]->GetPointsKey(), o_tmp.get(),
                                           FaceExp->GetBasis(0)->GetPointsKey(),FaceExp->GetBasis(1)->GetPointsKey(),o_tmp2.get());
                    break;
                }
                case 3:
                {
                    //Directions A and B positive
                    Vmath::Vcopy(nquad1*nquad2,inarray.get(),nquad0,o_tmp.get(),1);
                    //interpolate
                    LibUtilities::Interp2D(m_base[1]->GetPointsKey(), m_base[2]->GetPointsKey(), o_tmp.get(),
                                           FaceExp->GetBasis(0)->GetPointsKey(),FaceExp->GetBasis(1)->GetPointsKey(),o_tmp2.get());
                }
                break;
            default:
                ASSERTL0(false,"face value (> 3) is out of range");
                break;
            }

            int nq1 = FaceExp->GetNumPoints(0);
            int nq2 = FaceExp->GetNumPoints(1);
            
            if ((int)orient == 7)
            {
                for (int j = 0; j < nq2; ++j)
                {
                    Vmath::Vcopy(nq1, o_tmp2.get()+((j+1)*nq1-1), -1, outarray.get()+j*nq1, 1);
                }
            }
            else
            {
                Vmath::Vcopy(nq1*nq2, o_tmp2.get(), 1, outarray.get(), 1);
            }
        }


        /**
	 * \brief Compute the normal of a triangular face
	 */
        void TetExp::v_ComputeFaceNormal(const int face)
        {
            int i;
            const SpatialDomains::GeomFactorsSharedPtr &geomFactors = 
                GetGeom()->GetMetricInfo();
            SpatialDomains::GeomType            type = geomFactors->GetGtype();
            const Array<TwoD, const NekDouble> &df   = geomFactors->GetDerivFactors();
            const Array<OneD, const NekDouble> &jac  = geomFactors->GetJac();

            int nq = m_base[0]->GetNumPoints()*m_base[0]->GetNumPoints();
            int vCoordDim = GetCoordim();
            
            m_faceNormals[face] = Array<OneD, Array<OneD, NekDouble> >(vCoordDim);
            Array<OneD, Array<OneD, NekDouble> > &normal = m_faceNormals[face];
            for (i = 0; i < vCoordDim; ++i)
            {
                normal[i] = Array<OneD, NekDouble>(nq);
            }

            // Regular geometry case
            if (type == SpatialDomains::eRegular ||
                type == SpatialDomains::eMovingRegular)
            {
                NekDouble fac;
                
                // Set up normals
                switch (face)
                {
                    case 0:
                    {
                        for (i = 0; i < vCoordDim; ++i)
                        {
                            Vmath::Fill(nq,-df[3*i+2][0],normal[i],1);
                        }
                        
                        break;
                    }
                    case 1:
                    {
                        for (i = 0; i < vCoordDim; ++i)
                        {
                            Vmath::Fill(nq,-df[3*i+1][0],normal[i],1);
                        }
                        
                        break;
                    }
                    case 2:
                    {
                        for (i = 0; i < vCoordDim; ++i)
                        {
                            Vmath::Fill(nq,df[3*i][0]+df[3*i+1][0]+
                                        df[3*i+2][0],normal[i],1);
                        }
                        
                        break;
                    }
                    case 3:
                    {
                        for(i = 0; i < vCoordDim; ++i)
                        {
                            Vmath::Fill(nq,-df[3*i][0],normal[i],1);
                        }
                        break;
                    }
                    default:
                        ASSERTL0(false,"face is out of range (edge < 3)");
                }
                
                // normalise
                fac = 0.0;
                for (i = 0; i < vCoordDim; ++i)
                {
                    fac += normal[i][0]*normal[i][0];
                }
                fac = 1.0/sqrt(fac);
                for (i = 0; i < vCoordDim; ++i)
                {
                    Vmath::Smul(nq,fac,normal[i],1,normal[i],1);
                }
	    }
            else
            {
                // Set up deformed normals
                int j, k;

                int nq0 = geomFactors->GetPointsKey(0).GetNumPoints();
                int nq1 = geomFactors->GetPointsKey(1).GetNumPoints();
                int nq2 = geomFactors->GetPointsKey(2).GetNumPoints();
                int nqtot;
                int nq01 =nq0*nq1;

                if (face == 0)
                {
                    nqtot = nq01;
                }
                else if (face == 1)
                {
                    nqtot = nq0*nq2;
                }
                else
                {
                    nqtot = nq1*nq2;
                }

                LibUtilities::PointsKey points0;
                LibUtilities::PointsKey points1;

                Array<OneD,NekDouble> work   (nq,              0.0);
                Array<OneD,NekDouble> normals(vCoordDim*nqtot, 0.0);

                // Extract Jacobian along face and recover local derivates
                // (dx/dr) for polynomial interpolation by multiplying m_gmat by
                // jacobian
                switch (face)
	        {
                    case 0:
                    {
                        for(j = 0; j < nq01; ++j)
                        {
                            normals[j]         = -df[2][j]*jac[j];
                            normals[nqtot+j]   = -df[5][j]*jac[j];
                            normals[2*nqtot+j] = -df[8][j]*jac[j];
                        }

                        points0 = geomFactors->GetPointsKey(0);
                        points1 = geomFactors->GetPointsKey(1);
                        break;
                    }

                    case 1:
                    {
                        for (j = 0; j < nq0; ++j)
                        {
                            for(k = 0; k < nq2; ++k)
                            {
                                int tmp = j+nq01*k;
                                normals[j+k*nq0]          =
                                    -df[1][tmp]*jac[tmp];
                                normals[nqtot+j+k*nq0]    =
                                    -df[4][tmp]*jac[tmp];
                                normals[2*nqtot+j+k*nq0]  =
                                    -df[7][tmp]*jac[tmp];
                            } 
                        }

                        points0 = geomFactors->GetPointsKey(0);
                        points1 = geomFactors->GetPointsKey(2);
                        break;
                    }

                    case 2:
                    {
                        for (j = 0; j < nq1; ++j)
                        {
                            for(k = 0; k < nq2; ++k)
                            {
                                int tmp = nq0-1+nq0*j+nq01*k;
                                normals[j+k*nq1]         =
                                    (df[0][tmp]+df[1][tmp]+df[2][tmp])*
                                        jac[tmp];
                                normals[nqtot+j+k*nq1]   =
                                    (df[3][tmp]+df[4][tmp]+df[5][tmp])*
                                        jac[tmp];
                                normals[2*nqtot+j+k*nq1] =
                                    (df[6][tmp]+df[7][tmp]+df[8][tmp])*
                                        jac[tmp];
                            }
                        }

                        points0 = geomFactors->GetPointsKey(1);
                        points1 = geomFactors->GetPointsKey(2);
                        break;
                    }

                    case 3:
                    {
                        for (j = 0; j < nq1; ++j)
                        {
                            for(k = 0; k < nq2; ++k)
                            {
                                int tmp = j*nq0+nq01*k;
                                normals[j+k*nq1]         =
                                    -df[0][tmp]*jac[tmp];
                                normals[nqtot+j+k*nq1]   =
                                    -df[3][tmp]*jac[tmp];
                                normals[2*nqtot+j+k*nq1] =
                                    -df[6][tmp]*jac[tmp];
                            }
                        }

                        points0 = geomFactors->GetPointsKey(1);
                        points1 = geomFactors->GetPointsKey(2);
                        break;
                    }

                    default:
                        ASSERTL0(false,"face is out of range (face < 3)");
                }

                // Interpolate Jacobian and invert
                LibUtilities::Interp2D(points0, points1, jac,
                                       m_base[0]->GetPointsKey(),
                                       m_base[0]->GetPointsKey(),
                                       work);
                Vmath::Sdiv(nq, 1.0, &work[0], 1, &work[0], 1);

                // Interpolate normal and multiply by inverse Jacobian.
                for(i = 0; i < vCoordDim; ++i)
                {
                    LibUtilities::Interp2D(points0, points1,
                                           &normals[i*nqtot],
                                           m_base[0]->GetPointsKey(),
                                           m_base[0]->GetPointsKey(),
                                           &normal[i][0]);
                    Vmath::Vmul(nq,work,1,normal[i],1,normal[i],1);
                }

                // Normalise to obtain unit normals.
                Vmath::Zero(nq,work,1);
                for(i = 0; i < GetCoordim(); ++i)
                {
                    Vmath::Vvtvp(nq,normal[i],1,normal[i],1,work,1,work,1);
                }

                Vmath::Vsqrt(nq,work,1,work,1);
                Vmath::Sdiv (nq,1.0,work,1,work,1);

                for(i = 0; i < GetCoordim(); ++i)
                {
                    Vmath::Vmul(nq,normal[i],1,work,1,normal[i],1);
                }
            }
        }


        NekDouble TetExp::v_Linf(const Array<OneD, const NekDouble> &sol)
        {
            return Linf(sol);
        }


        NekDouble TetExp::v_Linf()
        {
            return Linf();
        }


        NekDouble TetExp::v_L2(const Array<OneD, const NekDouble> &sol)
        {
            return StdExpansion::L2(sol);
        }


        NekDouble TetExp::v_L2()
        {
            return StdExpansion::L2();
        }


        //-----------------------------
        // Operator creation functions
        //-----------------------------
        void TetExp::v_HelmholtzMatrixOp(
                  const Array<OneD, const NekDouble> &inarray,
                        Array<OneD,NekDouble> &outarray,
                  const StdRegions::StdMatrixKey &mkey)
        {
            TetExp::v_HelmholtzMatrixOp_MatFree(inarray,outarray,mkey);
        }


        void TetExp::v_LaplacianMatrixOp(
                  const Array<OneD, const NekDouble> &inarray,
                  Array<OneD,NekDouble> &outarray,
                  const StdRegions::StdMatrixKey &mkey)
        {
            TetExp::v_LaplacianMatrixOp_MatFree(inarray,outarray,mkey);
        }

        void TetExp::v_LaplacianMatrixOp(
                  const int k1, 
                  const int k2,
                  const Array<OneD, const NekDouble> &inarray,
                        Array<OneD,NekDouble> &outarray,
                  const StdRegions::StdMatrixKey &mkey)
        {
            StdExpansion::LaplacianMatrixOp_MatFree(k1,k2,inarray,outarray,
                                                        mkey);
        }


        //-----------------------------
        // Matrix creation functions
        //-----------------------------
        DNekMatSharedPtr TetExp::v_GenMatrix(
                 const StdRegions::StdMatrixKey &mkey)
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
            case StdRegions::eInvLaplacianWithUnityMean:
                returnval = Expansion3D::v_GenMatrix(mkey);
                break;
            default:
                returnval = StdTetExp::v_GenMatrix(mkey);
            }

            return returnval;
        }


        DNekScalMatSharedPtr TetExp::CreateMatrix(const MatrixKey &mkey)
        {
            DNekScalMatSharedPtr returnval;

            ASSERTL2(m_metricinfo->GetGtype() != SpatialDomains::eNoGeomType,"Geometric information is not set up");

            switch(mkey.GetMatrixType())
            {
            case StdRegions::eMass:
                {
                    if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed ||
                            mkey.GetNVarCoeff())
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
                    if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed ||
                            mkey.GetNVarCoeff())
                    {
                        NekDouble one = 1.0;
                        DNekMatSharedPtr mat = GenMatrix(mkey);

                        returnval = MemoryManager<DNekScalMat>
                                                ::AllocateSharedPtr(one,mat);
                    }
                    else
                    {
                        NekDouble jac = (m_metricinfo->GetJac())[0];
                        Array<TwoD, const NekDouble> df
                                                = m_metricinfo->GetDerivFactors();
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
                        MatrixKey deriv2key(StdRegions::eWeakDeriv2,
                                            mkey.GetShapeType(), *this);

                        DNekMat &deriv0 = *GetStdMatrix(deriv0key);
                        DNekMat &deriv1 = *GetStdMatrix(deriv1key);
                        DNekMat &deriv2 = *GetStdMatrix(deriv2key);

                        int rows = deriv0.GetRows();
                        int cols = deriv1.GetColumns();

                        DNekMatSharedPtr WeakDeriv = MemoryManager<DNekMat>
                                                ::AllocateSharedPtr(rows,cols);
                        (*WeakDeriv) = df[3*dir][0]*deriv0
                                     + df[3*dir+1][0]*deriv1
                                     + df[3*dir+2][0]*deriv2;

                        returnval = MemoryManager<DNekScalMat>
                                            ::AllocateSharedPtr(jac,WeakDeriv);
                    }
                }
                break;
            case StdRegions::eLaplacian:
                {
                    if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed ||
                       (mkey.GetNVarCoeff() > 0)||(mkey.ConstFactorExists(StdRegions::eFactorSVVCutoffRatio)))
                    {
                        NekDouble one = 1.0;
                        DNekMatSharedPtr mat = GenMatrix(mkey);
                        
                        returnval = MemoryManager<DNekScalMat>
                                                ::AllocateSharedPtr(one,mat);
                    }
                    else
                    {
                        MatrixKey lap00key(StdRegions::eLaplacian00,
                                           mkey.GetShapeType(), *this);
                        MatrixKey lap01key(StdRegions::eLaplacian01,
                                           mkey.GetShapeType(), *this);
                        MatrixKey lap02key(StdRegions::eLaplacian02,
                                           mkey.GetShapeType(), *this);
                        MatrixKey lap11key(StdRegions::eLaplacian11,
                                           mkey.GetShapeType(), *this);
                        MatrixKey lap12key(StdRegions::eLaplacian12,
                                           mkey.GetShapeType(), *this);
                        MatrixKey lap22key(StdRegions::eLaplacian22,
                                           mkey.GetShapeType(), *this);

                        DNekMat &lap00 = *GetStdMatrix(lap00key);
                        DNekMat &lap01 = *GetStdMatrix(lap01key);
                        DNekMat &lap02 = *GetStdMatrix(lap02key);
                        DNekMat &lap11 = *GetStdMatrix(lap11key);
                        DNekMat &lap12 = *GetStdMatrix(lap12key);
                        DNekMat &lap22 = *GetStdMatrix(lap22key);

                        NekDouble jac = (m_metricinfo->GetJac())[0];
                        Array<TwoD, const NekDouble> gmat
                                                    = m_metricinfo->GetGmat();

                        int rows = lap00.GetRows();
                        int cols = lap00.GetColumns();

                        DNekMatSharedPtr lap = MemoryManager<DNekMat>
                                                ::AllocateSharedPtr(rows,cols);

                        (*lap)  = gmat[0][0]*lap00
                                + gmat[4][0]*lap11
                                + gmat[8][0]*lap22
                                + gmat[3][0]*(lap01 + Transpose(lap01))
                                + gmat[6][0]*(lap02 + Transpose(lap02))
                                + gmat[7][0]*(lap12 + Transpose(lap12));

                        returnval = MemoryManager<DNekScalMat>
                                                ::AllocateSharedPtr(jac,lap);
                    }
                }
                break;
            case StdRegions::eHelmholtz:
                {
                    NekDouble factor = mkey.GetConstFactor(StdRegions::eFactorLambda);
                    MatrixKey masskey(StdRegions::eMass, mkey.GetShapeType(), *this);
                    DNekScalMat &MassMat = *(this->m_matrixManager[masskey]);
                    MatrixKey lapkey(StdRegions::eLaplacian, mkey.GetShapeType(), *this, mkey.GetConstFactors(), mkey.GetVarCoeffs());
                    DNekScalMat &LapMat = *(this->m_matrixManager[lapkey]);

                    int rows = LapMat.GetRows();
                    int cols = LapMat.GetColumns();

                    DNekMatSharedPtr helm = MemoryManager<DNekMat>::AllocateSharedPtr(rows, cols);

                    NekDouble one = 1.0;
                    (*helm) = LapMat + factor*MassMat;

                    returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one, helm);
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
            case StdRegions::eHybridDGHelmholtz:
            case StdRegions::eHybridDGLamToU:
            case StdRegions::eHybridDGLamToQ0:
            case StdRegions::eHybridDGLamToQ1:
            case StdRegions::eHybridDGLamToQ2:
            case StdRegions::eHybridDGHelmBndLam:
            case StdRegions::eInvLaplacianWithUnityMean:
                {
                    NekDouble one    = 1.0;
                    
                    DNekMatSharedPtr mat = GenMatrix(mkey);
                    returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,mat);
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
            case StdRegions::ePreconR:
                {
                    NekDouble one = 1.0;
                    MatrixKey helmkey(StdRegions::eHelmholtz, mkey.GetShapeType(), *this,mkey.GetConstFactors(), mkey.GetVarCoeffs());
                    DNekScalBlkMatSharedPtr helmStatCond = GetLocStaticCondMatrix(helmkey);
                    DNekScalMatSharedPtr A =helmStatCond->GetBlock(0,0);

                    DNekScalMatSharedPtr Atmp;
                    DNekMatSharedPtr R=BuildTransformationMatrix(A,mkey.GetMatrixType());

                    returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,R);
                }
                break;
            case StdRegions::ePreconRT:
                {
                    NekDouble one = 1.0;
                    MatrixKey helmkey(StdRegions::eHelmholtz, mkey.GetShapeType(), *this,mkey.GetConstFactors(), mkey.GetVarCoeffs());
                    DNekScalBlkMatSharedPtr helmStatCond = GetLocStaticCondMatrix(helmkey);
                    DNekScalMatSharedPtr A =helmStatCond->GetBlock(0,0);

                    DNekScalMatSharedPtr Atmp;
                    DNekMatSharedPtr RT=BuildTransformationMatrix(A,mkey.GetMatrixType());

                    returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,RT);
                }
                break;
            default:
                {
                    //ASSERTL0(false, "Missing definition for " + (*StdRegions::MatrixTypeMap[mkey.GetMatrixType()]));
                    NekDouble        one = 1.0;
                    DNekMatSharedPtr mat = GenMatrix(mkey);
                    
                    returnval = MemoryManager<DNekScalMat>::AllocateSharedPtr(one,mat);
                }
                break;
            }

            return returnval;
        }


        DNekScalBlkMatSharedPtr TetExp::CreateStaticCondMatrix(
                 const MatrixKey &mkey)
        {
            DNekScalBlkMatSharedPtr returnval;

            ASSERTL2(m_metricinfo->GetGtype() != SpatialDomains::eNoGeomType,"Geometric information is not set up");

            // set up block matrix system
            unsigned int nbdry = NumBndryCoeffs();
            unsigned int nint = (unsigned int)(m_ncoeffs - nbdry);
            unsigned int exp_size[] = {nbdry, nint};
            unsigned int nblks = 2;
            returnval = MemoryManager<DNekScalBlkMat>::AllocateSharedPtr(nblks, nblks, exp_size, exp_size);

            NekDouble factor = 1.0;
            MatrixStorage AMatStorage = eFULL;
            
            switch(mkey.GetMatrixType())
            {
            case StdRegions::eLaplacian:
            case StdRegions::eHelmholtz: // special case since Helmholtz not defined in StdRegions
                // use Deformed case for both regular and deformed geometries
                factor = 1.0;
                goto UseLocRegionsMatrix;
                break;
            default:
                if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed ||
                        mkey.GetNVarCoeff())
                {
                    factor = 1.0;
                    goto UseLocRegionsMatrix;
                }
                else
                {
                    DNekScalMatSharedPtr mat = GetLocMatrix(mkey);
                    factor = mat->Scale();
                    goto UseStdRegionsMatrix;
                }
                break;
            UseStdRegionsMatrix:
                {
                    NekDouble            invfactor = 1.0/factor;
                    NekDouble            one = 1.0;
                    DNekBlkMatSharedPtr  mat = GetStdStaticCondMatrix(mkey);
                    DNekScalMatSharedPtr Atmp;
                    DNekMatSharedPtr     Asubmat;

                    //TODO: check below
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
                    DNekMatSharedPtr A = MemoryManager<DNekMat>::AllocateSharedPtr(nbdry,nbdry,AMatStorage);
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
                break;
            }
            return returnval;
        }


        DNekMatSharedPtr TetExp::v_CreateStdMatrix(
                 const StdRegions::StdMatrixKey &mkey)
        {
            LibUtilities::BasisKey bkey0 = m_base[0]->GetBasisKey();
            LibUtilities::BasisKey bkey1 = m_base[1]->GetBasisKey();
            LibUtilities::BasisKey bkey2 = m_base[2]->GetBasisKey();
            StdRegions::StdTetExpSharedPtr tmp = MemoryManager<StdTetExp>::AllocateSharedPtr(bkey0, bkey1, bkey2);

            return tmp->GetStdMatrix(mkey);
        }

        DNekScalMatSharedPtr TetExp::v_GetLocMatrix(const MatrixKey &mkey)
        {
            return m_matrixManager[mkey];
        }

        DNekScalBlkMatSharedPtr TetExp::v_GetLocStaticCondMatrix(const MatrixKey &mkey)
        {
            return m_staticCondMatrixManager[mkey];
        }

        void TetExp::v_DropLocStaticCondMatrix(const MatrixKey &mkey)
        {
            m_staticCondMatrixManager.DeleteObject(mkey);
        }

        void TetExp::GeneralMatrixOp_MatOp(
                            const Array<OneD, const NekDouble> &inarray,
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


        void TetExp::v_LaplacianMatrixOp_MatFree_Kernel(
            const Array<OneD, const NekDouble> &inarray,
                  Array<OneD,       NekDouble> &outarray,
                  Array<OneD,       NekDouble> &wsp)
        {
            // This implementation is only valid when there are no
            // coefficients associated to the Laplacian operator
            if (m_metrics.count(MetricLaplacian00) == 0)
            {
                ComputeLaplacianMetric();
            }

            int nquad0  = m_base[0]->GetNumPoints();
            int nquad1  = m_base[1]->GetNumPoints();
            int nquad2  = m_base[2]->GetNumPoints();
            int nqtot   = nquad0*nquad1*nquad2;

            ASSERTL1(wsp.num_elements() >= 6*nqtot,
                     "Insufficient workspace size.");

            const Array<OneD, const NekDouble>& base0  = m_base[0]->GetBdata();
            const Array<OneD, const NekDouble>& base1  = m_base[1]->GetBdata();
            const Array<OneD, const NekDouble>& base2  = m_base[2]->GetBdata();
            const Array<OneD, const NekDouble>& dbase0 = m_base[0]->GetDbdata();
            const Array<OneD, const NekDouble>& dbase1 = m_base[1]->GetDbdata();
            const Array<OneD, const NekDouble>& dbase2 = m_base[2]->GetDbdata();
            const Array<OneD, const NekDouble>& metric00 = m_metrics[MetricLaplacian00];
            const Array<OneD, const NekDouble>& metric01 = m_metrics[MetricLaplacian01];
            const Array<OneD, const NekDouble>& metric02 = m_metrics[MetricLaplacian02];
            const Array<OneD, const NekDouble>& metric11 = m_metrics[MetricLaplacian11];
            const Array<OneD, const NekDouble>& metric12 = m_metrics[MetricLaplacian12];
            const Array<OneD, const NekDouble>& metric22 = m_metrics[MetricLaplacian22];

            // Allocate temporary storage
            Array<OneD,NekDouble> wsp0 (wsp);
            Array<OneD,NekDouble> wsp1 (wsp+1*nqtot   );// TensorDeriv 1
            Array<OneD,NekDouble> wsp2 (wsp+2*nqtot);// TensorDeriv 2
            Array<OneD,NekDouble> wsp3 (wsp+3*nqtot);// TensorDeriv 3
            Array<OneD,NekDouble> wsp4 (wsp+4*nqtot);// wsp4 == g1
            Array<OneD,NekDouble> wsp5 (wsp+5*nqtot);// wsp5 == g2
            
            // LAPLACIAN MATRIX OPERATION
            // wsp1 = du_dxi1 = D_xi1 * inarray = D_xi1 * u
            // wsp2 = du_dxi2 = D_xi2 * inarray = D_xi2 * u
            // wsp2 = du_dxi3 = D_xi3 * inarray = D_xi3 * u
            StdExpansion3D::PhysTensorDeriv(inarray,wsp0,wsp1,wsp2);

            // wsp0 = k = g0 * wsp1 + g1 * wsp2 = g0 * du_dxi1 + g1 * du_dxi2
            // wsp2 = l = g1 * wsp1 + g2 * wsp2 = g0 * du_dxi1 + g1 * du_dxi2
            // where g0, g1 and g2 are the metric terms set up in the GeomFactors class
            // especially for this purpose
            Vmath::Vvtvvtp(nqtot,&metric00[0],1,&wsp0[0],1,&metric01[0],1,&wsp1[0],1,&wsp3[0],1);
            Vmath::Vvtvp  (nqtot,&metric02[0],1,&wsp2[0],1,&wsp3[0],1,&wsp3[0],1);
            Vmath::Vvtvvtp(nqtot,&metric01[0],1,&wsp0[0],1,&metric11[0],1,&wsp1[0],1,&wsp4[0],1);
            Vmath::Vvtvp  (nqtot,&metric12[0],1,&wsp2[0],1,&wsp4[0],1,&wsp4[0],1);
            Vmath::Vvtvvtp(nqtot,&metric02[0],1,&wsp0[0],1,&metric12[0],1,&wsp1[0],1,&wsp5[0],1);
            Vmath::Vvtvp  (nqtot,&metric22[0],1,&wsp2[0],1,&wsp5[0],1,&wsp5[0],1);

            // outarray = m = (D_xi1 * B)^T * k
            // wsp1     = n = (D_xi2 * B)^T * l
            IProductWRTBase_SumFacKernel(dbase0,base1,base2,wsp3,outarray,wsp0,false,true,true);
            IProductWRTBase_SumFacKernel(base0,dbase1,base2,wsp4,wsp1,    wsp0,true,false,true);
            IProductWRTBase_SumFacKernel(base0,base1,dbase2,wsp5,wsp2,    wsp0,true,true,false);

            // outarray = outarray + wsp1
            //          = L * u_hat
            Vmath::Vadd(m_ncoeffs,wsp1.get(),1,outarray.get(),1,outarray.get(),1);
            Vmath::Vadd(m_ncoeffs,wsp2.get(),1,outarray.get(),1,outarray.get(),1);
        }


        void TetExp::v_ComputeLaplacianMetric()
        {
            if (m_metrics.count(MetricQuadrature) == 0)
            {
                ComputeQuadratureMetric();
            }

            int i, j;
            const unsigned int nqtot = GetTotPoints();
            const unsigned int dim = 3;
            const MetricType m[3][3] = { {MetricLaplacian00, MetricLaplacian01, MetricLaplacian02},
                                       {MetricLaplacian01, MetricLaplacian11, MetricLaplacian12},
                                       {MetricLaplacian02, MetricLaplacian12, MetricLaplacian22}
            };

            for (unsigned int i = 0; i < dim; ++i)
            {
                for (unsigned int j = i; j < dim; ++j)
                {
                    m_metrics[m[i][j]] = Array<OneD, NekDouble>(nqtot);
                }
            }

            // Allocate temporary storage
            Array<OneD,NekDouble> alloc(13*nqtot,0.0);
            Array<OneD,NekDouble> wsp1 (alloc         );// TensorDeriv 1
            Array<OneD,NekDouble> wsp2 (alloc+ 1*nqtot);// TensorDeriv 2
            Array<OneD,NekDouble> wsp3 (alloc+ 2*nqtot);// TensorDeriv 3
            Array<OneD,NekDouble> g0   (alloc+ 3*nqtot);// g0
            Array<OneD,NekDouble> g1   (alloc+ 4*nqtot);// g1
            Array<OneD,NekDouble> g2   (alloc+ 5*nqtot);// g2
            Array<OneD,NekDouble> g3   (alloc+ 6*nqtot);// g3
            Array<OneD,NekDouble> g4   (alloc+ 7*nqtot);// g4
            Array<OneD,NekDouble> g5   (alloc+ 8*nqtot);// g5
            Array<OneD,NekDouble> h0   (alloc+ 9*nqtot);// h0
            Array<OneD,NekDouble> h1   (alloc+10*nqtot);// h1
            Array<OneD,NekDouble> h2   (alloc+11*nqtot);// h2
            Array<OneD,NekDouble> h3   (alloc+12*nqtot);// h3
            // Reuse some of the storage as workspace
            Array<OneD,NekDouble> wsp4 (alloc+ 4*nqtot);// wsp4 == g1
            Array<OneD,NekDouble> wsp5 (alloc+ 5*nqtot);// wsp5 == g2
            Array<OneD,NekDouble> wsp6 (alloc+ 8*nqtot);// wsp6 == g5
            Array<OneD,NekDouble> wsp7 (alloc+ 9*nqtot);// wsp7 == h0
            Array<OneD,NekDouble> wsp8 (alloc+10*nqtot);// wsp8 == h1
            Array<OneD,NekDouble> wsp9 (alloc+11*nqtot);// wsp9 == h2

            const Array<TwoD, const NekDouble>& df = m_metricinfo->GetDerivFactors();
            const Array<OneD, const NekDouble>& z0 = m_base[0]->GetZ();
            const Array<OneD, const NekDouble>& z1 = m_base[1]->GetZ();
            const Array<OneD, const NekDouble>& z2 = m_base[2]->GetZ();
            const unsigned int nquad0 = m_base[0]->GetNumPoints();
            const unsigned int nquad1 = m_base[1]->GetNumPoints();
            const unsigned int nquad2 = m_base[2]->GetNumPoints();

            for(j = 0; j < nquad2; ++j)
            {
                for(i = 0; i < nquad1; ++i)
                {
                    Vmath::Fill(nquad0, 4.0/(1.0-z1[i])/(1.0-z2[j]), &h0[0]+i*nquad0 + j*nquad0*nquad1,1);
                    Vmath::Fill(nquad0, 2.0/(1.0-z1[i])/(1.0-z2[j]), &h1[0]+i*nquad0 + j*nquad0*nquad1,1);
                    Vmath::Fill(nquad0, 2.0/(1.0-z2[j]),             &h2[0]+i*nquad0 + j*nquad0*nquad1,1);
                    Vmath::Fill(nquad0, (1.0+z1[i])/(1.0-z2[j]),     &h3[0]+i*nquad0 + j*nquad0*nquad1,1);
                }
            }
            for(i = 0; i < nquad0; i++)
            {
                Blas::Dscal(nquad1*nquad2, 1+z0[i], &h1[0]+i, nquad0);
            }

            // Step 3. Construct combined metric terms for physical space to
            // collapsed coordinate system.
            // Order of construction optimised to minimise temporary storage
            if(m_metricinfo->GetGtype() == SpatialDomains::eDeformed)
            {
                // wsp4
                Vmath::Vadd(nqtot, &df[1][0], 1, &df[2][0], 1, &wsp4[0], 1);
                Vmath::Vvtvvtp(nqtot, &df[0][0], 1, &h0[0], 1, &wsp4[0], 1, &h1[0], 1, &wsp4[0], 1);
                // wsp5
                Vmath::Vadd(nqtot, &df[4][0], 1, &df[5][0], 1, &wsp5[0], 1);
                Vmath::Vvtvvtp(nqtot, &df[3][0], 1, &h0[0], 1, &wsp5[0], 1, &h1[0], 1, &wsp5[0], 1);
                // wsp6
                Vmath::Vadd(nqtot, &df[7][0], 1, &df[8][0], 1, &wsp6[0], 1);
                Vmath::Vvtvvtp(nqtot, &df[6][0], 1, &h0[0], 1, &wsp6[0], 1, &h1[0], 1, &wsp6[0], 1);

                // g0
                Vmath::Vvtvvtp(nqtot, &wsp4[0], 1, &wsp4[0], 1, &wsp5[0], 1, &wsp5[0], 1, &g0[0], 1);
                Vmath::Vvtvp  (nqtot, &wsp6[0], 1, &wsp6[0], 1, &g0[0],   1, &g0[0],   1);

                // g4
                Vmath::Vvtvvtp(nqtot, &df[2][0], 1, &wsp4[0], 1, &df[5][0], 1, &wsp5[0], 1, &g4[0], 1);
                Vmath::Vvtvp  (nqtot, &df[8][0], 1, &wsp6[0], 1, &g4[0], 1, &g4[0], 1);

                // overwrite h0, h1, h2
                // wsp7 (h2f1 + h3f2)
                Vmath::Vvtvvtp(nqtot, &df[1][0], 1, &h2[0], 1, &df[2][0], 1, &h3[0], 1, &wsp7[0], 1);
                // wsp8 (h2f4 + h3f5)
                Vmath::Vvtvvtp(nqtot, &df[4][0], 1, &h2[0], 1, &df[5][0], 1, &h3[0], 1, &wsp8[0], 1);
                // wsp9 (h2f7 + h3f8)
                Vmath::Vvtvvtp(nqtot, &df[7][0], 1, &h2[0], 1, &df[8][0], 1, &h3[0], 1, &wsp9[0], 1);

                // g3
                Vmath::Vvtvvtp(nqtot, &wsp4[0], 1, &wsp7[0], 1, &wsp5[0], 1, &wsp8[0], 1, &g3[0], 1);
                Vmath::Vvtvp  (nqtot, &wsp6[0], 1, &wsp9[0], 1, &g3[0],   1, &g3[0],   1);

                // overwrite wsp4, wsp5, wsp6
                // g1
                Vmath::Vvtvvtp(nqtot, &wsp7[0], 1, &wsp7[0], 1, &wsp8[0], 1, &wsp8[0], 1, &g1[0], 1);
                Vmath::Vvtvp  (nqtot, &wsp9[0], 1, &wsp9[0], 1, &g1[0],   1, &g1[0],   1);

                // g5
                Vmath::Vvtvvtp(nqtot, &df[2][0], 1, &wsp7[0], 1, &df[5][0], 1, &wsp8[0], 1, &g5[0], 1);
                Vmath::Vvtvp  (nqtot, &df[8][0], 1, &wsp9[0], 1, &g5[0], 1, &g5[0], 1);

                // g2
                Vmath::Vvtvvtp(nqtot, &df[2][0], 1, &df[2][0], 1, &df[5][0], 1, &df[5][0], 1, &g2[0], 1);
                Vmath::Vvtvp  (nqtot, &df[8][0], 1, &df[8][0], 1, &g2[0],      1, &g2[0],      1);
            }
            else
            {
                // wsp4
                Vmath::Svtsvtp(nqtot, df[0][0], &h0[0], 1, df[1][0] + df[2][0], &h1[0], 1, &wsp4[0], 1);
                // wsp5
                Vmath::Svtsvtp(nqtot, df[3][0], &h0[0], 1, df[4][0] + df[5][0], &h1[0], 1, &wsp5[0], 1);
                // wsp6
                Vmath::Svtsvtp(nqtot, df[6][0], &h0[0], 1, df[7][0] + df[8][0], &h1[0], 1, &wsp6[0], 1);

                // g0
                Vmath::Vvtvvtp(nqtot, &wsp4[0], 1, &wsp4[0], 1, &wsp5[0], 1, &wsp5[0], 1, &g0[0], 1);
                Vmath::Vvtvp  (nqtot, &wsp6[0], 1, &wsp6[0], 1, &g0[0],   1, &g0[0],   1);

                // g4
                Vmath::Svtsvtp(nqtot, df[2][0], &wsp4[0], 1, df[5][0], &wsp5[0], 1, &g4[0], 1);
                Vmath::Svtvp  (nqtot, df[8][0], &wsp6[0], 1, &g4[0], 1, &g4[0], 1);

                // overwrite h0, h1, h2
                // wsp7 (h2f1 + h3f2)
                Vmath::Svtsvtp(nqtot, df[1][0], &h2[0], 1, df[2][0], &h3[0], 1, &wsp7[0], 1);
                // wsp8 (h2f4 + h3f5)
                Vmath::Svtsvtp(nqtot, df[4][0], &h2[0], 1, df[5][0], &h3[0], 1, &wsp8[0], 1);
                // wsp9 (h2f7 + h3f8)
                Vmath::Svtsvtp(nqtot, df[7][0], &h2[0], 1, df[8][0], &h3[0], 1, &wsp9[0], 1);

                // g3
                Vmath::Vvtvvtp(nqtot, &wsp4[0], 1, &wsp7[0], 1, &wsp5[0], 1, &wsp8[0], 1, &g3[0], 1);
                Vmath::Vvtvp  (nqtot, &wsp6[0], 1, &wsp9[0], 1, &g3[0],   1, &g3[0],   1);

                // overwrite wsp4, wsp5, wsp6
                // g1
                Vmath::Vvtvvtp(nqtot, &wsp7[0], 1, &wsp7[0], 1, &wsp8[0], 1, &wsp8[0], 1, &g1[0], 1);
                Vmath::Vvtvp  (nqtot, &wsp9[0], 1, &wsp9[0], 1, &g1[0],   1, &g1[0],   1);

                // g5
                Vmath::Svtsvtp(nqtot, df[2][0], &wsp7[0], 1, df[5][0], &wsp8[0], 1, &g5[0], 1);
                Vmath::Svtvp  (nqtot, df[8][0], &wsp9[0], 1, &g5[0], 1, &g5[0], 1);

                // g2
                Vmath::Fill(nqtot, df[2][0]*df[2][0] + df[5][0]*df[5][0] + df[8][0]*df[8][0], &g2[0], 1);
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
    }//end of namespace
}//end of namespace
