//===- InterDyckGraph.h -- Offline constraint graph -----------------------------//
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


#ifndef InterDyckGraph_H
#define InterDyckGraph_H

#include "Graphs/ConsG.h"
#include "Util/SCC.h"
#include "WPA/Andersen.h"
#include "SVF-FE/PAGBuilder.h"

namespace SVF
{
/*!
 * Offline constraint graph for Andersen's analysis.
 * In OCG, a 'ref' node is used to represent the point-to set of a constraint node.
 * 'Nor' means a constraint node of its corresponding ref node.
 */
class InterDyckGraph: public ConstraintGraph
{

public:
    typedef SCCDetection<InterDyckGraph*> OSCC;
    typedef std::map<const LoadCGEdge*, NodeID> LoadLabelMap;
    typedef std::map<const StoreCGEdge*, NodeID> StoreLabelMap;
    typedef std::map<const ConstraintEdge*, NodeID> CallLabelMap;
    typedef std::map<const ConstraintEdge*, NodeID> RetLabelMap;

    typedef std::map<NodeID, NodeID> ID2IDMap;

    typedef std::set<AddrCGEdge*> AddrEdges;
    typedef std::set<ConstraintEdge*> NonLabelEdges;

    typedef std::map<PointsTo, PointsTo, MemRegion::equalPointsTo > PtsToRepPtsSetMap;
    typedef std::map<PointsTo, NodeID, MemRegion::equalPointsTo> PointsToIDMap;

private:
    InterDyckGraph(PointerAnalysis* pta) : ConstraintGraph(pta->getPAG())
    {
        buildDyckGraph(pta);
    }
    static InterDyckGraph* IDG;

public:

    /// Singleton design here to make sure we only have one instance during any analysis
    //@{
    static inline InterDyckGraph* GetIDG(PAG *pag = PAG::getPAG())
    {
        if (IDG == NULL)
        {
    		/// Create Andersen's pointer analysis
    		Andersen *ander = AndersenWaveDiff::createAndersenWaveDiff(pag);
        	IDG = new InterDyckGraph(ander);
        }
        return IDG;
    }

    void buildDyckGraph(PointerAnalysis* pta);

    /// Get superset cpts set
    inline const PointsTo getRepPointsTo(const PointsTo& cpts)
    {
    	PointsTo pts = cpts;
		PointsTo rep = cptsToRepCPtsMap[pts];
    	while(rep!=pts){
    		pts = rep;
    		rep = cptsToRepCPtsMap[pts];
    	}
    	return rep;
    }

    /// set superset cpts set
    inline void setRepPointsTo(const PointsTo& cpts, const PointsTo& rep)
    {
    	cptsToRepCPtsMap[cpts] = rep;
    }

    inline void addToPts2IDMap(const PointsTo& cpts){
    	if(pointsToIDMap.find(cpts)==pointsToIDMap.end()){
    		pointsToIDMap.insert(std::make_pair(cpts, numPts++));
    	}
    }
    inline NodeID getPts2ID(const PointsTo& cpts) const{
    	PointsToIDMap::const_iterator it = pointsToIDMap.find(cpts);
    	assert(it!=pointsToIDMap.end());
    	return it->second;
    }
    inline NodeID getLoadLabel(const LoadCGEdge* edge) const{
    	LoadLabelMap::const_iterator it = ldLabels.find(edge);
    	assert(it!=ldLabels.end() && "not found?");
    	return it->second;
    }
    inline NodeID getStoreLabel(const StoreCGEdge* edge) const{
    	StoreLabelMap::const_iterator it = stLabels.find(edge);
    	assert(it!=stLabels.end() && "not found?");
    	return it->second;
    }
    inline NodeID getCallLabel(const ConstraintEdge* edge) const{
    	CallLabelMap::const_iterator it = callLabels.find(edge);
    	assert(it!=callLabels.end() && "not found?");
    	return it->second;
    }
    inline NodeID getRetLabel(const ConstraintEdge* edge) const{
    	RetLabelMap::const_iterator it = retLabels.find(edge);
    	assert(it!=retLabels.end() && "not found?");
    	return it->second;
    }
    inline bool isCallEdge(const ConstraintEdge* edge) const{
    	return callLabels.find(edge)!=callLabels.end();
    }
    inline bool isRetEdge(const ConstraintEdge* edge) const{
    	return retLabels.find(edge)!=retLabels.end();
    }
    /// Reset labels
    NodeID resetLabels(ID2IDMap& id2idMap, NodeID label);

    /// Dump graph into dot file
    void dump(std::string name);

private:
    PointsToIDMap pointsToIDMap;
    PtsToRepPtsSetMap cptsToRepCPtsMap;
    NodeID numPts;
    LoadLabelMap ldLabels;
    StoreLabelMap stLabels;
    CallLabelMap callLabels;
    RetLabelMap retLabels;
    NonLabelEdges nonLabelEdges;
};

} // End namespace SVF

namespace llvm
{
/* !
 * GraphTraits specializations for the generic graph algorithms.
 * Provide graph traits for traversing from a constraint node using standard graph traversals.
 */

template<> struct GraphTraits<SVF::InterDyckGraph*> : public GraphTraits<SVF::GenericGraph<SVF::ConstraintNode,SVF::ConstraintEdge>* >
{
    typedef SVF::ConstraintNode *NodeRef;
};

} // End namespace llvm

#endif //InterDyckGraph_H
