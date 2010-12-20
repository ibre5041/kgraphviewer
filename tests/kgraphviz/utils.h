/*
    Copyright (C) 2010 Kevin Funk <krf@electrostorm.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KGRAPHVIZ_TESTS_UTILS_H
#define KGRAPHVIZ_TESTS_UTILS_H

#include <kgraphviz/dotgraph.h>

namespace Utilities
{
  bool haveSameElementCount(const KGraphViz::DotGraph* left,
                            const KGraphViz::DotGraph* right)
  {
    return (left->nodes().size() == right->nodes().size())
        && (left->edges().size() == right->edges().size());
  }
}

#endif