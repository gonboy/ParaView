/*=========================================================================

   Program: ParaView
   Module:    pqHistogramWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

=========================================================================*/

/*!
 * \file pqHistogramWidget.cxx
 *
 * \brief
 *   The pqHistogramWidget class is used to display and interact with
 *   a histogram chart.
 *
 * \author Mark Richardson
 * \date   May 10, 2005
 */

#include "pqHistogramWidget.h"

#include "pqChartAxis.h"
#include "pqChartLabel.h"
#include "pqChartMouseBox.h"
#include "pqChartValue.h"
#include "pqChartZoomPan.h"
#include "pqHistogramChart.h"
#include "pqHistogramColor.h"
#include "pqHistogramSelection.h"
#include "pqHistogramSelectionModel.h"
#include "pqLineChart.h"

#include <QCursor>
#include <QEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QPrinter>
#include <QTimer>

// Set up a margin around the histogram.
#define MARGIN 3
#define DBL_MARGIN 6


/// \class pqHistogramWidgetData
/// \brief
///   The pqHistogramWidgetData class stores information needed for
///   the mouse interactions.
class pqHistogramWidgetData
{
public:
  pqHistogramWidgetData();
  ~pqHistogramWidgetData() {}

public:
  /// Stores the previously used list of selected ranges.
  pqHistogramSelectionList Selection;
};


pqHistogramWidgetData::pqHistogramWidgetData()
  : Selection()
{
}


pqHistogramWidget::pqHistogramWidget(QWidget *p) :
  QAbstractScrollArea(p),
  BackgroundColor(Qt::white),
  Mode(pqHistogramWidget::NoMode),
  Interact(pqHistogramWidget::Bin),
  EasyBinSelection(true),
  SelectMode(pqHistogramWidget::Bin),
  Title(new pqChartLabel()),
  XAxis(new pqChartAxis(pqChartAxis::Bottom)),
  YAxis(new pqChartAxis(pqChartAxis::Left)),
  FAxis(new pqChartAxis(pqChartAxis::Right)),
  Histogram(new pqHistogramChart()),
  LineChart(new pqLineChart()),
  Data(new pqHistogramWidgetData()),
  Mouse(new pqChartMouseBox()),
  ZoomPan(new pqChartZoomPan(this)),
  MoveTimer(0),
  LastBin(-1),
  LastValueX(-1),
  MouseDown(false),
  SkipContextMenu(false)
{
  // Set up the default Qt properties.
  this->setFocusPolicy(Qt::ClickFocus);
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  QPalette newPalette = this->viewport()->palette();
  newPalette.setColor(QPalette::Background, QColor(Qt::white));
  this->viewport()->setPalette(newPalette);
  this->setAttribute(Qt::WA_KeyCompression);

  // Setup the chart title
  connect(this->Title, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->Title, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));

  // Set up the histogram and its parameters.
  QFont myFont = font();
  this->XAxis->setNeigbors(this->YAxis, this->FAxis);
  this->XAxis->setTickLabelFont(myFont);
  connect(this->XAxis, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->XAxis, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));

  this->YAxis->setNeigbors(this->XAxis, 0);
  this->YAxis->setTickLabelFont(myFont);
  connect(this->YAxis, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->YAxis, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));

  this->FAxis->setNeigbors(this->XAxis, 0);
  this->FAxis->setTickLabelFont(myFont);
  this->FAxis->setAxisColor(Qt::darkBlue); // Differentiate the function grid.
  connect(this->FAxis, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->FAxis, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));

  this->Histogram->setAxes(this->XAxis, this->YAxis);
  connect(this->Histogram, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->Histogram, SIGNAL(repaintNeeded()), this,
      SLOT(repaintChart()));

  this->LineChart->setAxes(this->XAxis, this->FAxis);
  connect(this->LineChart, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->LineChart, SIGNAL(repaintNeeded()), this,
      SLOT(repaintChart()));

  // Set up the widget for keyboard input.
  this->setAttribute(Qt::WA_InputMethodEnabled);

  // Connect to the zoom/pan object signal.
  this->ZoomPan->setObjectName("ZoomPan");
  connect(this->ZoomPan, SIGNAL(contentsSizeChanging(int, int)),
      this, SLOT(layoutChart(int, int)));
}

