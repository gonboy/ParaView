/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef pqStandardLegacyCustomPanels_h
#define pqStandardLegacyCustomPanels_h

#include "pqObjectPanelInterface.h"
#include <QObject>

#include "pqComponentsModule.h"

/// pqStandardLegacyCustomPanels is used by pqPropertiesPanel to create legacy
/// custom panels. In time, this class needs to disappear with all panels being
/// created using the newer panels API.
class PQCOMPONENTS_EXPORT pqStandardLegacyCustomPanels :
  public QObject,
  public pqObjectPanelInterface
{
  Q_OBJECT
  Q_INTERFACES(pqObjectPanelInterface)
  typedef QObject Superclass;
public:
  pqStandardLegacyCustomPanels(QObject* parent=0);
  virtual ~pqStandardLegacyCustomPanels();

  /// create panel for proxy.
  virtual pqObjectPanel* createPanel(pqProxy* proxy, QWidget* parent);

  /// returns if a panel can be created.
  virtual bool canCreatePanel(pqProxy* proxy) const;

private:
  Q_DISABLE_COPY(pqStandardLegacyCustomPanels)
};
#endif
