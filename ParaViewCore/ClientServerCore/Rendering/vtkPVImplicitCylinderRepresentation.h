/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImplicitCylinderRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImplicitCylinderRepresentation - extends vtkImplicitCylinderRepresentation
// .SECTION Description
// vtkPVImplicitCylinderRepresentation extends vtkImplicitCylinderRepresentation
// to add ParaView proper initialisation values

#ifndef vtkPVImplicitCylinderRepresentation_h
#define vtkPVImplicitCylinderRepresentation_h

#include "vtkImplicitCylinderRepresentation.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports


class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVImplicitCylinderRepresentation
  : public vtkImplicitCylinderRepresentation
{
public:
  static vtkPVImplicitCylinderRepresentation* New();
  vtkTypeMacro(vtkPVImplicitCylinderRepresentation, vtkImplicitCylinderRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVImplicitCylinderRepresentation();
  ~vtkPVImplicitCylinderRepresentation();

private:
  vtkPVImplicitCylinderRepresentation(const vtkPVImplicitCylinderRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVImplicitCylinderRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