pqHistogramWidget::~pqHistogramWidget()
{
  delete this->ZoomPan;
  delete this->Mouse;
  delete this->Data;
  delete this->LineChart;
  delete this->Histogram;
  delete this->FAxis;
  delete this->YAxis;
  delete this->XAxis;
  delete this->Title;
}

void pqHistogramWidget::setBackgroundColor(const QColor& color)
{
  this->BackgroundColor = color;

  this->layoutChart(this->ZoomPan->contentsWidth(),
      this->ZoomPan->contentsHeight());
}

bool pqHistogramWidget::event(QEvent *e)
{
  bool ret = QAbstractScrollArea::event(e);

  if(e->type() == QEvent::FontChange)
    {
    // Block the axis update signals until all the changes are
    // made. This avoids laying out the chart for each individual
    // change.
    this->XAxis->blockSignals(true);
    this->XAxis->setTickLabelFont(this->font());
    this->XAxis->blockSignals(false);

    this->YAxis->blockSignals(true);
    this->YAxis->setTickLabelFont(this->font());
    this->YAxis->blockSignals(false);

    this->FAxis->blockSignals(true);
    this->FAxis->setTickLabelFont(this->font());
    this->FAxis->blockSignals(false);

    this->layoutChart(this->ZoomPan->contentsWidth(),
        this->ZoomPan->contentsHeight());
    }
  return ret;
}

void pqHistogramWidget::setInteractMode(InteractMode mode)
{
  if(mode != this->Interact)
    {
    if(this->Interact == pqHistogramWidget::ValueMove)
      this->LastValueX = -1;
    this->Interact = mode;
    if(this->Interact != pqHistogramWidget::Function)
      {
      // If the user is switching between bin and value selection,
      // clear the selection.
      if(this->SelectMode != this->Interact && (this->Interact ==
          pqHistogramWidget::Bin || this->SelectMode ==
          pqHistogramWidget::Bin))
        {
        this->selectNone();
        }

      this->SelectMode = this->Interact;
      }

    emit this->interactModeChanged(this->Interact);
    }
}

void pqHistogramWidget::setEasyBinSelection(bool Enable)
{
  this->EasyBinSelection = Enable;
}

void pqHistogramWidget::selectAll()
{
  if(this->Interact == pqHistogramWidget::Bin)
    this->Histogram->getSelectionModel()->selectAllBins();
  else if(this->Interact == pqHistogramWidget::Value)
    this->Histogram->getSelectionModel()->selectAllValues();
}

void pqHistogramWidget::selectNone()
{
  this->Histogram->getSelectionModel()->selectNone();
}

void pqHistogramWidget::selectInverse()
{
  if(this->Histogram->getSelectionModel()->hasSelection())
    {
    this->Histogram->getSelectionModel()->selectInverse();
    }
  else
    {
    this->selectAll();
    }
}

void pqHistogramWidget::updateLayout()
{
  // All of the chart members' layouts are interrelated. When one
  // of them needs to be updated, they all need to be updated.
  this->layoutChart(this->ZoomPan->contentsWidth(),
      this->ZoomPan->contentsHeight());
  this->viewport()->update();
}

void pqHistogramWidget::repaintChart()
{
  this->viewport()->update();
}

void pqHistogramWidget::layoutChart(int w, int h)
{
  QRect area(MARGIN, MARGIN, w - DBL_MARGIN, h - DBL_MARGIN);
  
  const QRect title_request = this->Title->getSizeRequest();
  this->Title->setBounds(QRect(area.left(), area.top(), area.width(), title_request.height()));
  area = QRect(area.left(), area.top() + title_request.height(), area.right(), area.height() - title_request.height());
  
  this->XAxis->layoutAxis(area);
  this->YAxis->layoutAxis(area);
  this->FAxis->layoutAxis(area);
  this->Histogram->layoutChart();
  this->LineChart->layoutChart();
}

void pqHistogramWidget::moveTimeout()
{
  this->Histogram->getSelectionModel()->endInteractiveChange();
}

QSize pqHistogramWidget::sizeHint() const
{
  this->ensurePolished();
  int f = 150 + 2*this->frameWidth();
  return QSize(f, f);
}

