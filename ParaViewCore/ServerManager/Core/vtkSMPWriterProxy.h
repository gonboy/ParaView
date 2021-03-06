/*=========================================================================

Program:   ParaView
Module:    vtkSMPWriterProxy.h

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPWriterProxy - proxy for a VTK writer that supports parallel 
// writing.
// .SECTION Description
// vtkSMPWriterProxy is the proxy for all writers that can write in parallel.
// The only extra functionality provided by this class is that it sets up the
// piece information on the writer.

#ifndef vtkSMPWriterProxy_h
#define vtkSMPWriterProxy_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMWriterProxy.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMPWriterProxy : public vtkSMWriterProxy
{
public:
  static vtkSMPWriterProxy* New();
  vtkTypeMacro(vtkSMPWriterProxy, vtkSMWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMPWriterProxy();
  ~vtkSMPWriterProxy();

private:
  vtkSMPWriterProxy(const vtkSMPWriterProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMPWriterProxy&) VTK_DELETE_FUNCTION;
};


#endif

