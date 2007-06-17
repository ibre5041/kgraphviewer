/* This file is part of KGraphViewer.
   Copyright (C) 2005 Gaël de Chalendar <kleag@free.fr>

   KGraphViewer is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

/* This file was callgraphview.cpp, part of KCachegrind.
   Copyright (C) 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

   KCachegrind is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.
*/


/*
 * Callgraph View
 */

#include <stdlib.h>
#include <math.h>
#include <iostream>

#include <QMatrix>
#include <QPainter>
#include <QStyle>
#include <QImage>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QPixmap>
#include <QResizeEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QGraphicsSimpleTextItem>
#include <QScrollBar>

#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>
//#include <ktempfile.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kprinter.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <krun.h>
#include <kcursor.h>
#include <kselectaction.h>
#include <ktoggleaction.h>
#include <kstandarddirs.h>
#include <kactionmenu.h>
#include <kconfiggroup.h>
    
#include "dotgraphview.h"
#include "dotgraph.h"
#include "pannerview.h"
#include "canvassubgraph.h"
#include "canvasedge.h"
#include "dot2qtconsts.h"
#include "graphnode.h"
#include "canvasnode.h"
#include "graphedge.h"
#include "FontsCache.h"
#include "kgraphviewer_partsettings.h"
#include "simpleprintingcommand.h"

// DotGraphView defaults

#define DEFAULT_ZOOMPOS      Auto
#define KGV_MAX_PANNER_NODES 100

using namespace KGraphViewer;
//
// DotGraphView
//
DotGraphView::DotGraphView(KActionCollection* actions, QWidget* parent) : 
    QGraphicsView(parent), 
    m_labelViews(),
    m_popup(0),
    m_zoom(1),
    m_isMoving(false),
    m_exporter(),
    m_zoomPosition(DEFAULT_ZOOMPOS), 
    m_lastAutoPosition(DotGraphView::TopLeft),
    m_graph(0),
    m_focusedNode(0),
    m_printCommand(0),
    m_actions(actions)
{
  m_canvas = 0;
  m_xMargin = m_yMargin = 0;
  m_birdEyeView = new PannerView(this);
  m_cvZoom = 1;

  m_exporter.setGraphOptions(this);

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  
  m_birdEyeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_birdEyeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_birdEyeView->raise();
  m_birdEyeView->hide();

  setFocusPolicy(Qt::StrongFocus);
  setBackgroundMode(Qt::NoBackground);
//   viewport()->setMouseTracking(true);
  
  connect(m_birdEyeView, SIGNAL(zoomRectMovedTo(QPointF)),
          this, SLOT(zoomRectMovedTo(QPointF)));
  connect(m_birdEyeView, SIGNAL(zoomRectMoveFinished()),
          this, SLOT(zoomRectMoveFinished()));

  setWhatsThis( i18n( 
    "<h1>GraphViz dot format graph visualization</h1>"
    "<p>If the graph is larger than the widget area, an overview "
        "panner is shown in one edge. Choose throug the context menu "
        "if the optimal position of this overview should be automatically "
        "computed or put it where you want.</p>"
    "<h2>How to work with it ?</h2>"
    "<ul>"
        "<li>To move the graph, you can:"
            "  <ul>"
                "    <li>click & drag it</li>"
                "    <li>use the elevators</li>"
                "    <li>press the arrows keys</li>"
                "    <li>click somewhere in the panner view</li>" 
                "    <li>use the mouse wheel (up and down with no modifier, left and right with the <Alt> key pressed)</li>"
                    "    <li>or click & drag the panner view</li>"
                    "  </ul>"
                "</li>"
            "<li>To zoom, you can either use the zoom in and zoom out toolbar butons or click on the <Shift> key while rolling your mouse wheel.</li>"
                "<li>Try the contextual menu (usually by right-clicking) to discover other "
                    "possibilities.</li>"
                "<li>Try the <tt>Print preview</tt> or the <tt>Page setup</tt> buttons to explore the printing options.</li>"
                "</ul>"
            ) );

  readViewConfig();
  
  QMatrix m;
  m.scale(m_zoom,m_zoom);
  setMatrix(m);
  setupPopup();
  setInteractive(true);
  setDragMode(ScrollHandDrag);
//   setDragMode(NoDrag);
  setRenderHint(QPainter::Antialiasing);
}

DotGraphView::~DotGraphView()
{
  saveViewConfig();
  delete m_birdEyeView;
  if (m_popup != 0)
  {
    delete m_popup;
  }
  if (m_canvas) 
  {
    setScene(0);
    delete m_canvas;
  }
  if (m_graph != 0)
  {
    delete m_graph;
  }
}

