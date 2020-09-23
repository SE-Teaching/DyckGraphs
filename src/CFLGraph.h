//===- CFLGraph.h -- Offline constraint graph -----------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2017>  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * CFLGraph.h
 *
 *  Created on: Oct 28, 2018
 *      Author: Yuxiang Lei
 */

#ifndef CFLGraph_H
#define CFLGraph_H

#include "Graphs/ConsG.h"
#include "Util/SCC.h"

namespace SVF
{
class MemSSA;
/*!
 * Offline constraint graph for Andersen's analysis.
 * In OCG, a 'ref' node is used to represent the point-to set of a constraint node.
 * 'Nor' means a constraint node of its corresponding ref node.
 */
class CFLGraph: public ConstraintGraph
{

public:
    typedef SCCDetection<CFLGraph*> OSCC;
    typedef std::set<LoadCGEdge*> LoadEdges;
    typedef std::set<StoreCGEdge*> StoreEdges;
    typedef std::set<AddrCGEdge*> AddrEdges;

public:
    CFLGraph(PAG *p, MemSSA* mssa) : ConstraintGraph(p)
    {
        buildOfflineCG(mssa);
    }


    void buildOfflineCG(MemSSA* mssa);

};

} // End namespace SVF

namespace llvm
{
/* !
 * GraphTraits specializations for the generic graph algorithms.
 * Provide graph traits for traversing from a constraint node using standard graph traversals.
 */

template<> struct GraphTraits<SVF::CFLGraph*> : public GraphTraits<SVF::GenericGraph<SVF::ConstraintNode,SVF::ConstraintEdge>* >
{
    typedef SVF::ConstraintNode *NodeRef;
};

} // End namespace llvm

#endif //CFLGraph_H
