///////////////////////////////////////////////////////////////////////////////
//
// File DriverModifiedArnoldi.cpp
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
// Description: Driver for eigenvalue analysis using the modified Arnoldi
//              method.
//
///////////////////////////////////////////////////////////////////////////////

#include <iomanip>

#include <Auxiliary/DriverModifiedArnoldi.h>

namespace Nektar

{
	string DriverModifiedArnoldi::className = GetDriverFactory().RegisterCreatorFunction("ModifiedArnoldi", DriverModifiedArnoldi::create);
 
	/**
	 *
	 */
	DriverModifiedArnoldi::DriverModifiedArnoldi(
			LibUtilities::SessionReaderSharedPtr        pSession)
        : DriverArnoldi(pSession)
	{
	}
	

    /**
     *
     */
	DriverModifiedArnoldi::~DriverModifiedArnoldi()
	{
	}
	

	/**
	 *
	 */
	void DriverModifiedArnoldi::v_InitObject()
	{
            try
            {
                ASSERTL0(m_session->DefinesSolverInfo("EqType"),
                     "EqType SolverInfo tag must be defined.");
                std::string vEquation = m_session->GetSolverInfo("EqType");
                if (m_session->DefinesSolverInfo("SolverType"))
                {
                    vEquation = m_session->GetSolverInfo("SolverType");
                }
                ASSERTL0(GetEquationSystemFactory().ModuleExists(vEquation),
                         "EquationSystem '" + vEquation + "' is not defined.\n"
                         "Ensure equation name is correct and module is compiled.\n");

                m_session->SetTag("AdvectiveType","Linearised");
                
                m_equ = Array<OneD, EquationSystemSharedPtr>(1);
                m_equ[0] = GetEquationSystemFactory().CreateInstance(vEquation, m_comm, m_session);
            }
            catch (int e)
            {
                ASSERTL0(e == -1, "No such class class defined.");
                cout << "An error occured during driver initialisation." << endl;
            }
            
            m_session->MatchSolverInfo("SolverType","VelocityCorrectionScheme",m_TimeSteppingAlgorithm, false);

            
            m_nfields   = m_equ[0]->UpdateFields().num_elements();

            if(m_TimeSteppingAlgorithm)
            {
                m_period = m_session->GetParameter("TimeStep")* m_session->GetParameter("NumSteps");
            }
            else
            {
                m_period = 1.0;
                ASSERTL0(m_session->DefinesFunction("BodyForce"),"A BodyForce section needs to be defined for this solver type");
                m_forces = m_equ[0]->UpdateForces();
            }
            
            m_session->LoadParameter("kdim",  m_kdim,  8);
            m_session->LoadParameter("nvec",  m_nvec,  1);
            m_session->LoadParameter("nits",  m_nits,  500);
            m_session->LoadParameter("evtol", m_evtol, 1e-06);
	}
    

