/*=========================================================================

  Program:   earaView
  Module:    vtkPEquivalenceSet.h

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
// .NAME vtkPEquivalenceSet - distributed method of Equivalence
// .SECTION Description
// Same as EquivalenceSet, but resolving is a global operation.
// .SEE vtkEquivalenceSet

#ifndef vtkPEquivalenceSet_h
#define vtkPEquivalenceSet_h

#include "vtkEquivalenceSet.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPEquivalenceSet : public vtkEquivalenceSet
{
public:
  vtkTypeMacro(vtkPEquivalenceSet,vtkEquivalenceSet);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPEquivalenceSet *New();

  // Globally equivalent set IDs are reassigned to be sequential.
  virtual int ResolveEquivalences ();

protected:
  vtkPEquivalenceSet();
  ~vtkPEquivalenceSet();

private:
  vtkPEquivalenceSet(const vtkPEquivalenceSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPEquivalenceSet&) VTK_DELETE_FUNCTION;
};

#endif /* vtkPEquivalenceSet_h */