bool DotGraphView::loadDot(const QString& dotFileName)
{
  kDebug() << "DotGraphView::loadDot '" << dotFileName << "'" << endl;
  hide();
//   disconnect(m_canvas,SIGNAL(changed()),m_birdEyeView,SLOT(updateScene()));
  m_birdEyeView->setScene(0);
//   disconnect(m_canvas,SIGNAL(changed()),this,SLOT(updateScene()));
  setScene(0);
  
  if (m_canvas) 
  {
    delete m_canvas;
    m_canvas = 0;
  }
  
  QString layoutCommand = (m_graph!=0?m_graph->layoutCommand():"");
  if (m_graph != 0)
    delete m_graph;
  m_graph = new DotGraph(layoutCommand,dotFileName);
  if (layoutCommand == "")
  {
    layoutCommand = m_graph->chooseLayoutProgramForFile(m_graph->dotFileName());
  }
  m_graph->layoutCommand(layoutCommand);
  
//   std::cerr << "Parsing " << m_graph->dotFileName() << " with " << m_graph->layoutCommand() << std::endl;
  if (m_graph->parseDot(m_graph->dotFileName()))
  {
//     kDebug() << "Successfully parsed!" << endl;
  }
  else
  {
    kError() << "NOT successfully parsed!" << endl;
    return false;
  }
  
  viewport()->setUpdatesEnabled(false);
  
  if (m_graph->nodes().size() > KGV_MAX_PANNER_NODES)
  {
    m_birdEyeView->setDrawingEnabled(false);
  }
  //  QCanvasEllipse* eItem;
  double scaleX = 1.0, scaleY = 1.0;

  if (_detailLevel == 0)      { scaleX = m_graph->scale() * 0.7; scaleY = m_graph->scale() * 0.7; }
  else if (_detailLevel == 1) { scaleX = m_graph->scale() * 1.0; scaleY = m_graph->scale() * 1.0; }
  else if (_detailLevel == 2) { scaleX = m_graph->scale() * 1.3; scaleY = m_graph->scale() * 1.3; }
  else                        { scaleX = m_graph->scale() * 1.0; scaleY = m_graph->scale() * 1.0; }
  
  int gh = int(m_graph->height());
//   kDebug() << "detail level = " << _detailLevel << " ; scaleX = " << scaleX << endl;
  QGraphicsScene* newCanvas;
    int w = (int)(scaleX * m_graph->width() / m_graph->horizCellFactor());
    int h = (int)(scaleY * gh / m_graph->vertCellFactor());
    
    m_xMargin = 50;
    m_yMargin = 50;
    

    newCanvas = new QGraphicsScene(0,0,w+2*m_xMargin, h+2*m_yMargin);
    std::cerr << "Created canvas " << newCanvas << std::endl;
    newCanvas->setBackgroundBrush(QBrush(QColor(m_graph->backgroundColor())));
    m_canvasNaturalSize = newCanvas->sceneRect().size();
    QSizeF newCanvasSize = newCanvas->sceneRect().size();
  
//   std::set< GraphNode* >& nodes = m_graph->nodesOfCell(0);
  GraphNodeMap& nodes = m_graph->nodes();
//   std::cerr << "Adding " << nodes.size() << " nodes to canvas" << std::endl;
  uint nbNodesShown = 0;
  GraphNodeMap::iterator nodesIt, nodesIt_end;
  nodesIt = nodes.begin();
  nodesIt_end = nodes.end();
  for (; nodesIt != nodesIt_end; nodesIt++)
  {
    GraphNode* gnode = (*nodesIt);
    
    DotRenderOpVec::const_iterator it, it_end;
    it = gnode->renderOperations().begin(); it_end = gnode->renderOperations().end();
    for (; it != it_end; it++)
    {
      if ((*it).renderop != "T" || gnode->shape() == "plaintext")
      {
        CanvasNode *cnode = CanvasNode::dotShapedCanvasNode(
          *it, gnode->renderOperations(), this, gnode, newCanvas, 
          scaleX, scaleY, m_xMargin, m_yMargin, gh, 
          int(m_graph->wdhcf()), int(m_graph->hdvcf()));
        if (cnode == 0) continue;
        
        
        gnode->setCanvasNode(cnode);
        if (dynamic_cast<QGraphicsPolygonItem*>(cnode) != 0)
        {
//           std::cerr << "setting z = " << gnode->z() << std::endl;
          dynamic_cast<QGraphicsPolygonItem*>(cnode)->setZValue(gnode->z());
          dynamic_cast<QGraphicsPolygonItem*>(cnode)->show();
          nbNodesShown++;
        }
        break;
      }
    }
  }
//   std::cerr << "  " << nbNodesShown << " nodes shown" << std::endl;
  
  uint nbEdgesShown = 0;
  GraphEdgeMap::iterator edgesIt, edgesIt_end;
  edgesIt = m_graph->edges().begin();
  edgesIt_end = m_graph->edges().end();
  for (; edgesIt != edgesIt_end; edgesIt++)
  {
    GraphEdge* gedge = (*edgesIt).second;
    
/*    if (!( ( m_graph->nodesOfCell(0).find (e->fromNode()) != m_graph->nodesOfCell(0).end() )
           && ( m_graph->nodesOfCell(0).find (e->toNode()) != m_graph->nodesOfCell(0).end() ) ))
      continue;*/
    
    CanvasEdge* cedge = new CanvasEdge(gedge, newCanvas,scaleX, scaleY, m_xMargin, m_yMargin, gh, 
                           int(m_graph->wdhcf()), int(m_graph->hdvcf()));
    
    gedge->setCanvasEdge(cedge);
//     std::cerr << "setting z = " << gedge->z() << std::endl;
    cedge->setZValue(gedge->z());
    cedge->show();
    nbEdgesShown++;
  }
//   std::cerr << "  " << nbEdgesShown << " edges shown" << std::endl;
  
  uint nbSubgraphsShown = 0;
  GraphSubgraphMap::iterator subgraphsIt, subgraphsIt_end;
  subgraphsIt = m_graph->subgraphs().begin();
  subgraphsIt_end = m_graph->subgraphs().end();
  for (; subgraphsIt != subgraphsIt_end; subgraphsIt++)
  {
    GraphSubgraph* gsubgraph = (*subgraphsIt);
    
/*    if (!( ( m_graph->nodesOfCell(0).find (e->fromNode()) != m_graph->nodesOfCell(0).end() )
    && ( m_graph->nodesOfCell(0).find (e->toNode()) != m_graph->nodesOfCell(0).end() ) ))
    continue;*/
    
    CanvasSubgraph* csubgraph = new CanvasSubgraph(this, gsubgraph, 
        newCanvas,scaleX, scaleY, m_xMargin, m_yMargin, gh, 
        int(m_graph->wdhcf()), int(m_graph->hdvcf()));
    
    csubgraph->show();
    nbSubgraphsShown++;
  }

//   std::cerr << "Adding graph render operations: " << m_graph->renderOperations().size() << std::endl;
  DotRenderOpVec::const_iterator graphRenderOpsIt, graphRenderOpsIt_end;
  graphRenderOpsIt = m_graph->renderOperations().begin();
  graphRenderOpsIt_end = m_graph->renderOperations().end();
  for (; graphRenderOpsIt != graphRenderOpsIt_end; graphRenderOpsIt++)
  {
    const DotRenderOp& dro = (*graphRenderOpsIt);
    if ( dro.renderop == "T" )
    {
//       std::cerr << "Adding graph label '"<<dro.str<<"'" << std::endl;
      QString str = QString::fromUtf8(dro.str.c_str());
      int stringWidthGoal = int(dro.integers[3] * scaleX);
      int fontSize = m_graph->fontSize();
      QFont* font = FontsCache::changeable().fromName(m_graph->fontName());
      font->setPointSize(fontSize);
      QFontMetrics fm(*font);
      while (fm.width(str) > stringWidthGoal && fontSize > 1)
      {
        fontSize--;
        font->setPointSize(fontSize);
        fm = QFontMetrics(*font);
      }
      QGraphicsSimpleTextItem* labelView = new QGraphicsSimpleTextItem(str, 0, newCanvas);
      labelView->setFont(*font);
      labelView->setPos(
                  (scaleX * 
                       (
                         (dro.integers[0]) 
                         + (((dro.integers[2])*(dro.integers[3]))/2)
                         - ( (dro.integers[3])/2 )
                       )
                      + m_xMargin ),
                      ((gh - (dro.integers[1]))*scaleY)+ m_yMargin);
      /// @todo port that ; how to set text color ?
      labelView->setPen(QPen(Dot2QtConsts::componentData().qtColor(m_graph->fontColor())));
      labelView->setFont(*font);
      m_labelViews.insert(labelView);
    }
  }
  
  m_birdEyeView->setScene(newCanvas);
//   std::cerr << "After m_birdEyeView set canvas" << std::endl;
  
  setScene(newCanvas);
  m_canvas = newCanvas;
//   std::cerr << "After set canvas" << std::endl;
  

  m_cvZoom = 0;
  updateSizes();
  zoomRectMovedTo(QPointF(0,0));

  viewport()->setUpdatesEnabled(true);
  show();
  std::set<QGraphicsSimpleTextItem*>::iterator labelViewsIt, labelViewsIt_end;
  labelViewsIt = m_labelViews.begin(); labelViewsIt_end = m_labelViews.end();
  for (; labelViewsIt != labelViewsIt_end; labelViewsIt++)
  {
    (*labelViewsIt)->show();
  }
  m_canvas->update();
  return true;
}