	/**
	 *
	 */
	void DriverModifiedArnoldi::v_Execute()
	{
            int nq                  = m_equ[0]->UpdateFields()[0]->GetNpoints();
            int ntot                = m_nfields*nq;
            int converged           = 0;
            NekDouble resnorm       = 0.0;
            std::string evlFile     = m_session->GetFilename().substr(0,m_session->GetFilename().find_last_of('.')) + ".evl";
            ofstream evlout(evlFile.c_str());
            
            // Allocate memory
            Array<OneD, NekDouble> alpha     = Array<OneD, NekDouble>  (m_kdim + 1,      0.0);
            Array<OneD, NekDouble> wr        = Array<OneD, NekDouble>  (m_kdim,          0.0);
            Array<OneD, NekDouble> wi        = Array<OneD, NekDouble>  (m_kdim,          0.0);
            Array<OneD, NekDouble> zvec      = Array<OneD, NekDouble>  (m_kdim * m_kdim, 0.0);
            
            Array<OneD, Array<OneD, NekDouble> > Kseq
                = Array<OneD, Array<OneD, NekDouble> > (m_kdim + 1);
            Array<OneD, Array<OneD, NekDouble> > Tseq
                = Array<OneD, Array<OneD, NekDouble> > (m_kdim + 1);
            for (int i = 0; i < m_kdim + 1; ++i)
            {
                Kseq[i] = Array<OneD, NekDouble>(ntot, 0.0);
                Tseq[i] = Array<OneD, NekDouble>(ntot, 0.0);
            }
            
            m_equ[0]->PrintSummary(cout);
            
            // Print session parameters
            cout << "\tArnoldi solver type   : Modified Arnold" << endl;
            cout << "\tKrylov-space dimension: " << m_kdim << endl;
            cout << "\tNumber of vectors:      " << m_nvec << endl;
            cout << "\tMax iterations:         " << m_nits << endl;
            cout << "\tEigenvalue tolerance:   " << m_evtol << endl;
            cout << "=======================================================================" << endl;
            
            m_equ[0]->DoInitialise();

            // Copy starting vector into second sequence element (temporary).
            v_CopyFieldToArnoldiArray(Kseq[1]);
            
            // Perform one iteration to enforce boundary conditions.
            // Set this as the initial value in the sequence.
            EV_update(Kseq[1], Kseq[0]);
            cout << "Iteration: " << 0 <<  endl;
            
            // Normalise first vector in sequence
            alpha[0] = std::sqrt(Vmath::Dot(ntot, &Kseq[0][0], 1, &Kseq[0][0], 1));
            alpha[0] = std::sqrt(alpha[0]);
            Vmath::Smul(ntot, 1.0/alpha[0], Kseq[0], 1, Kseq[0], 1);
            
        // Fill initial krylov sequence
            NekDouble resid0;
            for (int i = 1; !converged && i <= m_kdim; ++i)
            {
                // Compute next vector
                EV_update(Kseq[i-1], Kseq[i]);
                
                // Normalise
                alpha[i] = std::sqrt(Vmath::Dot(ntot, &Kseq[i][0], 1, &Kseq[i][0], 1));
                alpha[i] = std::sqrt(alpha[i]);
                Vmath::Smul(ntot, 1.0/alpha[i], Kseq[i], 1, Kseq[i], 1);
                
                // Copy Krylov sequence into temporary storage
                for (int k = 0; k < i + 1; ++k)
                {
                    Vmath::Vcopy(ntot, Kseq[k], 1, Tseq[k], 1);
                }
                
                // Generate Hessenberg matrix and compute eigenvalues of it.
                EV_small(Tseq, ntot, alpha, i, zvec, wr, wi, resnorm);
                
                // Test for convergence.
                converged = EV_test(i,i,zvec,wr,wi,resnorm,std::min(i,m_nvec),evlout,resid0);
                cout << "Iteration: " <<  i <<  " (residual : " << resid0 << ")" <<endl;
            }
            
            // Continue with full sequence
            for (int i = m_kdim + 1; !converged && i <= m_nits; ++i)
            {
                // Shift all the vectors in the sequence.
                // First vector is removed.
                NekDouble invnorm = 1.0/sqrt(Blas::Ddot(ntot,Kseq[1],1,Kseq[1],1));
                for (int j = 1; j <= m_kdim; ++j)
                {
                    alpha[j-1] = alpha[j];
                    Vmath::Smul(ntot,invnorm,Kseq[j],1,Kseq[j],1);
                    Vmath::Vcopy(ntot, Kseq[j], 1, Kseq[j-1], 1);
                }
                
                // Compute next vector
                EV_update(Kseq[m_kdim - 1], Kseq[m_kdim]);
                
                // Compute new scale factor
                alpha[m_kdim] = std::sqrt(Vmath::Dot(ntot, &Kseq[m_kdim][0], 1, &Kseq[m_kdim][0], 1));
                alpha[m_kdim] = std::sqrt(alpha[m_kdim]);
                Vmath::Smul(ntot, 1.0/alpha[m_kdim], Kseq[m_kdim], 1, Kseq[m_kdim], 1);
                
                // Copy Krylov sequence into temporary storage
                for (int k = 0; k < m_kdim + 1; ++k)
                {
                    Vmath::Vcopy(ntot, Kseq[k], 1, Tseq[k], 1);
                }
                
                // Generate Hessenberg matrix and compute eigenvalues of it
                EV_small(Tseq, ntot, alpha, m_kdim, zvec, wr, wi, resnorm);
                
                // Test for convergence.
                converged = EV_test(i,m_kdim,zvec,wr,wi,resnorm,m_nvec,evlout,resid0);
                cout << "Iteration: " <<  i <<  " (residual : " << resid0 << ")" <<endl;
        }
            
            // Close the runtime info file.
            evlout.close();
            
            m_equ[0]->Output();
            
            // Evaluate and output computation time and solution accuracy.
            // The specific format of the error output is essential for the
            // regression tests to work.
            // Evaluate L2 Error
            for(int i = 0; i < m_equ[0]->GetNvariables(); ++i)
            {
                NekDouble vL2Error = m_equ[0]->L2Error(i,false);
                NekDouble vLinfError = m_equ[0]->LinfError(i);
                if (m_comm->GetRank() == 0)
                {
                    cout << "L 2 error (variable " << m_equ[0]->GetVariable(i) << ") : " << vL2Error << endl;
                    cout << "L inf error (variable " << m_equ[0]->GetVariable(i) << ") : " << vLinfError << endl;
                }
            }
	}
    
    
    void DriverModifiedArnoldi::OutputEv(FILE* pFile, const int nev, Array<OneD, NekDouble> &workl, int* ipntr, NekDouble period, bool TimeSteppingAlgorithm)
    {
        int k;
	
        //Plotting of real and imaginary part of the
        //eigenvalues from workl
        for(int k=0; k<=nev-1; ++k)
        {                
            double r = workl[ipntr[5]-1+k];
            double i = workl[ipntr[6]-1+k];
            double res;
            
            if(TimeSteppingAlgorithm)
            {
                cout << k << ": Mag " << sqrt(r*r+i*i) << ", angle " << atan2(i,r) << " growth " << log(sqrt(r*r+i*i))/period << 
                    " Frequency " << atan2(i,r)/period << endl;
                
                fprintf (pFile, "EV: %i\t , Mag: %f\t, angle:  %f\t, growth:  %f\t, Frequency:  %f\t \n",k, sqrt(r*r+i*i), atan2(i,r),log(sqrt(r*r+i*i))/period, atan2(i,r)/period );
            }
            else
            {
                NekDouble invmag = 1.0/(r*r + i*i);
                cout << k << ": Re " << sqrt(r*r+i*i) << ", Imag " << atan2(i,r) << " inverse real " << -r*invmag <<  " inverse imag " << i*invmag << endl;
		
                fprintf (pFile, "EV: %i\t , Re: %f\t, Imag:  %f\t, inverse real:  %f\t, inverse imag:  %f\t \n",k, sqrt(r*r+i*i), atan2(i,r),-r*invmag, i*invmag);
            }
        }
    }
    
