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

#ifndef GRAPH_ELEMENT_H
#define GRAPH_ELEMENT_H

#include <QObject>

#include "kgraphviz_export.h"
#include "support/dotrenderop.h"

#include <QList>
#include <QMap>
#include <QTextStream>

namespace KGraphViz
{

class CanvasElement;
class GraphElementPrivate;

/**
 * The base of all GraphViz dot graph elements (nodes, edges, subgraphs,
 * graphs). It is used to store the element attributes
 */
class KGRAPHVIZ_EXPORT GraphElement: public QObject
{
  Q_OBJECT

public:
  GraphElement();
  GraphElement(const GraphElement& element);

  virtual ~GraphElement();
  
  inline void setId(const QString& id) {m_attributes["id"]=id;}
  inline void setStyle(const QString& ls) {m_attributes["style"]=ls;}
  inline void setShape(const QString& lc) {m_attributes["shape"]=lc;}
  inline void setColor(const QString& nt) {m_attributes["color"]=nt;}
  inline void setLineColor(const QString& nt) {m_attributes["color"]=nt;}
  inline void setBackColor(const QString& nc) {m_attributes["bgcolor"]=nc;}
  
  inline QString id() const {return m_attributes["id"];}
  inline QString style() const {return m_attributes["style"];}
  inline QString shape() const {return m_attributes["shape"];}
  inline QString color() const {return m_attributes["color"];}
  inline QString lineColor() const {return m_attributes["color"];}
  virtual QString backColor() const;
  
  inline void setLabel(const QString& label) {m_attributes["label"]=label;}
  inline const QString label() const {return m_attributes["label"];}

  inline unsigned int fontSize() const {return m_attributes["fontsize"].toUInt();}
  inline void setFontSize(unsigned int fs) {m_attributes["fontsize"]=QString::number(fs);}
  inline QString fontName() const {return m_attributes["fontname"];}
  inline void setFontName(const QString& fn) {m_attributes["fontname"]=fn;}
  inline QString fontColor() const {return m_attributes["fontcolor"];}
  inline void setFontColor(const QString& fc) {m_attributes["fontcolor"] = fc;}

  DotRenderOpVec& renderOperations();
  DotRenderOpVec renderOperations() const;
  void setRenderOperations(const DotRenderOpVec& dotRenderOpVec);

  inline QString shapeFile() const {return m_attributes["shapefile"];}
  inline void setShapeFile(const QString& sf) {m_attributes["shapefile"] = sf;}
  
  inline QString url() const {return m_attributes["URL"];}
  inline void setUrl(const QString& theUrl) {m_attributes["URL"] = theUrl;}

  virtual void updateWithElement(const GraphElement& element);

  inline QMap<QString,QString>& attributes() {return m_attributes;}
  inline const QMap<QString,QString>& attributes() const {return m_attributes;}

  inline QList<QString>& originalAttributes() {return m_originalAttributes;}
  inline const QList<QString>& originalAttributes() const {return m_originalAttributes;}

  virtual inline void storeOriginalAttributes() {m_originalAttributes = m_attributes.keys();}

  virtual void removeAttribute(const QString& attribName);

  CanvasElement* canvasElement() const;
  void setCanvasElement(CanvasElement* canvasElement);

  double z() const;
  void setZ(double z);

  void exportToGraphviz(void* element)  const;
  void importFromGraphviz(void* element, QList<QString> drawingAttributes);

Q_SIGNALS:
  void changed();

protected:
  QMap<QString,QString> m_attributes;
  QList<QString> m_originalAttributes;

  GraphElementPrivate* const d_ptr;

private:
  Q_DECLARE_PRIVATE(GraphElement);
};

KGRAPHVIZ_EXPORT QTextStream& operator<<(QTextStream& s, const GraphElement& n);

}

#endif