void DotGraphView::updateSizes(QSizeF s)
{
  kDebug() << "DotGraphView::updateSizes" << endl;
  if (m_canvas == 0)
    return;
  if (s == QSizeF(0,0)) s = size();

  // the part of the canvas that should be visible
  int cWidth  = int((m_canvas->width()  - 2*m_xMargin + 100));
  int cHeight = int((m_canvas->height() - 2*m_yMargin + 100));

  // hide birds eye view if no overview needed
  if (//!_data || !_activeItem ||
      !KGraphViewerPartSettings::birdsEyeViewEnabled() ||
      (((cWidth * m_zoom) < s.width()) && (cHeight * m_zoom) < s.height())) 
  {
    m_birdEyeView->hide();
    return;
  }
  m_birdEyeView->hide();

  // first, assume use of 1/3 of width/height (possible larger)
  double zoom = .33 * s.width() / cWidth;
  if (zoom * cHeight < .33 * s.height()) zoom = .33 * s.height() / cHeight;

  // fit to widget size
  if (cWidth  * zoom  > s.width())   zoom = s.width() / (double)cWidth;
  if (cHeight * zoom  > s.height())  zoom = s.height() / (double)cHeight;

  // scale to never use full height/width
  zoom = zoom * 3/4;

  // at most a zoom of 1/3
  if (zoom > .33) zoom = .33;

  if (zoom != m_cvZoom) 
  {
    m_cvZoom = zoom;

    QMatrix wm;
    wm.scale( zoom, zoom );
    m_birdEyeView->setMatrix(wm);

    // make it a little bigger to compensate for widget frame
    m_birdEyeView->resize(int(cWidth * zoom) + 4,
                          int(cHeight * zoom) + 4);

  }

  int cvW = m_birdEyeView->width();
  int cvH = m_birdEyeView->height();
  int x = width()- cvW - verticalScrollBar()->width()    -2;
  int y = height()-cvH - horizontalScrollBar()->height() -2;
  QPoint oldZoomPos = m_birdEyeView->pos();
  QPoint newZoomPos = QPoint(0,0);
  ZoomPosition zp = m_zoomPosition;
  if (zp == Auto) 
  {
    QPointF tl1Pos = mapToScene(QPoint(0,0));
    QPointF tl2Pos = mapToScene(QPoint(cvW,cvH));
    QPointF tr1Pos = mapToScene(QPoint(x,0));
    QPointF tr2Pos = mapToScene(QPoint(x+cvW,cvH));
    QPointF bl1Pos = mapToScene(QPoint(0,y));
    QPointF bl2Pos = mapToScene(QPoint(cvW,y+cvH));
    QPointF br1Pos = mapToScene(QPoint(x,y));
    QPointF br2Pos = mapToScene(QPoint(x+cvW,y+cvH));
    int tlCols = m_canvas->items(QRectF(tl1Pos.x(),tl1Pos.y(),tl2Pos.x(),tl2Pos.y())).count();
    int trCols = m_canvas->items(QRectF(tr1Pos.x(),tr1Pos.y(),tr2Pos.x(),tr2Pos.y())).count();
    int blCols = m_canvas->items(QRectF(bl1Pos.x(),bl1Pos.y(),bl2Pos.x(),bl2Pos.y())).count();
    int brCols = m_canvas->items(QRectF(br1Pos.x(),br1Pos.y(),br2Pos.x(),br2Pos.y())).count();
    int minCols = tlCols;
    zp = m_lastAutoPosition;
    switch(zp) 
    {
      case DotGraphView::TopRight:    minCols = trCols; break;
      case DotGraphView::BottomLeft:  minCols = blCols; break;
      case DotGraphView::BottomRight: minCols = brCols; break;
      default:
      case DotGraphView::TopLeft:     minCols = tlCols; break;
    }
    if (minCols > tlCols) { minCols = tlCols; zp = DotGraphView::TopLeft; }
    if (minCols > trCols) { minCols = trCols; zp = DotGraphView::TopRight; }
    if (minCols > blCols) { minCols = blCols; zp = DotGraphView::BottomLeft; }
    if (minCols > brCols) { minCols = brCols; zp = DotGraphView::BottomRight; }
  
    m_lastAutoPosition = zp;
  }

  switch(zp) 
  {
    case DotGraphView::TopRight:
      newZoomPos = QPoint(x,0);
    break;
    case DotGraphView::BottomLeft:
      newZoomPos = QPoint(0,y);
    break;
    case DotGraphView::BottomRight:
      newZoomPos = QPoint(x,y);
    break;
    default:
    break;
  }
  if (newZoomPos != oldZoomPos) 
    m_birdEyeView->move(newZoomPos);
  m_birdEyeView->show();
  QSizeF newCanvasSize = m_canvas->sceneRect().size();
  if (newCanvasSize.width() < viewport()->width())
  {
    newCanvasSize.setWidth(viewport()->width());
  }
  else if (viewport()->width() < m_canvasNaturalSize.width())
  {
    newCanvasSize.setWidth(m_canvasNaturalSize.width());
  }
  if (newCanvasSize.height() < viewport()->height())
  {
    newCanvasSize.setHeight(viewport()->height());
  }
  else if (viewport()->height() < m_canvasNaturalSize.height())
  {
    newCanvasSize.setHeight(m_canvasNaturalSize.height());
  }
//   std::cerr << "done." << std::endl;
}