void pqHistogramWidget::keyPressEvent(QKeyEvent *e)
{
  bool handled = true;
  if(e->key() == Qt::Key_Plus || e->key() == Qt::Key_Minus ||
      e->key() == Qt::Key_Equal)
    {
    // If only the ctrl key is down, zoom only in the x. If only
    // the alt key is down, zoom only in the y. Otherwise, zoom
    // both axes by the same amount. Mask off the shift key since
    // it is needed to press the plus key.
    pqChartZoomPan::InteractFlags flags = pqChartZoomPan::ZoomBoth;
    int state = e->modifiers() & (Qt::ControlModifier | Qt::AltModifier |
        Qt::MetaModifier);
    if(state == Qt::ControlModifier)
      flags = pqChartZoomPan::ZoomXOnly;
    else if(state == Qt::AltModifier)
      flags = pqChartZoomPan::ZoomYOnly;

    // Zoom in for the plus/equal key and out for the minus key.
    if(e->key() == Qt::Key_Minus)
      this->ZoomPan->zoomOut(flags);
    else
      this->ZoomPan->zoomIn(flags);
    }
  else if(e->key() == Qt::Key_Up)
    {
    this->ZoomPan->panUp();
    }
  else if(e->key() == Qt::Key_Down)
    {
    this->ZoomPan->panDown();
    }
  else if(e->key() == Qt::Key_Left)
    {
    if(e->modifiers() == Qt::AltModifier)
      this->ZoomPan->historyPrevious();
    else
      this->ZoomPan->panLeft();
    }
  else if(e->key() == Qt::Key_Right)
    {
    if(e->modifiers() == Qt::AltModifier)
      this->ZoomPan->historyNext();
    else
      this->ZoomPan->panRight();
    }
  else if(e->key() == Qt::Key_M)
    {
    if(this->Interact == pqHistogramWidget::Value)
      setInteractMode(pqHistogramWidget::ValueMove);
    else if(this->Interact == pqHistogramWidget::ValueMove)
      setInteractMode(pqHistogramWidget::Value);
    else
      handled = false;
    }
  else
    handled = false;

  if(handled)
    e->accept();
  else
    QAbstractScrollArea::keyPressEvent(e);
}

void pqHistogramWidget::showEvent(QShowEvent *e)
{
  QAbstractScrollArea::showEvent(e);
  this->ZoomPan->updateContentSize();
}

void pqHistogramWidget::paintEvent(QPaintEvent *e)
{
  // Get the clip area from the paint event. Set the painter to
  // content coordinates.
  QRect area = e->rect();
  if(!area.isValid())
    return;
    
  QPainter painter(this->viewport());
    
  this->draw(painter, area);
  
  e->accept();
}

