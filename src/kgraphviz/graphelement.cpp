/* This file is part of KGraphViewer.
   Copyright (C) 2007 Gael de Chalendar <kleag@free.fr>

   KGraphViewer is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA
*/

#include "graphelement.h"
#include "graphelement_p.h"

#include "canvaselement.h"
#include "support/dotdefaults.h"

#include <math.h>

#include <kdebug.h>

#include <QRegExp>
#include <graphviz/gvc.h>

namespace KGraphViz
{

GraphElementPrivate::GraphElementPrivate() :
  m_selected(false),
  m_z(1.0),
  m_canvasElement(0)
{
}

GraphElementPrivate::~GraphElementPrivate()
{
}

GraphElementPrivate::GraphElementPrivate(const GraphElementPrivate& other) :
  m_selected(other.m_selected),
  m_z(other.m_z),
  m_renderOperations(other.m_renderOperations),
  m_canvasElement(other.m_canvasElement)
{
}

GraphElement::GraphElement() :
    QObject(),
    d_ptr(new GraphElementPrivate)
{
/*  label("");
  id("");
  style(DOT_DEFAULT_STYLE);
  shape(DOT_DEFAULT_SHAPE);
  lineColor(DOT_DEFAULT_LINECOLOR);
  backColor(DOT_DEFAULT_BACKCOLOR);
  fontName(DOT_DEFAULT_FONTNAME);
  fontColor(DOT_DEFAULT_FONTCOLOR);
  url("");
  shapeFile("");*/
  setFontSize(DOT_DEFAULT_FONTSIZE);
}

GraphElement::GraphElement(const GraphElement& element) :
  QObject(),
  d_ptr(element.d_ptr)
{
  kDebug() ;
  updateWithElement(element);
}

GraphElement::~GraphElement()
{
  delete d_ptr;
}

CanvasElement* GraphElement::canvasElement() const
{
  Q_D(const GraphElement);
  return d->m_canvasElement;
}

void GraphElement::setCanvasElement(CanvasElement* canvasElement)
{
  Q_D(GraphElement);
  d->m_canvasElement = canvasElement;
}

bool GraphElement::isSelected() const
{
  Q_D(const GraphElement);
  return d->m_selected;
}

void GraphElement::setSelected(bool selected)
{
  Q_D(GraphElement);
  d->m_selected = selected;
}

double GraphElement::z() const
{
  Q_D(const GraphElement);
  return d->m_z;
}

void GraphElement::setZ(double z)
{
  Q_D(GraphElement);
  d->m_z = z;
}

DotRenderOpVec& GraphElement::renderOperations()
{
  Q_D(GraphElement);
  return d->m_renderOperations;
}

DotRenderOpVec GraphElement::renderOperations() const
{
  Q_D(const GraphElement);
  return d->m_renderOperations;
}

void GraphElement::setRenderOperations(const DotRenderOpVec& dotRenderOpVec)
{
  Q_D(GraphElement);
  d->m_renderOperations = dotRenderOpVec;
}

void GraphElement::updateWithElement(const GraphElement& element)
{
  kDebug() << element.id();
  bool modified = false;
  if (element.z() != z())
  {
    setZ(element.z());
    modified = true;
  }
  QMap <QString, QString >::const_iterator it = element.attributes().constBegin();
  for (;it != element.attributes().constEnd(); it++)
  {
    const QString &attrib = it.key();
    if ( (!m_attributes.contains(attrib)) || (m_attributes[attrib] != it.value()) )
    {
      m_attributes[attrib] = it.value();
      if (attrib == "z")
      {
        bool ok;
        setZ(m_attributes[attrib].toDouble(&ok));
      }
      modified = true;
    }
  }
  if (modified)
  {
    kDebug() << "modified: update render operations";
    setRenderOperations(element.renderOperations());
/*    foreach (DotRenderOp op, m_renderOperations)
    {
      QString msg;
      QTextStream dd(&msg);
      dd << "an op: " << op.renderop << " ";
      foreach (int i, op.integers)
      {
        dd << i << " ";
      }
      dd << op.str;
      kDebug() << msg;
    }
    kDebug() << "modified: emiting changed";*/
    emit changed();
  }
  kDebug() << "done" << renderOperations().size();
}


QString GraphElement::backColor() const
{
  if (m_attributes.find("fillcolor") != m_attributes.end())
  {
    return m_attributes["fillcolor"];
  }
  else if ( (m_attributes.find("color") != m_attributes.end())
    && (m_attributes["style"] == "filled") )
  {
    return m_attributes["color"];
  }
  else
  {
    return DOT_DEFAULT_NODE_BACKCOLOR;
  }
}

void GraphElement::removeAttribute(const QString& attribName)
{
  kDebug() << attribName;
  m_attributes.remove(attribName);
  emit changed();
}

void GraphElement::exportToGraphviz(void* element) const
{
  QMap<QString,QString>::const_iterator it, it_end;
  it = attributes().begin(); it_end = attributes().end();
  for (;it != it_end; it++)
  {
    if (!it.value().isEmpty())
    {
      if (it.key() == "label")
      {
        QString label = it.value();
        if (label != "label")
        {
          label.replace(QRegExp("\n"),"\\n");
          //           kDebug() << it.key() << "=\"" << label << "\",";
          agsafeset(element, it.key().toUtf8().data(), label.toUtf8().data(), QString().toUtf8().data());
        }
      }
      else if (it.key() == "_draw_" || it.key() == "_ldraw_")
      {
      }
      else if (originalAttributes().isEmpty() || originalAttributes().contains(it.key()))
      {
        //         kDebug() << it.key() << it.value();
        
        agsafeset(element, it.key().toUtf8().data(), it.value().toUtf8().data(), QString().toUtf8().data());
      }
    }
  }
}

QTextStream& operator<<(QTextStream& s, const GraphElement& n)
{
  QMap<QString,QString>::const_iterator it, it_end;
  it = n.attributes().begin(); it_end = n.attributes().end();
  for (;it != it_end; it++)
  {
    if (!it.value().isEmpty())
    {
      if (it.key() == "label")
      {
        QString label = it.value();
        if (label != "label")
        {
          label.replace(QRegExp("\n"),"\\n");
//           kDebug() << it.key() << "=\"" << label << "\",";
          s << it.key() << "=\"" << label << "\",";
        }
      }
      else if (it.key() == "_draw_" || it.key() == "_ldraw_")
      {
      }
      else if (n.originalAttributes().isEmpty() || n.originalAttributes().contains(it.key()))
      {
//         kDebug() << it.key() << it.value();
        
          s << it.key() << "=\"" << it.value() << "\",";
      }
    }
  }
  return s;
}

}

#include "graphelement.moc"