void DotGraphView::focusInEvent(QFocusEvent*)
{
  if (!m_canvas) return;

//   m_canvas->update();
}

void DotGraphView::focusOutEvent(QFocusEvent* e)
{
  // trigger updates as in focusInEvent
  focusInEvent(e);
}

void DotGraphView::keyPressEvent(QKeyEvent* e)
{
  if (!m_canvas) 
  {
    e->ignore();
    return;
  }

  // move canvas...
  if (e->key() == Qt::Key_Home)
    scrollContentsBy(int(-m_canvas->width()),0);
  else if (e->key() == Qt::Key_End)
    scrollContentsBy(int(m_canvas->width()),0);
  else if (e->key() == Qt::Key_Prior)
    scrollContentsBy(0,-viewport()->height()/2);
  else if (e->key() == Qt::Key_Next)
    scrollContentsBy(0,viewport()->height()/2);
  else if (e->key() == Qt::Key_Left)
    scrollContentsBy(-viewport()->width()/10,0);
  else if (e->key() == Qt::Key_Right)
    scrollContentsBy(viewport()->width()/10,0);
  else if (e->key() == Qt::Key_Down)
    scrollContentsBy(0,viewport()->height()/10);
  else if (e->key() == Qt::Key_Up)
    scrollContentsBy(0,-viewport()->height()/10);
  else 
  {
    e->ignore();
    return;
  }
}

void DotGraphView::wheelEvent(QWheelEvent* e)
{
  if (!m_canvas) 
  {
    e->ignore();
    return;
  }
  e->accept();
  if (e->state() == Qt::ShiftModifier)
  {
    // move canvas...
    if (e->delta() < 0)
    {
      zoomOut();
    }
    else 
    {
      zoomIn();
    }
  }
  else
  {
    if (e->orientation() == Qt::Horizontal)
    {
      if (e->delta() < 0)
      {
        scrollContentsBy(-viewport()->width()/10,0);
      }
      else
      {
        scrollContentsBy(viewport()->width()/10,0);
      }
    }
    else
    {
      if (e->delta() < 0)
      {
        scrollContentsBy(0,viewport()->height()/10);
      }
      else
      {
        scrollContentsBy(0,-viewport()->height()/10);
      }
    }
  }
}

