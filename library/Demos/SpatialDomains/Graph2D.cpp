#include <cstdio>
#include <cstdlib>
#include <iostream> 
#include "SpatialDomains/MeshGraph2D.h"
#include <SpatialDomains/BoundaryConditions.h>

#include <boost/shared_ptr.hpp>

using namespace Nektar;
using namespace SpatialDomains; 
using namespace std;

// compile using Builds/Demos/SpatialDomains -> make DEBUG=1 Graph1D

int main(int argc, char *argv[]){

    //if(argc != 2){
    //  cerr << "usage: Graph2D file" << endl;
    //  exit(1);
    //}

    //string in(argv[argc-1]);
    // If we all have the same relative structure, these should work for everyone.
#if 0 
    string in("../../../library/Demos/SpatialDomains/meshdef2D.xml");
    string bcfile("../../../library/Demos/SpatialDomains/BC1.xml");
#else 
    string in("C:/Data/PhD/Research/dev/Nektar++/library/Demos/SpatialDomains/meshdef2D.xml");
    string bcfile("c:/Data/PhD/Research/dev/Nektar++/library/Demos/SpatialDomains/BC1.xml");
#endif

    MeshGraph2D graph2D;
    BoundaryConditions bcs(&graph2D);

    graph2D.Read(in);
    bcs.Read(bcfile);

    try
    {
        ConstForcingFunctionShPtr ffunc  = bcs.GetForcingFunction("u");
        NekDouble val = ffunc->Evaluate(8.0);
        
        ConstForcingFunctionShPtr ffunc2  = bcs.GetForcingFunction("v");
        val = ffunc->Evaluate(1.5);
        
        ConstInitialConditionShPtr ic = bcs.GetInitialCondition("v");
        val = ic->Evaluate(1.5);

        NekDouble tolerance = bcs.GetParameter("Tolerance");

        BoundaryConditionCollection &bConditions = bcs.GetBoundaryConditions();

        BoundaryConditionShPtr bcShPtr = (*bConditions[0])["v"];

        if (bcShPtr->GetBoundaryConditionType() == eDirichlet)
        {
            DirichletBCShPtr dirichletBCShPtr = boost::static_pointer_cast<DirichletBoundaryCondition>(bcShPtr);
            val = dirichletBCShPtr->m_DirichletCondition.Evaluate(1.5);
        }

        std::string fcn1 = bcs.GetFunction("F1");
        std::string fcn2 = bcs.GetFunction("F2");
        std::string fcn3 = bcs.GetFunction("F3");
        std::string fcn4 = bcs.GetFunction("F4");

        Equation eqn1 = bcs.GetFunctionAsEquation("F3");

        ConstExpansionElementShPtr exp = bcs.GetExpansionElement(0);
    }
    catch(std::string err)
    {
        cout << err << endl;
    }
    
    return 0;
}
