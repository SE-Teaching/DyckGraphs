//===- InterDyckGraph.cpp -- Offline constraint graph -----------------------------//
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



#include "InterDyckGraph.h"
#include "MSSA/MemSSA.h"

using namespace SVF;
using namespace SVFUtil;

InterDyckGraph* InterDyckGraph::IDG = NULL;

/*!
 * Builder of InterDyckGraph
 */
void InterDyckGraph::buildDyckGraph(PointerAnalysis* pta)
{
    AddrEdges addrs;

    // collect pts at stores and loads
    for (ConstraintEdge::ConstraintEdgeSetTy::iterator it = LoadCGEdgeSet.begin(), eit =
                LoadCGEdgeSet.end(); it != eit; ++it)
    {
        LoadCGEdge *load = SVFUtil::dyn_cast<LoadCGEdge>(*it);
        const PointsTo& pts = pta->getPts(load->getSrcID());
        addToPts2IDMap(pts);
        setRepPointsTo(pts,pts);
    }
    for (ConstraintEdge::ConstraintEdgeSetTy::iterator it = StoreCGEdgeSet.begin(), eit =
                StoreCGEdgeSet.end(); it != eit; ++it)
    {
        StoreCGEdge *store = SVFUtil::dyn_cast<StoreCGEdge>(*it);
        const PointsTo& pts = pta->getPts(store->getDstID());
        addToPts2IDMap(pts);
        setRepPointsTo(pts,pts);
    }

    /// establish the representation pts of each points-to set, i.e., super set of each pts
    for(PointsToIDMap::iterator cit = pointsToIDMap.begin(), ecit = pointsToIDMap.end(); cit!=ecit; ++cit)
    {
        const PointsTo& lpts = cit->first;
        for(PointsToIDMap::iterator it = cit; it!=ecit; ++it){
            const PointsTo& rpts = it->first;
            	if(rpts.contains(lpts))
            		setRepPointsTo(lpts,rpts);
        }
    }


    // label each load with a unique id from the established representation mapping
    for (ConstraintEdge::ConstraintEdgeSetTy::iterator it = LoadCGEdgeSet.begin(), eit =
                LoadCGEdgeSet.end(); it != eit; ++it)
    {
        LoadCGEdge *load = SVFUtil::dyn_cast<LoadCGEdge>(*it);
        const PointsTo& pts = pta->getPts(load->getSrcID());
        const PointsTo& rep = getRepPointsTo(pts);
        ldLabels[load] = getPts2ID(rep);
    }

    // label each store with a unique id from the established representation mapping
    for (ConstraintEdge::ConstraintEdgeSetTy::iterator it = StoreCGEdgeSet.begin(), eit =
                StoreCGEdgeSet.end(); it != eit; ++it)
    {
        StoreCGEdge *store = SVFUtil::dyn_cast<StoreCGEdge>(*it);
        const PointsTo& pts = pta->getPts(store->getDstID());
        const PointsTo& rep = getRepPointsTo(pts);
        stLabels[store] = getPts2ID(rep);
    }

    // label each call with a unique id from the callsite
    PAGEdge::PAGEdgeSetTy& calls = getPAGEdgeSet(PAGEdge::Call);
    for (PAGEdge::PAGEdgeSetTy::iterator iter = calls.begin(), eiter =
                calls.end(); iter != eiter; ++iter)
    {
    	CallPE* pagEdge = SVFUtil::dyn_cast<CallPE>(*iter);
        ConstraintEdge* cgEdge = getEdge(getConstraintNode(pagEdge->getSrcID()),getConstraintNode(pagEdge->getDstID()),ConstraintEdge::Copy);
        callLabels[cgEdge] = pagEdge->getCallSite()->getId();
    }

    // label each call with a unique id from the callsite
    PAGEdge::PAGEdgeSetTy& rets = getPAGEdgeSet(PAGEdge::Ret);
    for (PAGEdge::PAGEdgeSetTy::iterator iter = rets.begin(), eiter =
                rets.end(); iter != eiter; ++iter)
    {
    	RetPE* pagEdge = SVFUtil::dyn_cast<RetPE>(*iter);
        ConstraintEdge* cgEdge = getEdge(getConstraintNode(pagEdge->getSrcID()),getConstraintNode(pagEdge->getDstID()),ConstraintEdge::Copy);
        retLabels[cgEdge] = pagEdge->getCallSite()->getId();
    }


    // for all other types of edges  x-->y, they will be transformed to be x--ob_1-->z-->cb_1-->y
    for (ConstraintEdge::ConstraintEdgeSetTy::iterator it = directEdgeSet.begin(), eit =
    		directEdgeSet.end(); it != eit; ++it)
    {
    	ConstraintEdge *edge = *it;
    	if(isCallEdge(edge)==false && isRetEdge(edge) == false){
    		nonLabelEdges.insert(edge);
    	}
    }

    for (ConstraintEdge::ConstraintEdgeSetTy::iterator it = nonLabelEdges.begin(), eit =
    		nonLabelEdges.end(); it != eit; ++it)
    {
    	ConstraintEdge *edge = *it;
        removeDirectEdge(edge);

        NodeID refId = pag->addDummyValNode();
        ConstraintNode* node = new ConstraintNode(refId);
        addConstraintNode(node, refId);

        StoreCGEdge* store = addStoreCGEdge(edge->getSrcID(),refId);
        LoadCGEdge* load = addLoadCGEdge(refId,edge->getDstID());
        assert(store && "store not created?");
        assert(load && "load not created?");

        NodeID maxID = pointsToIDMap.size() + 1;
        stLabels[store] = maxID;
        ldLabels[load] = maxID;
    }

    // collect and remove all address edges
    for (ConstraintEdge::ConstraintEdgeSetTy::iterator it = AddrCGEdgeSet.begin(), eit =
    		AddrCGEdgeSet.end(); it != eit; ++it)
    {
        AddrCGEdge *ad = SVFUtil::dyn_cast<AddrCGEdge>(*it);
        addrs.insert(ad);
    }

    for (AddrEdges::iterator it = addrs.begin(), eit = addrs.end(); it != eit; ++it)
    {
        removeAddrEdge(*it);
    }
}