void DotGraphView::zoomIn()
{
  applyZoom(1.10);
}


void DotGraphView::zoomOut()
{
  applyZoom(.90);
}

void DotGraphView::applyZoom(double factor)
{
  double newZoom = m_zoom * factor;
  if (newZoom < 0.1 || newZoom > 10)
    return;
  m_zoom = newZoom;
  if (m_zoom > 1.0 && m_zoom < 1.1) 
  {
    m_zoom = 1;
  }
  qreal centerX = (sceneRect().x() + (viewport()->width() / 2))*factor;
  qreal centerY = (sceneRect().y() + (viewport()->height() / 2))*factor;
  
  setUpdatesEnabled(false);
  QMatrix m;
  m.scale(m_zoom,m_zoom);
  setMatrix(m);
  centerOn(centerX, centerY);
  emit zoomed(m_zoom);
  setUpdatesEnabled(true);
  updateSizes();
}

void DotGraphView::resizeEvent(QResizeEvent* e)
{
  kDebug() << "resizeEvent" << endl;
  QGraphicsView::resizeEvent(e);
  if (m_canvas) updateSizes(e->size());
//   std::cerr << "resizeEvent end" << std::endl;
}

void DotGraphView::zoomRectMovedTo(QPointF newZoomPos)
{
  kDebug() << "DotGraphView::zoomRectMovedTo " << newZoomPos << endl;
  centerOn(newZoomPos);
  kDebug() << "  viewport "<< mapToScene(viewport()->rect()).boundingRect() << endl;
  QRectF sp = mapToScene(viewport()->rect()).boundingRect();
/*  QPointF p = newZoomPos - sp.bottomRight()/2;
  if (p.x() < 0) 
  {
    p.rx() = 0;
  }
  if (p.y() < 0) 
  {
    p.ry() = 0;
  }
  sp.setX(p.x()); sp.setY(p.y());*/
  kDebug() << "  sp "<< sp /*<< " ; p " << p*/ << endl;
  m_birdEyeView->setZoomRect(sp);
}
                    
void DotGraphView::zoomRectMoveFinished()
{
   kDebug() << "zoomRectMoveFinished" << endl;
    if (m_zoomPosition == Auto) updateSizes();
//   std::cerr << "zoomRectMoveFinished end" << std::endl;
}

void DotGraphView::mousePressEvent(QMouseEvent* e)
{
  kDebug() << "mousePressEvent" << e << endl;
//   setDragMode(ScrollHandDrag);
  QGraphicsView::mousePressEvent(e);

  m_isMoving = true;
}

void DotGraphView::mouseMoveEvent(QMouseEvent* e)
{
//   kDebug() << "mouseMoveEvent" << e << endl;
  QGraphicsView::mouseMoveEvent(e);
  if (m_isMoving) 
  {
    QRectF sp = mapToScene(viewport()->rect()).boundingRect();
    m_birdEyeView->setZoomRect(sp);
  }
}

void DotGraphView::mouseReleaseEvent(QMouseEvent* e)
{
  kDebug() << "mouseReleaseEvent" << e << endl;
//   kDebug() << "setDragMode(NoDrag)" << endl;
//   setDragMode(NoDrag);
  QGraphicsView::mouseReleaseEvent(e);
  m_isMoving = false;

  if (m_focusedNode != 0 && !m_focusedNode->node()->url().isEmpty())
  {
    KUrl url(m_focusedNode->node()->url());
    new KRun(url,0);
  }
}

void DotGraphView::mouseDoubleClickEvent(QMouseEvent* e)
{
  QGraphicsView::mouseDoubleClickEvent(e);
}

void DotGraphView::contextMenuEvent(QContextMenuEvent* e)
{
//   QList<QGraphicsItem *> l = scene()->collidingItems(scene()->itemAt(e->pos()));

  m_popup->exec(e->globalPos());
}

void DotGraphView::setLayoutCommand(const QString& command)
{
  m_graph->layoutCommand(command);
  reload();
}

DotGraphView::ZoomPosition DotGraphView::zoomPos(QString s)
{
  DotGraphView::ZoomPosition  res = DEFAULT_ZOOMPOS;
  if (s == QString("DotGraphView::TopLeft")) res = DotGraphView::TopLeft;
  if (s == QString("DotGraphView::TopRight")) res = DotGraphView::TopRight;
  if (s == QString("DotGraphView::BottomLeft")) res = DotGraphView::BottomLeft;
  if (s == QString("DotGraphView::BottomRight")) res = DotGraphView::BottomRight;
  if (s == QString("Automatic")) res = Auto;

  return res;
}

void DotGraphView::viewBevActivated(int newZoomPos)
{
  m_zoomPosition = (ZoomPosition)newZoomPos;
  updateSizes();
  emit(sigViewBevActivated(newZoomPos));
}

QString DotGraphView::zoomPosString(ZoomPosition p)
{
    if (p == DotGraphView::TopRight) return QString("DotGraphView::TopRight");
    if (p == DotGraphView::BottomLeft) return QString("DotGraphView::BottomLeft");
    if (p == DotGraphView::BottomRight) return QString("DotGraphView::BottomRight");
    if (p == Auto) return QString("Automatic");

    return QString("DotGraphView::TopLeft");
}