void pqHistogramWidget::mousePressEvent(QMouseEvent *e)
{
  // Get the current mouse position and convert it to contents coords.
  this->MouseDown = true;
  QPoint point = e->pos();
  point.rx() += this->ZoomPan->contentsX();
  point.ry() += this->ZoomPan->contentsY();

  // Save the necessary coordinates.
  this->Mouse->Last = point;
  this->ZoomPan->Last = e->globalPos();

  // Make sure the timer is allocated and connected.
  if(!this->MoveTimer)
    {
    this->MoveTimer = new QTimer(this);
    this->MoveTimer->setObjectName("MouseMoveTimeout");
    this->MoveTimer->setSingleShot(true);
    connect(this->MoveTimer, SIGNAL(timeout()), this, SLOT(moveTimeout()));
    }

  if(this->Histogram && e->button() == Qt::LeftButton)
    {
    // Block the selection changed signals in case the user is going to
    // drag the mouse. Use a timer to delay the signal if the user is
    // simply picking.
    pqHistogramSelection range;
    pqHistogramSelectionModel *model = this->Histogram->getSelectionModel();
    if(this->Interact == pqHistogramWidget::Bin)
      {
      int bin = this->Histogram->getBinAt(point.x(), point.y(), this->EasyBinSelection);
      range.setType(pqHistogramSelection::Bin);
      range.setRange(bin, bin);
      if(e->modifiers() & Qt::ShiftModifier)
        {
        if(bin != -1)
          {
          model->beginInteractiveChange();
          if(this->LastBin == -1)
            {
            model->setSelection(range);
            this->LastBin = bin;
            }
          else
            {
            range.setFirst(this->LastBin);
            model->setSelection(range);
            }
          }
        }
      else if(e->modifiers() & Qt::ControlModifier)
        {
        if(bin != -1)
          {
          model->beginInteractiveChange();
          model->xorSelection(range);
          this->LastBin = bin;

          // Set up the selection list so the first click doesn't
          // get changed.
          this->Data->Selection.clear();
          this->Data->Selection.append(range);
          }
        else
          {
          this->Data->Selection.clear();
          }
        }
      else
        {
        model->beginInteractiveChange();
        this->LastBin = bin;
        if(bin == -1)
          {
          model->selectNone();
          }
        else
          {
          model->setSelection(range);
          }
        }
      }
    else if(this->Interact == pqHistogramWidget::Value)
      {
      pqChartValue val;
      bool valid = this->Histogram->getValueAt(point.x(), point.y(), val);
      range.setType(pqHistogramSelection::Value);
      range.setRange(val, val);
      if(e->modifiers() & Qt::ShiftModifier)
        {
        if(valid)
          {
          model->beginInteractiveChange();
          if(this->LastValueX == -1)
            {
            this->LastValueX = point.x();
            model->setSelection(range);
            }
          else
            {
            pqChartValue last;
            if(this->Histogram->getValueAt(this->LastValueX, point.y(), last))
              {
              range.setFirst(last);
              model->setSelection(range);
              }
            }
          }
        }
      else if(e->modifiers() & Qt::ControlModifier)
        {
        if(valid)
          {
          model->beginInteractiveChange();
          this->LastValueX = point.x();
          model->xorSelection(range);

          // Set up the selection list so the first click doesn't
          // get changed.
          this->Data->Selection.clear();
          this->Data->Selection.append(range);
          }
        else
          {
          this->Data->Selection.clear();
          }
        }
      else
        {
        model->beginInteractiveChange();
        if(valid)
          {
          this->LastValueX = point.x();
          model->setSelection(range);
          }
        else
          {
          this->LastValueX = -1;
          model->selectNone();
          }
        }
      }
    else if(this->Interact == pqHistogramWidget::ValueMove)
      {
      bool valid = this->Histogram->getValueRangeAt(point.x(), point.y(), range);
      if(valid)
        {
        this->LastValueX = point.x();
        }
      else
        {
        this->LastValueX = -1;
        }
      }

    if(model->isInInteractiveChange())
      {
      this->Mode = pqHistogramWidget::MoveWait;
      this->MoveTimer->start(200);
      this->viewport()->update();
      }
    }

  e->accept();
}

void pqHistogramWidget::mouseReleaseEvent(QMouseEvent *e)
{
  // Get the current mouse position and convert it to contents coords.
  this->MouseDown = false;
  QPoint point = e->pos();
  point.rx() += this->ZoomPan->contentsX();
  point.ry() += this->ZoomPan->contentsY();

  if(this->Mode == pqHistogramWidget::ZoomBox)
    {
    this->Mode = pqHistogramWidget::NoMode;
    this->setCursor(Qt::ArrowCursor);
    this->Mouse->adjustBox(point);
    this->ZoomPan->zoomToRectangle(&this->Mouse->Box);
    this->Mouse->resetBox();
    }
  else if(this->Mode == pqHistogramWidget::SelectBox)
    {
    this->Mode = pqHistogramWidget::NoMode;
    this->Histogram->getSelectionModel()->endInteractiveChange();
    this->Data->Selection.clear();

    this->Mouse->adjustBox(point);
    QRect area = this->Mouse->Box;
    this->Mouse->resetBox();
    if(area.isValid())
      {
      // Translate the area to viewport coordinates.
      area.translate(-this->ZoomPan->contentsX(), -this->ZoomPan->contentsY());
      this->viewport()->update(area);
      }
    }
  else if(this->Mode == pqHistogramWidget::ValueDrag)
    {
    this->Mode = pqHistogramWidget::NoMode;
    this->setCursor(Qt::ArrowCursor);
    this->Histogram->getSelectionModel()->endInteractiveChange();
    this->Data->Selection.clear();
    }
  else if(this->Mode == pqHistogramWidget::Zoom ||
      this->Mode == pqHistogramWidget::Pan)
    {
    this->Mode = pqHistogramWidget::NoMode;
    this->ZoomPan->finishInteraction();
    }
  else if(this->Mode == pqHistogramWidget::MoveWait)
    {
    // Cancel the timer and send the selection changed signal.
    this->Mode = pqHistogramWidget::NoMode;
    this->setCursor(Qt::ArrowCursor);
    if(this->MoveTimer)
      {
      this->MoveTimer->stop();
      }

    this->Histogram->getSelectionModel()->endInteractiveChange();
    this->Data->Selection.clear();
    }
  else if(this->Mode != pqHistogramWidget::NoMode)
    {
    this->Mode = pqHistogramWidget::NoMode;
    this->setCursor(Qt::ArrowCursor);
    }

  e->accept();
}

void pqHistogramWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
  if(e->button() == Qt::MidButton && this->ZoomPan)
    {
    this->ZoomPan->resetZoom();
    }

  e->accept();
}

void pqHistogramWidget::mouseMoveEvent(QMouseEvent *e)
{
  if(!this->MouseDown)
    {
    return;
    }

  // Get the current mouse position and convert it to contents coords.
  QPoint point = e->pos();
  point.rx() += this->ZoomPan->contentsX();
  point.ry() += this->ZoomPan->contentsY();

  // Check for the move wait timer. If it is active, cancel
  // the timer so it does not send a selection update.
  if(this->Mode == pqHistogramWidget::MoveWait)
    {
    this->Mode = pqHistogramWidget::NoMode;
    if(this->MoveTimer)
      {
      this->MoveTimer->stop();
      }
    }

  bool handled = true;
  pqHistogramSelectionModel *model = this->Histogram->getSelectionModel();
  if(this->Mode == pqHistogramWidget::NoMode)
    {
    // Change the cursor for the mouse mode.
    model->beginInteractiveChange();
    if(e->buttons() == Qt::LeftButton)
      {
      if(this->Interact == pqHistogramWidget::Value)
        {
        if(this->LastValueX != -1)
          {
          this->Mode = pqHistogramWidget::ValueDrag;
          }
        }
      else if(this->Interact == pqHistogramWidget::ValueMove)
        {
        if(this->LastValueX != -1)
          {
          this->Mode = pqHistogramWidget::ValueDrag;
          this->setCursor(Qt::SizeAllCursor);
          }
        }
      else
        {
        this->Mode = pqHistogramWidget::SelectBox;
        }
      }
    else if(e->buttons() == Qt::MidButton)
      {
      if(e->modifiers() & Qt::ShiftModifier)
        {
        this->Mode = pqHistogramWidget::ZoomBox;
        this->ZoomPan->setZoomCursor();
        }
      else
        {
        this->Mode = pqHistogramWidget::Zoom;
        this->ZoomPan->startInteraction(pqChartZoomPan::Zoom);
        }
      }
    else if(e->buttons() == Qt::RightButton)
      {
      this->SkipContextMenu = true;
      this->Mode = pqHistogramWidget::Pan;
      this->ZoomPan->startInteraction(pqChartZoomPan::Pan);
      }
    else
      {
      handled = false;
      }
    }

  if(this->Mode == pqHistogramWidget::ZoomBox)
    {
    QRect area = this->Mouse->Box;
    this->Mouse->adjustBox(point);

    // Repaint the zoom box. Unite the previous area with the new
    // area to ensure all the changes get repainted.
    if(area.isValid())
      {
      area = area.unite(this->Mouse->Box);
      }
    else
      {
      area = this->Mouse->Box;
      }

    area.translate(-this->ZoomPan->contentsX(), -this->ZoomPan->contentsY());
    this->viewport()->update(area);
    }
  else if(this->Mode == pqHistogramWidget::SelectBox)
    {
    // Save a copy of the old area. The union of the old and new
    // areas need to be repainted.
    QRect area = this->Mouse->Box;
    this->Mouse->adjustBox(point);
    if(area.isValid())
      {
      area = area.unite(this->Mouse->Box);
      }
    else
      {
      area = this->Mouse->Box;
      }

    // Update the selection based on the new selection box.
    if(this->Interact == pqHistogramWidget::Bin)
      {
      // Block the selection changed signals. A single signal will
      // get sent when the mouse is released.
      pqHistogramSelectionList newSelection;
      this->Histogram->getBinsIn(this->Mouse->Box, newSelection,
          this->EasyBinSelection);
      if(e->modifiers() & Qt::ShiftModifier)
        {
        if(!this->Data->Selection.isEmpty())
          {
          model->subtractSelection(this->Data->Selection);
          }

        model->addSelection(newSelection);
        }
      else if(e->modifiers() & Qt::ControlModifier)
        {
        // Use a temporary selection model to find the intersection
        // of the new selection and the previous one.
        pqHistogramSelectionModel temp;
        temp.setSelection(this->Data->Selection);
        temp.xorSelection(newSelection);
        model->xorSelection(temp.getSelection());
        }
      else
        {
        model->setSelection(newSelection);
        }

      // Adjust the repaint area to include the highlight;
      int extra = this->Histogram->getBinWidth() + 1;
      area.setRight(area.right() + extra);
      area.setLeft(area.left() - extra);
      area.setTop(0);
      area.setBottom(this->ZoomPan->contentsHeight());

      // Save the new selection in place of the old one.
      this->Data->Selection.clear();
      this->Data->Selection = newSelection;
      }

    // Repaint the affected area.
    area.translate(-this->ZoomPan->contentsX(), -this->ZoomPan->contentsY());
    this->viewport()->update(area);
    }
  else if(this->Mode == pqHistogramWidget::ValueDrag)
    {
    if(this->Interact == pqHistogramWidget::Value)
      {
      QRect area;
      area.setTop(0);
      area.setBottom(this->ZoomPan->contentsHeight());
      if(this->LastValueX < point.x())
        {
        area.setLeft(this->LastValueX);
        area.setRight(point.x());
        }
      else
        {
        area.setRight(this->LastValueX);
        area.setLeft(point.x());
        }

      pqHistogramSelectionList newSelection;
      this->Histogram->getValuesIn(area, newSelection);
      if((e->modifiers() & Qt::ControlModifier) &&
          !(e->modifiers() & Qt::ShiftModifier))
        {
        // Use a temporary selection model to find the intersection
        // of the new selection and the previous one.
        pqHistogramSelectionModel temp;
        temp.setSelection(this->Data->Selection);
        temp.xorSelection(newSelection);
        model->xorSelection(temp.getSelection());
        }
      else
        {
        model->setSelection(newSelection);
        }

      // Adjust the repaint area to include the highlight;
      int extra = this->Histogram->getBinWidth() + 1;
      area.setRight(area.right() + extra);
      area.setLeft(area.left() - extra);

      // Save the new selection in place of the old one.
      this->Data->Selection.clear();
      this->Data->Selection = newSelection;

      area.translate(-this->ZoomPan->contentsX(),
          -this->ZoomPan->contentsY());
      this->viewport()->update(area);
      }
    else if(this->Histogram && this->LastValueX != point.x())
      {
      // Get the selection range under the previous mouse position
      // and move it to the new position.
      pqHistogramSelection range;
      if(this->Histogram->getValueRangeAt(this->LastValueX, point.y(), range))
        {
        // Get the value offset.
        pqChartValue offset = this->XAxis->getValueFor(point.x());
        offset -= this->XAxis->getValueFor(this->LastValueX);
        if(offset != 0)
          {
          // Save the last position and move the selection.
          model->moveSelection(range, offset);
          if(range.getFirst() == range.getSecond())
            {
            range.moveRange(offset);
            this->LastValueX = this->XAxis->getPixelFor(range.getFirst());
            }
          else
            {
            this->LastValueX = point.x();
            }

          this->viewport()->update();
          }
        }
      }
    }
  else if(this->Mode == pqHistogramWidget::Zoom)
    {
    pqChartZoomPan::InteractFlags flags = pqChartZoomPan::ZoomBoth;
    if(e->modifiers() == Qt::ControlModifier)
      {
      flags = pqChartZoomPan::ZoomXOnly;
      }
    else if(e->modifiers() == Qt::AltModifier)
      {
      flags = pqChartZoomPan::ZoomYOnly;
      }

    this->ZoomPan->interact(e->globalPos(), flags);
    }
  else if(this->Mode == pqHistogramWidget::Pan)
    {
    this->ZoomPan->interact(e->globalPos(), pqChartZoomPan::NoFlags);
    }
  else
    {
    handled = false;
    }

  if(handled)
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void pqHistogramWidget::wheelEvent(QWheelEvent *e)
{
  pqChartZoomPan::InteractFlags flags = pqChartZoomPan::ZoomBoth;
  if(e->modifiers() == Qt::ControlModifier)
    {
    flags = pqChartZoomPan::ZoomXOnly;
    }
  else if(e->modifiers() == Qt::AltModifier)
    {
    flags = pqChartZoomPan::ZoomYOnly;
    }

  // Get the current mouse position and convert it to contents coords.
  QPoint point = e->pos();
  point.rx() += this->ZoomPan->contentsX();
  point.ry() += this->ZoomPan->contentsY();
  this->ZoomPan->handleWheelZoom(e->delta(), point, flags);

  e->accept();
}

void pqHistogramWidget::resizeEvent(QResizeEvent *)
{
  this->ZoomPan->updateContentSize();
}

void pqHistogramWidget::contextMenuEvent(QContextMenuEvent *e)
{
  e->accept();
}

bool pqHistogramWidget::viewportEvent(QEvent *e)
{
  if(e->type() == QEvent::ContextMenu && this->SkipContextMenu)
    {
    this->SkipContextMenu = false;
    e->accept();
    return true;
    }

  return QAbstractScrollArea::viewportEvent(e);
}

void pqHistogramWidget::draw(QPainter& painter, QRect area)
{
  painter.translate(-this->ZoomPan->contentsX(), -this->ZoomPan->contentsY());
  area.translate(this->ZoomPan->contentsX(), this->ZoomPan->contentsY());
  painter.setClipRect(area);

  // Paint the widget background.
  painter.fillRect(area, this->BackgroundColor);

  // Paint the highlight background.
  this->Histogram->drawBackground(&painter, area);

  // Draw in the axes and grid.
  this->FAxis->drawAxis(&painter, area);
  this->YAxis->drawAxis(&painter, area);
  this->XAxis->drawAxis(&painter, area);

  // Paint the histogram.
  this->Histogram->drawChart(&painter, area);

  // Paint the line chart on top of the histogram.
  this->LineChart->drawChart(painter, area);

  // Draw in the axis lines again to ensure they are on top.
  this->FAxis->drawAxisLine(&painter);
  this->YAxis->drawAxisLine(&painter);
  this->XAxis->drawAxisLine(&painter);

  // Draw the chart title
  this->Title->draw(painter, area);

  if(this->Mouse->Box.isValid())
    {
    // Draw in mouse box selection or zoom if needed.
    painter.setPen(Qt::black);
    painter.setPen(Qt::DotLine);
    if(this->Mode == pqHistogramWidget::ZoomBox ||
        this->Mode == pqHistogramWidget::SelectBox)
      {
      painter.drawRect(this->Mouse->Box.x(), this->Mouse->Box.y(),
          this->Mouse->Box.width() - 1, this->Mouse->Box.height() - 1);
      }
    }
}

void pqHistogramWidget::printChart(QPrinter& printer)
{
  QSize viewport_size(this->rect().size());
  viewport_size.scale(printer.pageRect().size(), Qt::KeepAspectRatio);

  QPainter painter(&printer);
  painter.setWindow(this->rect());
  painter.setViewport(QRect(0, 0, viewport_size.width(), viewport_size.height()));

  this->draw(painter, this->rect());
}

void pqHistogramWidget::savePDF(const QStringList& files)
{
  for(int i = 0; i != files.size(); ++i)
    {
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(files[i]);
    
    this->printChart(printer);
    }
}

void pqHistogramWidget::savePNG(const QStringList& files)
{
  const QPixmap grab = QPixmap::grabWidget(this);
  for(int i = 0; i != files.size(); ++i)
    {
    grab.save(files[i], "PNG");
    }
}
