/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVFrustumActor
// .SECTION Description
// vtkPVFrustumActor is an actor that renders a frustum. Used in ParaView to
// show the frustum used for frustum selection extraction.

#ifndef vtkPVFrustumActor_h
#define vtkPVFrustumActor_h

#include "vtkOpenGLActor.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkOutlineSource;
class vtkPolyDataMapper;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVFrustumActor : public vtkOpenGLActor
{
public:
  static vtkPVFrustumActor* New();
  vtkTypeMacro(vtkPVFrustumActor, vtkOpenGLActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the frustum.
  void SetFrustum(double corners[24]);

  // Description:
  // Convenience method to set the color.
  void SetColor(double r, double g, double b);
  void SetLineWidth(double r);

protected:
  vtkPVFrustumActor();
  ~vtkPVFrustumActor();

  vtkOutlineSource* Outline;
  vtkPolyDataMapper* Mapper;

private:
  vtkPVFrustumActor(const vtkPVFrustumActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVFrustumActor&) VTK_DELETE_FUNCTION;

};

#endif