void DotGraphView::readViewConfig()
{
  KConfigGroup g(KGlobal::config(),"GraphViewLayout");
  
  QVariant dl = DEFAULT_DETAILLEVEL;
  _detailLevel     = g.readEntry("DetailLevel", dl).toInt();
  _layout          = GraphOptions::layout(g.readEntry("Layout",
            layoutString(DEFAULT_LAYOUT)));
  m_zoomPosition  = zoomPos(g.readEntry("ZoomPosition",
            zoomPosString(DEFAULT_ZOOMPOS)));
  emit(sigViewBevActivated(m_zoomPosition));
}

void DotGraphView::saveViewConfig()
{
//   kDebug() << "Saving view config" << endl;  
  KConfigGroup g(KGlobal::config(), "GraphViewLayout");

    writeConfigEntry(&g, "DetailLevel", _detailLevel, DEFAULT_DETAILLEVEL);
    writeConfigEntry(&g, "Layout",
         layoutString(_layout), layoutString(DEFAULT_LAYOUT).utf8().data());
    writeConfigEntry(&g, "ZoomPosition",
         zoomPosString(m_zoomPosition),
         zoomPosString(DEFAULT_ZOOMPOS).utf8().data());
  g.sync();
}

void DotGraphView::pageSetup()
{
  if (m_printCommand == 0)
  {
    m_printCommand = new KGVSimplePrintingCommand(this, 0);
  }
  m_printCommand->showPageSetup(m_graph->dotFileName());
  return;
}

void DotGraphView::print()
{
  if (m_printCommand == 0)
  {
    m_printCommand = new KGVSimplePrintingCommand(this, 0);
  }
  m_printCommand->print(m_graph->dotFileName());
  return;
}

void DotGraphView::printPreview()
{
  if (m_printCommand == 0)
  {
    m_printCommand = new KGVSimplePrintingCommand(this, 0);
  }
  m_printCommand->showPrintPreview(m_graph->dotFileName(), false);
  return;
}

bool DotGraphView::reload()
{
  QString fileName = m_graph->dotFileName();
  return loadDot(fileName);
}

void DotGraphView::dirty(const QString& dotFileName)
{
//   std::cerr << "SLOT dirty for " << dotFileName << std::endl;
  if (dotFileName == m_graph->dotFileName())
  {
    if (KMessageBox::questionYesNo(this, 
                                i18n("The file %1 has been modified on disk.\nDo you want to reload it ?",dotFileName),
                                i18n("Reload Confirmation"),
                                KStandardGuiItem::yes(),
                                KStandardGuiItem::no(),
                                "reloadOnChangeMode"   ) == KMessageBox::Yes)
    {
      loadDot(dotFileName);
    }
  }
}

KConfigGroup* DotGraphView::configGroup(KConfig* c,
                                         QString group, QString post)
{
  QStringList gList = c->groupList();
  if (gList.contains((group+post).ascii()) ) group += post;
  return new KConfigGroup(c, group);
}

void DotGraphView::writeConfigEntry(KConfigGroup* c, const char* pKey,
                                     QString value, const char* def)
{
  if (!c) return;
  if ((value.isEmpty() && ((def == 0) || (*def == 0))) ||
      (value == QString(def)))
    c->deleteEntry(pKey);
  else
    c->writeEntry(pKey, value);
}

void DotGraphView::writeConfigEntry(KConfigGroup* c, const char* pKey,
                                     int value, int def)
{
  if (!c) return;
  if (value == def)
    c->deleteEntry(pKey);
  else
    c->writeEntry(pKey, value);
}

void DotGraphView::writeConfigEntry(KConfigGroup* c, const char* pKey,
                                     double value, double def)
{
  if (!c) return;
  if (value == def)
    c->deleteEntry(pKey);
  else
    c->writeEntry(pKey, value);
}

void DotGraphView::writeConfigEntry(KConfigGroup* c, const char* pKey,
                                     bool value, bool def)
{
  if (!c) return;
  if (value == def)
    c->deleteEntry(pKey);
  else
    c->writeEntry(pKey, value);
}

const QString& DotGraphView::dotFileName() 
{
  return m_graph->dotFileName();
}

void DotGraphView::hideToolsWindows()
{
  if (m_printCommand != 0)
  {
    m_printCommand->hidePageSetup();
    m_printCommand->hidePrintPreview();
  }
}

