/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyleFly.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVInteractorStyleFly.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"

vtkCxxRevisionMacro(vtkPVInteractorStyleFly, "1.5");
vtkStandardNewMacro(vtkPVInteractorStyleFly);

//-------------------------------------------------------------------------
vtkPVInteractorStyleFly::vtkPVInteractorStyleFly()
{
  this->FlyFlag = 0;
  this->Speed = 20.0;
  this->LastRenderTime = 0.1; // Assume 10 frames per second.
  
  this->CameraXAxis[0] = 1.0;
  this->CameraXAxis[1] = 0.0;
  this->CameraXAxis[2] = 0.0;

  this->CameraYAxis[0] = 0.0;
  this->CameraYAxis[1] = 1.0;
  this->CameraYAxis[2] = 0.0;

  this->CameraZAxis[0] = 0.0;
  this->CameraZAxis[1] = 0.0;
  this->CameraZAxis[2] = 1.0;
}

//-------------------------------------------------------------------------
vtkPVInteractorStyleFly::~vtkPVInteractorStyleFly()
{
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleFly::OnLeftButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  double *range = this->CurrentRenderer->GetActiveCamera()->GetClippingRange();
  this->Fly(range[1], this->Speed);
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleFly::OnLeftButtonUp()
{
  this->FlyFlag = 0;
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleFly::OnRightButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  double *range = this->CurrentRenderer->GetActiveCamera()->GetClippingRange();
  this->Fly(range[1], -this->Speed);
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleFly::OnRightButtonUp()
{
  this->FlyFlag = 0;
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleFly::Fly(float scale, float speed)
{
  double time;

  this->FlyFlag = 1;

  if ( ! this->CurrentRenderer)
    {
    return;
    }
  
  // This is not a vtkKWObject, which is what contains the Script method,
  // so we'll call Script on vtkPVRenderView instead.
  // This seems very hackish.
  // This is to just get the current mouse position.
  vtkPVGenericRenderWindowInteractor *iren =
    (vtkPVGenericRenderWindowInteractor*)this->Interactor;
  vtkPVRenderView *view = iren->GetPVRenderView();
  const char *renWidgetName = view->GetVTKWidget()->GetWidgetName();
  vtkPVApplication *app = view->GetPVApplication();
  
  view->Script("winfo rootx %s", renWidgetName);
  int xmin = vtkKWObject::GetIntegerResult(app);
  view->Script("winfo rooty %s", renWidgetName);
  int ymin = vtkKWObject::GetIntegerResult(app);
  int x, y;
  float fx, fy, px, py;
  int *size;
  double *pos;
  vtkCamera *camera;
  float velocity;
  vtkTransform *transform = vtkTransform::New();
  
  while (this->FlyFlag)
    {
    // Get the position of the mouse in the renderer.
    view->Script("winfo pointerx %s", renWidgetName);
    x = vtkKWObject::GetIntegerResult(app);
    view->Script("winfo pointery %s", renWidgetName);
    y = vtkKWObject::GetIntegerResult(app);
    
    // Relative to renderer not window.
    x -= xmin;
    y -= ymin;
    
    // Compute the rotation angles (fx, fy).
    // Relative to center of renderer.
    camera = this->CurrentRenderer->GetActiveCamera();
    pos = camera->GetPosition();
    size = this->CurrentRenderer->GetSize();
    // Normalized -0.5 to 0.5
    fx = (size[0] * 0.5 - x) / size[0];
    fy = (size[1] * 0.5 - y) / size[1];

    // At this point we will compute the velocity from
    // the intermediate angle computation (normalize pointer position).
    // Absolute Value.
    px = (fx > 0 ? fx : -fx);
    py = (fy > 0 ? fy : -fy);
    // Maximum.
    if (px < py)
      {
      px = py;
      }
    // Just in case, clamp below 0.5
    if (px > 0.5)
      {
      px = 0.5;
      }

    // Compute the forwad movement.
    // Slow the velocity down during tight turns
    velocity = 0.02 * (speed * this->LastRenderTime) * scale * (1.0 - 2.0*px);

    // Finish computing the rotation angles.
    // I thought aboput scaling rotation with speed, but I want fly to degenerate
    // to rotate at zero speed.
    fx = 30.0 * fx * this->LastRenderTime;
    fy = 30.0 * fy * this->LastRenderTime;
    
    // move the camera
    this->ComputeCameraAxes();
    transform->Identity();
    transform->Translate(pos[0], pos[1], pos[2]);
    transform->RotateWXYZ(fx, this->CameraYAxis[0],
                          this->CameraYAxis[1], this->CameraYAxis[2]);
    transform->RotateWXYZ(fy, this->CameraXAxis[0],
                          this->CameraXAxis[1], this->CameraXAxis[2]);
    transform->Translate(-pos[0], -pos[1], -pos[2]);
    
    // actually flying
    transform->Translate(-velocity * this->CameraZAxis[0],
                         -velocity * this->CameraZAxis[1],
                         -velocity * this->CameraZAxis[2]);
    camera->ApplyTransform(transform);
    camera->OrthogonalizeViewUp();

    // Time the render to make flying independant from rendering rates.
    time = vtkTimerLog::GetCurrentTime();
    this->ResetLights();  
    this->CurrentRenderer->ResetCameraClippingRange();
    this->Interactor->Render();
    this->LastRenderTime = vtkTimerLog::GetCurrentTime() - time;
    
    // Check to see if the user has released the mouse button
    view->Script("update");
    }
    
  this->StopState();
  
  transform->Delete();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleFly::ComputeCameraAxes()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  
  camera->OrthogonalizeViewUp();
  camera->GetViewUp(this->CameraYAxis);
  camera->GetDirectionOfProjection(this->CameraZAxis);
  
  // I do not know if this is actually used, but this was originally
  // set to the ViewPlaneNormal.
  this->CameraZAxis[0] = - this->CameraZAxis[0];
  this->CameraZAxis[1] = - this->CameraZAxis[1];
  this->CameraZAxis[2] = - this->CameraZAxis[2];  

  vtkMath::Cross(this->CameraYAxis, this->CameraZAxis, this->CameraXAxis);
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleFly::ResetLights()
{
  vtkLightCollection *lights;
  vtkLight *light;
  vtkCamera *cam;

  if (this->CurrentRenderer == NULL) {return;}
  lights = this->CurrentRenderer->GetLights();
  lights->InitTraversal();
  light = lights->GetNextItem();
  if (light == NULL) 
    {
    return;
    }
  cam = this->CurrentRenderer->GetActiveCamera();
  light->SetPosition(cam->GetPosition());
  light->SetFocalPoint(cam->GetFocalPoint());
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleFly::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Speed: " << this->GetSpeed() << endl;
}