void InterDyckGraph::dump(std::string name){
    GraphPrinter::WriteGraphToFile(outs(), name, this);
}

// --- GraphTraits specialization for InterDyckGraph ---

namespace llvm
{
template<>
struct DOTGraphTraits<InterDyckGraph*> : public DOTGraphTraits<PAG*>
{

    typedef ConstraintNode NodeType;
    DOTGraphTraits(bool isSimple = false) :
        DOTGraphTraits<PAG*>(isSimple)
    {
    }

    /// Return name of the graph
    static std::string getGraphName(InterDyckGraph*)
    {
        return "InterDyckGraph Graph";
    }

    /// Return label of a VFG node with two display mode
    /// Either you can choose to display the name of the value or the whole instruction
    static std::string getNodeLabel(NodeType *n, InterDyckGraph*)
    {
        //std::string str;
        //raw_string_ostream rawstr(str);
        //rawstr<< n->getId();
        return "";
    }

    static std::string getNodeAttributes(NodeType *n, InterDyckGraph*)
    {
    	return "";
    }

    template<class EdgeIter>
    static std::string getEdgeAttributes(NodeType*, EdgeIter EI, InterDyckGraph*)
    {
    	InterDyckGraph* graph = InterDyckGraph::GetIDG();
        std::string str;
        raw_string_ostream rawstr(str);
        ConstraintEdge* edge = *(EI.getCurrent());

        if(LoadCGEdge *load = SVFUtil::dyn_cast<LoadCGEdge>(edge)){
        	rawstr << "cp-" << graph->getLoadLabel(load);
        }
        else if(StoreCGEdge *store = SVFUtil::dyn_cast<StoreCGEdge>(edge)){
        	rawstr << "op-" << graph->getStoreLabel(store);
        }
        else if(CopyCGEdge *copy = SVFUtil::dyn_cast<CopyCGEdge>(edge)){

        	if(graph->isCallEdge(copy)){
        		rawstr << "ob-" << graph->getCallLabel(copy);
        	}
        	else if(graph->isRetEdge(copy)){
        		rawstr << "cb-" << graph->getRetLabel(copy);
        	}
        	else
        		assert(false && "do we have a non-labeled edge?");
        }
        else
        	assert(false && "do we have a non-labeled edge?");

        return rawstr.str();
    }

    template<class EdgeIter>
    static std::string getEdgeSourceLabel(NodeType*, EdgeIter EI)
    {
    	return "";
    }
};
} // End namespace llvm