void DotGraphView::setupPopup()
{
  if (m_popup != 0)
  {
    return;
  }
  kDebug() << "DotGraphView::setupPopup" << endl;
  m_popup = new QMenu();

  m_layoutAlgoSelectAction = new KSelectAction(i18n("Select Layout Algo"),this);
  actionCollection()->addAction("view_layout_algo",m_layoutAlgoSelectAction);
  QStringList layoutAlgos;
  KAction* lea = new KAction(i18n(" "), this);
  actionCollection()->addAction("layout_specifiy",lea);
  lea->setCheckable(false);
  KAction* lda = new KAction(i18n("Dot"), this);
  actionCollection()->addAction("layout_dot",lda);
  lda->setCheckable(false);
  KAction* lna = new KAction(i18n("Neato"), this);
  actionCollection()->addAction("layout_neato",lna);
  lna->setCheckable(false);
  KAction* lta = new KAction(i18n("Twopi"), this);
  actionCollection()->addAction("layout_twopi",lta);
  lta->setCheckable(false);
  KAction* lfa = new KAction(i18n("Fdp"), this);
  actionCollection()->addAction("layout_fdp",lfa);
  lfa->setCheckable(false);
  KAction* lca = new KAction(i18n("Circo"), this);
  actionCollection()->addAction("layout_c",lca);
  lca->setCheckable(false);
  m_layoutAlgoSelectAction->addAction(lea);
  m_layoutAlgoSelectAction->addAction(lda);
  m_layoutAlgoSelectAction->addAction(lna);
  m_layoutAlgoSelectAction->addAction(lta);
  m_layoutAlgoSelectAction->addAction(lfa);
  m_layoutAlgoSelectAction->addAction(lca);

  m_layoutAlgoSelectAction->setCurrentItem(1);
  m_layoutAlgoSelectAction->setEditable(true);
  m_layoutAlgoSelectAction->setToolTip(i18n("Choose a GraphViz layout algorithm or edit your own one."));  
  m_layoutAlgoSelectAction->setWhatsThis(i18n(
  "Choose a GraphViz layout algorithm or type in your own command that will "
  "generate a graph in the xdot format on its standard output. For example, to"
  "manually specify the <tt>G</tt> option to the dot command, type in: "
  "<tt>dot -Gname=MyGraphName -Txdot </tt>"));  
  connect(m_layoutAlgoSelectAction, SIGNAL(triggered (const QString &)),
          this, SLOT(slotSelectLayoutAlgo(const QString&)));

  
  QMenu* layoutPopup = m_popup->addMenu(i18n("Layout"));
  layoutPopup->addAction(m_layoutAlgoSelectAction);
  layoutPopup->addAction(i18n("Specify layout command"), this, SLOT(slotLayoutSpecify()));
  layoutPopup->addAction(i18n("Reset layout command to default"), this, SLOT(slotLayoutReset()));
  
                        
  m_popup->insertSeparator();
  
  KActionMenu* file_exportMenu = new KActionMenu(i18n("Export Graph"), this);
  actionCollection()->addAction("file_export",file_exportMenu);
  connect(file_exportMenu,SIGNAL(activated(int)),this,SLOT(fileExportActivated(int)));
  file_exportMenu->setToolTip(i18n("Allows to export the graph to another format."));  
  file_exportMenu->setWhatsThis(i18n(
  "Use the Export Graph menu to export the graph to another format."
  "There is currently only on export format supported: PNG image."));  
  

  m_popup->addAction(file_exportMenu);
  KAction* exportToImageAction = new KAction(i18n("As Image ..."),this);
  actionCollection()->addAction("export_image", exportToImageAction);
  connect(exportToImageAction,SIGNAL(triggered(bool)), this, SLOT(slotExportImage()));
  
  file_exportMenu->addAction(exportToImageAction);

  
  m_popup->insertSeparator();

  m_bevEnabledAction = new KToggleAction(
          KIcon(KGlobal::dirs()->findResource("appdata","pics/kgraphviewer-bev.png")), 
          i18n("Enable Bird's-eye View"), this);
  actionCollection()->addAction("view_bev_enabled",m_bevEnabledAction);
  m_bevEnabledAction->setShortcut(Qt::CTRL+Qt::Key_B);
  m_bevEnabledAction->setWhatsThis(i18n("Enables or disables the Bird's-eye View"));
  connect(m_bevEnabledAction, 
          SIGNAL(toggled(bool)),
          this,
          SLOT(slotBevEnabled()));
  m_bevEnabledAction->setCheckable(true);
  m_popup->addAction(m_bevEnabledAction);
  
  m_bevPopup = new KSelectAction(i18n("Birds-eye View"), this);
  m_popup->addAction(m_bevPopup);
  actionCollection()->addAction("view_bev",m_bevPopup);

  KAction* btla = new KAction(i18n("Top Left"), this);
  btla->setCheckable(true);
  actionCollection()->addAction("bev_top_left",btla);
  connect(btla, SIGNAL(triggered (Qt::MouseButtons, Qt::KeyboardModifiers)), 
          this, SLOT(slotBevTopLeft()));
  KAction* btra = new KAction(i18n("Top Right"), this);
  btra->setCheckable(true);
  actionCollection()->addAction("bev_top_right",btra);
  connect(btra, SIGNAL(triggered (Qt::MouseButtons, Qt::KeyboardModifiers)), 
          this, SLOT(slotBevTopRight()));
  KAction* bbla = new KAction(i18n("Bottom Left"), this);
  bbla->setCheckable(true);
  actionCollection()->addAction("bev_bottom_left",bbla);
  connect(bbla, SIGNAL(triggered (Qt::MouseButtons, Qt::KeyboardModifiers)), 
          this, SLOT(slotBevBottomLeft()));
  KAction* bbra = new KAction(i18n("Bottom Right"), this);
  bbra->setCheckable(true);
  actionCollection()->addAction("bev_bottom_right",bbra);
  connect(bbra, SIGNAL(triggered (Qt::MouseButtons, Qt::KeyboardModifiers)), 
          this, SLOT(slotBevBottomRight()));
  KAction* bba = new KAction(i18n("Automatic"), this);
  bba->setCheckable(true);
  actionCollection()->addAction("bev_automatic",bba);
  connect(bba, SIGNAL(triggered (Qt::MouseButtons, Qt::KeyboardModifiers)), 
          this, SLOT(slotBevAutomatic()));
  m_bevPopup->addAction(btla);
  m_bevPopup->addAction(btra);
  m_bevPopup->addAction(bbla);
  m_bevPopup->addAction(bbra);
  m_bevPopup->addAction(bba);
  switch (m_zoomPosition)
  {
    case DotGraphView::TopLeft:
      btla->setChecked(true);
    break;
    case DotGraphView::TopRight:
      btra->setChecked(true);
    break;
    case DotGraphView::BottomLeft:
      bbla->setChecked(true);
    break;
    case DotGraphView::BottomRight:
      bbra->setChecked(true);
    break;
    case DotGraphView::Auto:
      bba->setChecked(true);
    break;
  }


  kDebug() << "    m_bevEnabledAction setting checked to : " << KGraphViewerPartSettings::birdsEyeViewEnabled() << endl;
  m_bevEnabledAction->setChecked(KGraphViewerPartSettings::birdsEyeViewEnabled());
  m_bevPopup->setEnabled(KGraphViewerPartSettings::birdsEyeViewEnabled());
}

