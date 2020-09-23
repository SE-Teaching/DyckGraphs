//===- CFLGraph.cpp -- Offline constraint graph -----------------------------//
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
 * CFLGraph.cpp
 *
 *  Created on: Oct 28, 2018
 *      Author: Yuxiang Lei
 */

#include "CFLGraph.h"
#include "MSSA/MemSSA.h"

using namespace SVF;
using namespace SVFUtil;

static llvm::cl::opt<bool> OCGDotGraph("dump-ocg", llvm::cl::init(false),
                                       llvm::cl::desc("Dump dot graph of Offline Constraint Graph"));


/*!
 * Builder of offline constraint graph
 */
void CFLGraph::buildOfflineCG(MemSSA* mssa)
{
	MRGenerator* mrgen = mssa->getMRGenerator();
	PointerAnalysis* pta = mssa->getPTA();

    LoadEdges loads;
    StoreEdges stores;
    AddrEdges addrs;

    // Add a copy edge between the ref node of src node and dst node
    for (ConstraintEdge::ConstraintEdgeSetTy::iterator it = LoadCGEdgeSet.begin(), eit =
                LoadCGEdgeSet.end(); it != eit; ++it)
    {
        LoadCGEdge *load = SVFUtil::dyn_cast<LoadCGEdge>(*it);
        loads.insert(load);
        NodeID src = load->getSrcID();
        NodeID dst = load->getDstID();
        const MemRegion* mr = mrgen->getMR(pta->getPts(src));
        addNormalGepCGEdge(src,dst, mr->getMRID());
    }
    // Add a copy edge between src node and the ref node of dst node
    for (ConstraintEdge::ConstraintEdgeSetTy::iterator it = StoreCGEdgeSet.begin(), eit =
                StoreCGEdgeSet.end(); it != eit; ++it)
    {
        StoreCGEdge *store = SVFUtil::dyn_cast<StoreCGEdge>(*it);
        stores.insert(store);
        NodeID src = store->getSrcID();
        NodeID dst = store->getDstID();
        const MemRegion* mr = mrgen->getMR(pta->getPts(dst));
        addNormalGepCGEdge(src,dst, mr->getMRID());
    }


    // Remove load and store edges in offline constraint graph
    for (LoadEdges::iterator it = loads.begin(), eit = loads.end(); it != eit; ++it)
    {
        removeLoadEdge(*it);
    }
    for (StoreEdges::iterator it = stores.begin(), eit = stores.end(); it != eit; ++it)
    {
        removeStoreEdge(*it);
    }
    for (AddrEdges::iterator it = addrs.begin(), eit = addrs.end(); it != eit; ++it)
    {
        removeAddrEdge(*it);
    }
    // Dump offline graph with removed load and store edges
    dump("oCG_final");
}


// --- GraphTraits specialization for offline constraint graph ---

namespace llvm
{
template<>
struct DOTGraphTraits<CFLGraph*> : public DOTGraphTraits<PAG*>
{

    typedef ConstraintNode NodeType;
    DOTGraphTraits(bool isSimple = false) :
        DOTGraphTraits<PAG*>(isSimple)
    {
    }

    /// Return name of the graph
    static std::string getGraphName(CFLGraph*)
    {
        return "Offline Constraint Graph";
    }

    /// Return label of a VFG node with two display mode
    /// Either you can choose to display the name of the value or the whole instruction
    static std::string getNodeLabel(NodeType *n, CFLGraph*)
    {
        std::string str;
        raw_string_ostream rawstr(str);
        if (PAG::getPAG()->findPAGNode(n->getId()))
        {
            PAGNode *node = PAG::getPAG()->getPAGNode(n->getId());
            bool briefDisplay = true;
            bool nameDisplay = true;


            if (briefDisplay)
            {
                if (SVFUtil::isa<ValPN>(node))
                {
                    if (nameDisplay)
                        rawstr << node->getId() << ":" << node->getValueName();
                    else
                        rawstr << node->getId();
                }
                else
                    rawstr << node->getId();
            }
            else
            {
                // print the whole value
                if (!SVFUtil::isa<DummyValPN>(node) && !SVFUtil::isa<DummyObjPN>(node))
                    rawstr << *node->getValue();
                else
                    rawstr << "";

            }

            return rawstr.str();
        }
        else
        {
            rawstr<< n->getId();
            return rawstr.str();
        }
    }

    static std::string getNodeAttributes(NodeType *n, CFLGraph*)
    {
        if (PAG::getPAG()->findPAGNode(n->getId()))
        {
            PAGNode *node = PAG::getPAG()->getPAGNode(n->getId());
            if (SVFUtil::isa<ValPN>(node))
            {
                if (SVFUtil::isa<GepValPN>(node))
                    return "shape=hexagon";
                else if (SVFUtil::isa<DummyValPN>(node))
                    return "shape=diamond";
                else
                    return "shape=circle";
            }
            else if (SVFUtil::isa<ObjPN>(node))
            {
                if (SVFUtil::isa<GepObjPN>(node))
                    return "shape=doubleoctagon";
                else if (SVFUtil::isa<FIObjPN>(node))
                    return "shape=septagon";
                else if (SVFUtil::isa<DummyObjPN>(node))
                    return "shape=Mcircle";
                else
                    return "shape=doublecircle";
            }
            else if (SVFUtil::isa<RetPN>(node))
            {
                return "shape=Mrecord";
            }
            else if (SVFUtil::isa<VarArgPN>(node))
            {
                return "shape=octagon";
            }
            else
            {
                assert(0 && "no such kind node!!");
            }
            return "";
        }
        else
        {
            return "shape=doublecircle";
        }
    }

    template<class EdgeIter>
    static std::string getEdgeAttributes(NodeType*, EdgeIter EI, CFLGraph*)
    {
        ConstraintEdge* edge = *(EI.getCurrent());
        assert(edge && "No edge found!!");
        if (edge->getEdgeKind() == ConstraintEdge::Addr)
        {
            return "color=green";
        }
        else if (edge->getEdgeKind() == ConstraintEdge::Copy)
        {
            return "color=black";
        }
        else if (edge->getEdgeKind() == ConstraintEdge::NormalGep
                 || edge->getEdgeKind() == ConstraintEdge::VariantGep)
        {
            return "color=purple";
        }
        else if (edge->getEdgeKind() == ConstraintEdge::Store)
        {
            return "color=blue";
        }
        else if (edge->getEdgeKind() == ConstraintEdge::Load)
        {
            return "color=red";
        }
        else
        {
            assert(0 && "No such kind edge!!");
        }
        return "";
    }

    template<class EdgeIter>
    static std::string getEdgeSourceLabel(NodeType*, EdgeIter)
    {
        return "";
    }
};
} // End namespace llvm