    void DriverModifiedArnoldi::EV_update(
                                          Array<OneD, NekDouble> &src,
                                          Array<OneD, NekDouble> &tgt)
    {
        // Copy starting vector into first sequence element.
        v_CopyArnoldiArrayToField(src);

        m_equ[0]->DoSolve();

        // Copy starting vector into first sequence element.
        v_CopyFieldToArnoldiArray(tgt);
    }
	
	
    /**
     *
     */
    void DriverModifiedArnoldi::EV_small(
                                         Array<OneD, Array<OneD, NekDouble> > &Kseq,
                                         const int ntot,
                                         const Array<OneD, NekDouble> &alpha,
                                         const int kdim,
                                         Array<OneD, NekDouble> &zvec,
                                         Array<OneD, NekDouble> &wr,
                                         Array<OneD, NekDouble> &wi,
                                         NekDouble &resnorm)
    {
        int kdimp = kdim + 1;
        int lwork = 10*kdim;
        int ier;
        Array<OneD, NekDouble> R(kdimp * kdimp, 0.0);
        Array<OneD, NekDouble> H(kdimp * kdim, 0.0);
        Array<OneD, NekDouble> rwork(lwork, 0.0);
	
        // Modified G-S orthonormalisation
        for (int i = 0; i < kdimp; ++i)
        {
            NekDouble gsc = std::sqrt(Vmath::Dot(ntot, &Kseq[i][0], 1, &Kseq[i][0], 1));
            ASSERTL0(gsc != 0.0, "Vectors are linearly independent.");
            R[i*kdimp+i] = gsc;
            Vmath::Smul(ntot, 1.0/gsc, Kseq[i], 1, Kseq[i], 1);
            for (int j = i + 1; j < kdimp; ++j)
            {
                gsc = Vmath::Dot(ntot, Kseq[i], 1, Kseq[j], 1);
                Vmath::Svtvp(ntot, -gsc, Kseq[i], 1, Kseq[j], 1, Kseq[j], 1);
                R[j*kdimp+i] = gsc;
            }
        }
	
        // Compute H matrix
        for (int i = 0; i < kdim; ++i)
        {
            for (int j = 0; j < kdim; ++j)
            {
                H[j*kdim+i] = alpha[j+1] * R[(j+1)*kdimp+i]
                    - Vmath::Dot(j, &H[0] + i, kdim, &R[0] + j*kdimp, 1);
                H[j*kdim+i] /= R[j*kdimp+j];
            }
        }
	
        H[(kdim-1)*kdim+kdim] = alpha[kdim]
            * std::fabs(R[kdim*kdimp+kdim] / R[(kdim-1)*kdimp + kdim-1]);
	
        Lapack::dgeev_('N','V',kdim,&H[0],kdim,&wr[0],&wi[0],0,1,&zvec[0],kdim,&rwork[0],lwork,ier);
	
        ASSERTL0(!ier, "Error with dgeev");
	
        resnorm = H[(kdim-1)*kdim + kdim];
    }
    
	
    /**
     *
     */
    int DriverModifiedArnoldi::EV_test(
                                       const int itrn,
                                       const int kdim,
                                       Array<OneD, NekDouble> &zvec,
                                       Array<OneD, NekDouble> &wr,
                                       Array<OneD, NekDouble> &wi,
                                       const NekDouble resnorm,
                                       const int nvec,
                                       ofstream &evlout,
                                       NekDouble &resid0)
    {
        int idone = 0;
        NekDouble re_ev, im_ev, abs_ev, ang_ev, re_Aev, im_Aev;	
        // NekDouble period = 0.1;
	
        Array<OneD, NekDouble> resid(kdim);
        for (int i = 0; i < kdim; ++i)
        {
            resid[i] = resnorm * std::fabs(zvec[kdim - 1 + i*kdim]) /
                std::sqrt(Vmath::Dot(kdim, &zvec[0] + i*kdim, 1, &zvec[0] + i*kdim, 1));
            if (wi[i] < 0.0) resid[i-1] = resid[i] = hypot(resid[i-1], resid[i]);
        }
        EV_sort(zvec, wr, wi, resid, kdim);
	
        if (resid[nvec-1] < m_evtol) idone = nvec;
	
        evlout << "-- Iteration = " << itrn << ", H(k+1, k) = " << resnorm << endl;
        evlout.precision(4);
        evlout.setf(ios::scientific, ios::floatfield);
        if(m_TimeSteppingAlgorithm)
        {
            evlout << "EV  Magnitude   Angle       Growth      Frequency   Residual"
                   << endl;
            for (int i = 0; i < kdim; i++) 
            {
                re_ev  = wr[i];
                im_ev  = wi[i];
                abs_ev = hypot (re_ev, im_ev);
                ang_ev = atan2 (im_ev, re_ev);
                re_Aev = log (abs_ev) / m_period;
                im_Aev = ang_ev       / m_period;
                evlout << setw(2)  << i
                       << setw(12) << abs_ev
                       << setw(12) << ang_ev
                       << setw(12) << re_Aev
                       << setw(12) << im_Aev
                       << setw(12) << resid[i]
                       << endl;
            }
        }
        else
        {
            evlout << "EV  Magnitude   Angle      inverse real  inverse imag  Residual"
                   << endl;
            for (int i = 0; i < kdim; i++) 
            {
                re_ev  = wr[i];
                im_ev  = wi[i];
                abs_ev = hypot (re_ev, im_ev);
                ang_ev = atan2 (im_ev, re_ev);
                re_Aev = -wr[i]/(abs_ev*abs_ev);
                im_Aev =  wi[i]/(abs_ev*abs_ev);
                evlout << setw(2)  << i
                       << setw(12) << abs_ev
                       << setw(12) << ang_ev
                       << setw(12) << re_Aev
                       << setw(12) << im_Aev
                       << setw(12) << resid[i]
                       << endl;
            }
            
        }
	
        resid0 = resid[0];
        return idone;
    }
    
    
    /**
     *
     */
    void DriverModifiedArnoldi::EV_sort(
                                        Array<OneD, NekDouble> &evec,
                                        Array<OneD, NekDouble> &wr,
                                        Array<OneD, NekDouble> &wi,
                                        Array<OneD, NekDouble> &test,
                                        const int dim)
    {
        Array<OneD, NekDouble> z_tmp(dim,0.0);
        NekDouble wr_tmp, wi_tmp, te_tmp;
        for (int j = 1; j < dim; ++j)
        {
            wr_tmp = wr[j];
            wi_tmp = wi[j];
            te_tmp = test[j];
            Vmath::Vcopy(dim, &evec[0] + j*dim, 1, &z_tmp[0], 1);
            int i = j - 1;
            while (i >= 0 && test[i] > te_tmp)
            {
                wr[i+1] = wr[i];
                wi[i+1] = wi[i];
                test[i+1] = test[i];
                Vmath::Vcopy(dim, &evec[0] + i*dim, 1, &evec[0] + (i+1)*dim, 1);
                i--;
            }
            wr[i+1] = wr_tmp;
            wi[i+1] = wi_tmp;
            test[i+1] = te_tmp;
            Vmath::Vcopy(dim, &z_tmp[0], 1, &evec[0] + (i+1)*dim, 1);
        }	
    };
    
}
	
	



/**
 * $Log $
**/