void DotGraphView::exportToImage()
{
  // write current content of canvas as image to file
  if (!m_canvas) return;
    
  QString fn = KFileDialog::getSaveFileName(KUrl(":"),QString("*.png"),0,QString(""));
  
  if (!fn.isEmpty()) 
  {
    QPixmap pix(m_canvas->sceneRect().size().toSize());
    QPainter p(&pix);
    m_canvas->render( &p );
    pix.save(fn,"PNG");
  }
}

void DotGraphView::slotExportImage()
{
  exportToImage();
}

void DotGraphView::slotLayoutSpecify()
{
  {
    bool ok = false;
    QString currentLayoutCommand = m_graph->layoutCommand();
    QString layoutCommand = 
    KInputDialog::getText(
      i18n("Layout Command"), 
      i18n("Type in a layout command for the current graph:"), 
      currentLayoutCommand, 
      &ok, 
      this, 
      0, 
      QString(), 
      i18n("Specify here the command that will be used to layout the graph.\n"
      "The command MUST write its results on stdout in xdot format."));
    //       std::cerr << "Got layout command: " << layoutCommand << std::endl;
    if (ok && layoutCommand != currentLayoutCommand)
    {
      //         std::cerr << "Setting new layout command: " << layoutCommand << std::endl;
      setLayoutCommand(layoutCommand);
    }
  }
}

void DotGraphView::slotLayoutReset()
{
  setLayoutCommand("");
}

void DotGraphView::slotSelectLayoutAlgo(const QString& ttext)
{
QString text = ttext;//.mid(1);
  kDebug() << "DotGraphView::slotSelectLayoutAlgo '" << text << "'" << endl;
  if (text == "&Dot")
  {
    setLayoutCommand("dot -Txdot");
  }
  else if (text == "&Neato")
  {
    setLayoutCommand("neato -Txdot");
  }
  else if (text == "&Twopi")
  {
    setLayoutCommand("twopi -Txdot");
  }
  else if (text == "&Fdp")
  {
    setLayoutCommand("fdp -Txdot");
  }
  else if (text == "&Circo")
  {
    setLayoutCommand("circo -Txdot");
  }
  else 
  {
    setLayoutCommand(text);
  }
}

void DotGraphView::slotSelectLayoutDot()
{
  kDebug() << "DotGraphView::slotSelectLayoutDot" << endl;
  setLayoutCommand("dot -Txdot");
}

void DotGraphView::slotSelectLayoutNeato()
{
  kDebug() << "DotGraphView::slotSelectLayoutNeato" << endl;
  setLayoutCommand("neato -Txdot");
}

void DotGraphView::slotSelectLayoutTwopi()
{
  kDebug() << "DotGraphView::slotSelectLayoutTwopi" << endl;
  setLayoutCommand("twopi -Txdot");
}

void DotGraphView::slotSelectLayoutFdp()
{
  kDebug() << "DotGraphView::slotSelectLayoutFdp" << endl;
  setLayoutCommand("fdp -Txdot");
}

void DotGraphView::slotSelectLayoutCirco()
{
  kDebug() << "DotGraphView::slotSelectLayoutCirco" << endl;
  setLayoutCommand("circo -Txdot");
}

void DotGraphView::slotBevEnabled()
{
  kDebug() << "DotGraphView::slotBevEnabled" << endl;
  kDebug() << "    m_bevEnabledAction is checked ? " << m_bevEnabledAction->isChecked() << endl;
  m_bevPopup->setEnabled(m_bevEnabledAction->isChecked());
  KGraphViewerPartSettings::setBirdsEyeViewEnabled(m_bevEnabledAction->isChecked());
///@TODO to port
//   KGraphViewerPartSettings::writeConfig();
  updateSizes();
}

void DotGraphView::slotBevTopLeft()
{
  viewBevActivated(DotGraphView::TopLeft);
}

void DotGraphView::slotBevTopRight()
{
  viewBevActivated(DotGraphView::TopRight);
}

void DotGraphView::slotBevBottomLeft()
{
  viewBevActivated(DotGraphView::BottomLeft);
}

void DotGraphView::slotBevBottomRight()
{
  viewBevActivated(DotGraphView::BottomRight); 
}

void DotGraphView::slotBevAutomatic()
{
  viewBevActivated(Auto); 
}


#include "dotgraphview.moc"

