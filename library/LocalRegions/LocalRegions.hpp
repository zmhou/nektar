///////////////////////////////////////////////////////////////////////////////
//
// File LocalRegions.hpp
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
// Description: General header for localregions containing enum and constants 
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LOCALREGIONS_H
#define LOCALREGIONS_H

#include <string>
#include <StdRegions/StdRegions.hpp>
#include <SpatialDomains/SpatialDomains.hpp>

// Definition of main page for doxygen for Local Region Library
/**
 *\mainpage Local Regions Library
 */

namespace Nektar
{
  namespace LocalRegions
  {

    enum GeomState
    {
      eNotFilled,
      ePtsFilled,
    };

  // Defines a "fast find"
  // Assumes that first/last define the beginning/ending of
  // a continuous range of classes, and that start is
  // an iterator between first and last

    template<class InputIterator, class EqualityComparable>
    InputIterator find(InputIterator first, InputIterator last,
		       InputIterator startingpoint,
		       const EqualityComparable& value)
    {

      InputIterator val;
    
      if(startingpoint == first)
      {
	val = find(first,last,value);
      }
      else
      {
	val = find(startingpoint,last,value);
	if(val == last)
	{
	  val = find(first,startingpoint,value);
	  if(val == startingpoint)
	  {
	    val = last;
	  }
	}
      }
      return val;
    }

  } // end of namespace
} // end of namespace

#endif //LOCALREGIONS_H

/** 
 *    $Log: LocalRegions.hpp,v $
 *    Revision 1.1  2006/05/04 18:58:45  kirby
 *    *** empty log message ***
 *
 *    Revision 1.2  2006/03/13 11:17:03  sherwin
 *
 *    First compiing version of Demos in SpatialDomains and LocalRegions. However they do not currently seem to execute properly
 *
 *    Revision 1.1  2006/03/12 07:43:31  sherwin
 *
 *    First revision to meet coding standard. Needs to be compiled
 *
 **/
