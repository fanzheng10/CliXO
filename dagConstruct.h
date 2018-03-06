#ifndef DAG_CONSTRUCT
#define DAG_CONSTRUCT

#include <math.h>
#include <time.h>
#include <list>
#include <algorithm>
#include <thread>
#include <mutex>
#include "dag.h"
#include "graph_undirected.h"
#include "graph_undirected_bitset.h"
#include "util.h"
#include "nodeDistanceObject.h"
#include "boost/dynamic_bitset/dynamic_bitset.hpp"

//can I have global variable here?
//unsigned long recursion;
//unsigned long recursion2;
bool useChordal = false;
double globalDensity = 1.0;

void printCluster(const boost::dynamic_bitset<unsigned long> & cluster, vector<string> & nodeIDsToNames) {
    for (unsigned i = 0; i < cluster.size(); ++i) {
        if (cluster[i]) {
            cout << nodeIDsToNames[i] << ",";
        }
    }
}

bool compPairSecondAscending(const pair< unsigned, unsigned > & i,const pair< unsigned, unsigned > & j) {
    return (i.second < j.second);
}


bool compPairSecondDescending(const pair< unsigned, unsigned > & i,const pair< unsigned, unsigned > & j) {
    return (i.second > j.second);
}

bool compClustersToCombine(const pair<pair<unsigned long, unsigned long>, double> & i, const pair<pair<unsigned long, unsigned long>, double> & j) {
    if (i.second == j.second) {
        if (i.first.first == j.first.first) {
            return i.first.second < j.first.second;
        }
        return i.first.first < j.first.first;
    }
    return i.second > j.second;
}

bool compEdgesToAdd(const pair<pair<unsigned, unsigned>, double> & i, const pair<pair<unsigned, unsigned>, double> & j) {
    if (i.first.first == j.first.first) {
        return i.first.second < j.first.second;
    }
    return i.first.first < j.first.first;
}

bool newClustersComp(const pair<boost::dynamic_bitset<unsigned long>, unsigned> & i, const pair<boost::dynamic_bitset<unsigned long>, unsigned> & j) {
    if (i.second == j.second) {
        return i.first < j.first;
    }
    return i.second < j.second;
}


class validClusterBitset {
public:
    validClusterBitset(const boost::dynamic_bitset<unsigned long> & cluster, unsigned clustID, double thisWeight) {
        elements = cluster;
        ID = clustID;
        weight = thisWeight;
        numElementsHere = cluster.count();
    }

    validClusterBitset(const vector<unsigned> & cluster, unsigned clustID, double thisWeight, unsigned numNodes) {
        elements = boost::dynamic_bitset<unsigned long>(numNodes);
        for (vector<unsigned>::const_iterator it = cluster.begin(); it != cluster.end(); ++it) {
            elements[*it] = 1;
        }
        ID = clustID;
        weight = thisWeight;
        numElementsHere = elements.count();
    }

    bool operator<(const validClusterBitset & b) const {
        return numElementsHere < b.numElements();
    }

    inline unsigned getID() {
        return ID;
    }

    inline void setID(unsigned newID) {
        ID = newID;
        return;
    }

    inline double getWeight() {
        return weight;
    }

    inline const boost::dynamic_bitset<unsigned long> & getElements() {
        return elements;
    }

    inline unsigned isElement(unsigned i) {
        return elements[i];
    }

    inline unsigned numElements() const {
        return numElementsHere;
    }

    inline void addElement(unsigned newElem) {
        elements[newElem] = 1;
        ++numElementsHere;
        return;
    }

    inline vector<unsigned> getElementsVector() {
        vector<unsigned> result;
        result.reserve(numElements());
        for (unsigned i = elements.find_first(); i < elements.size(); i = elements.find_next(i)) {
            result.push_back(i);
        }
        return result;
    }

private:
    boost::dynamic_bitset<unsigned long> elements;
    unsigned ID;
    double weight;
    unsigned numElementsHere;
};


class ClusterBitset {
public:
    ClusterBitset(const boost::dynamic_bitset<unsigned long> & cluster, unsigned long & clustID, unsigned long & thisTrieID, double thisClusterWeight = 0) {
        elements = cluster;
        isClusterNew = true;
        isClusterAddedToExplain = false;
        isClusterUnexplainedCounted = false;
        necessary = false;
        checkedFinal = false;
        ID = clustID;
        trieID = thisTrieID;
        numElementsHere = cluster.count();
        clusterWeight = thisClusterWeight;
        valid = false;
        uniquelyExplainedEdges = 0;
    }

    ClusterBitset() {
        isClusterNew = false;
        isClusterAddedToExplain = false;
        isClusterUnexplainedCounted = false;
        necessary = false;
        checkedFinal = false;
        ID = 0;
        numElementsHere = 0;
        trieID = 0;
        clusterWeight = 0;
        valid = false;
        uniquelyExplainedEdges = 0;
    }

    inline double getWeight() {
        return clusterWeight;
    }


    inline double getThresh(double alpha, unsigned numUnexplainedEdges) {
        if (numUnexplainedEdges == 0) {
            return 0;
        }
        return clusterWeight - alpha;
    }

    inline bool isUnexplainedCounted() {
        return isClusterUnexplainedCounted;
    }

    inline void setUnexplainedCounted() {
        isClusterUnexplainedCounted = true;
    }

    inline void setUnexplainedUncounted() {
        isClusterUnexplainedCounted = false;
    }

    inline unsigned getUniquelyExplainedEdges() {
        return uniquelyExplainedEdges;
    }

    inline void setUniquelyExplainedEdges(unsigned val) {
        uniquelyExplainedEdges = val;
        setUnexplainedCounted();
    }

    inline void setWeight(const double & weight) {
        clusterWeight = weight;
    }

    inline vector<pair<double,unsigned> >::iterator historicalWeightsBegin() {
        return historicalWeights.begin();
    }

    inline vector<pair<double,unsigned> >::iterator historicalWeightsEnd() {
        return historicalWeights.end();
    }

    inline void clearHistoricalWeights() {
        historicalWeights.clear();
    }

    inline unsigned long getID() {
        return ID;
    }

    inline unsigned long getTrieID() {
        return trieID;
    }

    inline void setTrieID(unsigned long newTrieID) {
        trieID = newTrieID;
    }

    inline void addElement(unsigned newElem) {
        setNew();
        elements[newElem] = 1;
        ++numElementsHere;
        if (elementsVector.size()) {
            Utils::insertInOrder(elementsVector, newElem);
        }
        return;
    }

    inline const boost::dynamic_bitset<unsigned long> & getElements() {
        return elements;
    }

    inline const vector<unsigned> & getElementsVector() {
        if (elementsVector.size()) {
            return elementsVector;
        }
        elementsVector.reserve(numElements());
        for (unsigned i = 0; i < elements.size(); ++i) {
            if (elements[i] == true) {
                elementsVector.push_back(i);
            }
        }
        return elementsVector;
    }

    inline unsigned numElements() const {
        return numElementsHere;
    }

    inline bool isNew() {
        return isClusterNew;
    }

    inline bool isAddedToExplain() {
        return isClusterAddedToExplain;
    }

    inline void setAddedToExplain() {
        isClusterAddedToExplain = true;
    }

    inline void setRemovedFromExplain() {
        isClusterAddedToExplain = false;
    }

    inline void setOld() {
        isClusterNew = false;
    }

    inline void setNew() {
        isClusterNew = true;
        necessary = false;
        checkedFinal = false;
        isClusterUnexplainedCounted= false;
    }

    inline bool isElement(unsigned elemID) {
        return elements.test(elemID);
    }

    inline unsigned size() {
        return elements.size();
    }

    inline void setValid() {
        valid = true;
        necessary = true;
        checkedFinal = true;
    }

    inline void setInvalid() {
        /*if (valid) {
          numExtensions = 0;
          }*/
        valid = false;
    }

    inline bool isValid() {
        return valid;
        /*if (valid) {
          return 1;
        }
        return 0;*/
    }

    inline bool wasNecessary() {
        return necessary;
    }

    inline void setNecessary() {
        necessary = true;
    }

    inline bool wasCheckedFinal() {
        return checkedFinal;
    }

    inline void setCheckedFinal() {
        checkedFinal = true;
    }

private:
    boost::dynamic_bitset<unsigned long> elements;
    bool isClusterNew;
    bool isClusterAddedToExplain;
    bool isClusterUnexplainedCounted;
    bool necessary;
    bool checkedFinal;
    unsigned long ID;
    unsigned numElementsHere;
    unsigned long trieID;
    vector<unsigned> elementsVector;
    double clusterWeight;
    vector<pair<double,unsigned> > historicalWeights;
    bool valid;
    unsigned uniquelyExplainedEdges;
};


class clusterMapClass {
public:
    clusterMapClass() {};

    inline bool addKey(const boost::dynamic_bitset<unsigned long> & key) {
        return Utils::insertInOrder(clustersVec, key);
    }

    inline bool deleteKey(const boost::dynamic_bitset<unsigned long> & key) {
        return Utils::eraseInOrder(clustersVec, key);
    }

    inline bool keyExists(const boost::dynamic_bitset<unsigned long> & key) {
        return Utils::elementExists(clustersVec, key);
    }

private:
    vector<boost::dynamic_bitset<unsigned long> > clustersVec;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////


class currentClusterClassBitset {
public:
    currentClusterClassBitset(unsigned numNodesHere, unsigned numBlocks, double FirstWeight = 1, double thisAlpha = 0) {
        numNodes = numNodesHere;
        nodesToClusters = vector<vector<unsigned long> >(numNodes, vector<unsigned long>());
        nextID = 0;
        clustersDeleted = 0;
        clustersAdded = 0;
        clustersExtended = 0;
        newClusts = 0;
        firstWeight = FirstWeight;
        curWeight = firstWeight;
        maxNewWeight = firstWeight;
        minWeightAdded = firstWeight;
        combiningNow = false;
        alpha = thisAlpha;
//        largestCluster = 0;

        edgesToClusters = vector<vector<unsigned> >(numNodes, vector<unsigned>(numNodes, 0));
        isEdgeExplained = vector<vector<char> >(numNodes, vector<char>(numNodes, 0));
    };

    /*inline void printCluster(const boost::dynamic_bitset<unsigned long> & cluster) {
      for (unsigned i = 0; i < cluster.size(); ++i) {
        if (cluster[i]) {
      cout << i << ",";
        }
      }
      }*/

    inline void setCurWeight(const double & weight) {
        if (weight < minWeightAdded) {
            minWeightAdded = weight;
        }
        curWeight = weight;
    }


    inline void setEdgeExplained(const unsigned & i, const unsigned & j) {
        isEdgeExplained[i][j] = 1;
    }

    inline bool isThisEdgeExplained(const unsigned & i, const unsigned & j) {
        return isEdgeExplained[i][j];
    }


    inline bool addClusterToExplainEdge(const unsigned & edgeEnd1, const unsigned & edgeEnd2, const unsigned long & clustID) {
        ++edgesToClusters[edgeEnd1][edgeEnd2];
        return true;
    }

    inline bool addClusterToExplanation(const vector<unsigned> & cluster, const unsigned long & clustID, graph_undirected_bitset & clusterGraph) {
        bool validClust = false;
        for (vector<unsigned>::const_iterator it1 = cluster.begin(); it1 != cluster.end(); ++it1) {
            vector<unsigned>::const_iterator it2 = it1;
            ++it2;
            for ( ; it2 != cluster.end(); ++it2) {
                if (clusterGraph.isEdge(*it1, *it2)) {
                    validClust = addClusterToExplainEdge(*it1, *it2, clustID);
                }
            }
        }
        currentClusters[clustID].setAddedToExplain();
        return validClust;
    }

    inline void extendExplanation(unsigned long clustID, unsigned nodeToAdd) {
        unsigned i = currentClusters[clustID].getElements().find_first();
        while (i < nodeToAdd) {
            addClusterToExplainEdge(i, nodeToAdd, clustID);
            i = currentClusters[clustID].getElements().find_next(i);
        }
        while (i < numNodes) {
            addClusterToExplainEdge(nodeToAdd, i, clustID);
            i = currentClusters[clustID].getElements().find_next(i);
        }
        return;
    }

    inline void removeClusterToExplainEdge(const unsigned & edgeEnd1, const unsigned & edgeEnd2, const unsigned long & clustID) {
        --edgesToClusters[edgeEnd1][edgeEnd2];
        return;
    }

    inline void removeClusterFromExplanation(const vector<unsigned> & cluster, const unsigned long & clustID) {
        for (vector<unsigned>::const_iterator it1 = cluster.begin(); it1 != cluster.end(); ++it1) {
            vector<unsigned>::const_iterator it2 = it1;
            ++it2;
            for ( ; it2 != cluster.end(); ++it2) {
                removeClusterToExplainEdge(*it1,*it2,clustID);
            }
        }
        currentClusters[clustID].setRemovedFromExplain();
    }

    inline void removeClusterFromExplanation(unsigned long clusterToRemove) {
        removeClusterFromExplanation(currentClusters[clusterToRemove].getElementsVector(), currentClusters[clusterToRemove].getID());
    }

    /*inline*/ unsigned long addCluster(const boost::dynamic_bitset<unsigned long> & newCluster, nodeDistanceObject & nodeDistances, vector<string> & nodeIDsToNames, graph_undirected_bitset & clusterGraph, unsigned & largestCluster) {
        unsigned long newClusterTrieID = 0;
        unsigned long newID = 0;
        if (clusterMap.addKey(newCluster) == true) {
            if (openIDs.size() != 0) {
                newID = openIDs.back();
                openIDs.pop_back();
            } else {
                newID = nextID;
                ++nextID;
            }

            if (currentClusters.size() <= newID) {
                currentClusters.resize(newID+1);
            }
            currentClusters[newID] = ClusterBitset(newCluster, newID, newClusterTrieID, curWeight);
            for (unsigned i = 0; i < newCluster.size() ;  ++i) {
                if (newCluster[i] == true) {
                    Utils::insertInOrder(nodesToClusters[i], newID);
                }
            }
            ++clustersAdded;
            ++newClusts;
            resetClusterWeight(newID, nodeDistances, clusterGraph, largestCluster);

        }
        return newID;
    }

    inline void printAll(vector<string> & nodeIDsToNames) {
        cout << "@ PRINTING CLUSTERS" << endl;
        for (vector<ClusterBitset>::iterator clustIt = currentClusters.begin(); clustIt != currentClusters.end(); ++clustIt) {
            if (clustIt->numElements() != 0) {
                cout << "@ ";
                printCluster(clustIt->getElements(), nodeIDsToNames);
                cout << endl;
            }
        }
        cout << "@ DONE PRINTING CLUSTERS" << endl;
    }

    inline void resetAllUnexplained() {
        for (vector<ClusterBitset>::iterator clustIt = currentClusters.begin(); clustIt != currentClusters.end(); ++clustIt) {
            if ((clustIt->numElements() != 0) && (!clustIt->isValid())) {
                clustIt->setUnexplainedUncounted();
            }
        }
    }

    inline unsigned setNumUniquelyUnexplainedEdges(unsigned long id) {
        unsigned ret = 0;
        const vector<unsigned> cluster = currentClusters[id].getElementsVector();
        for (vector<unsigned>::const_iterator it1 = cluster.begin(); it1 != cluster.end(); ++it1) {
            vector<unsigned>::const_iterator it2 = it1;
            ++it2;
            for ( ; it2 != cluster.end(); ++it2) {
                if (!isThisEdgeExplained(*it1,*it2)) {
                    if (edgesToClusters[*it1][*it2] == 1) {
                        ++ret;
                    }
                }
            }
        }
        currentClusters[id].setUniquelyExplainedEdges(ret);
        return ret;
    }

    inline void setAllNumUniquelyExplained() {
        for (vector<ClusterBitset>::iterator clustIt = currentClusters.begin();
             clustIt != currentClusters.end(); ++clustIt) {
            if ((clustIt->numElements() != 0) && (!clustIt->isValid())) {
                setNumUniquelyUnexplainedEdges(clustIt->getID());
            }
        }
    }

    /*inline*/ void setClusterValid(const boost::dynamic_bitset<unsigned long> & cluster, graph_undirected_bitset & clusterGraph) {
        vector<unsigned> clusterElems;
        clusterElems.reserve(cluster.size());
//        cout << "Debug:";
        for (unsigned i = cluster.find_first(); i < cluster.size(); i = cluster.find_next(i)) {
            clusterElems.push_back(i);
//            cout << i << " ";
        }
//        cout << endl;
        for (unsigned i = 0; i < clusterElems.size()-1;  ++i) {
            for (unsigned j = i+1; j < clusterElems.size(); ++j) {
                if (clusterGraph.isEdge(clusterElems[i], clusterElems[j])) //used to be a bug
                {
//                    cout << i << " " << j << endl;
                    setEdgeExplained(clusterElems[i], clusterElems[j]);
                }
            }
        }
        return;
    }

    inline void setClusterValid(unsigned long clusterID, graph_undirected_bitset & clusterGraph) {
        setNumUniquelyUnexplainedEdges(clusterID);
        setClusterValid(getElements(clusterID), clusterGraph);
        currentClusters[clusterID].setValid();
        return;
    }

    inline void setNecessary(unsigned long clusterID) {
        currentClusters[clusterID].setNecessary();
        return;
    }

    inline bool wasNecessary(unsigned long clusterID) {
        return currentClusters[clusterID].wasNecessary();
    }

    inline void setCheckedFinal(unsigned long clusterID) {
        currentClusters[clusterID].setCheckedFinal();
        return;
    }

    inline bool wasCheckedFinal(unsigned long clusterID) {
        return currentClusters[clusterID].wasCheckedFinal();
    }

    inline double getMaxThresh(unsigned long id) {
        return currentClusters[id].getThresh(alpha, getNumUnexplainedEdges(id));
    }

    inline double getThresh(unsigned long id) {
//        unsigned clust_size = currentClusters[id].size();
        if (!currentClusters[id].isValid() && !currentClusters[id].isUnexplainedCounted()) {
            setNumUniquelyUnexplainedEdges(id);
        }
        return currentClusters[id].getThresh(alpha, getNumUniquelyUnexplainedEdges(id));
    }

    inline double getMaxThresh() {
        unsigned numFound = 0;
        nextThreshold = 0;
        for (vector<ClusterBitset>::reverse_iterator clustIt = currentClusters.rbegin();
             clustIt != currentClusters.rend(); ++clustIt) {
            if (clustIt->isNew()) {
                ++numFound;
                double clustThresh = getMaxThresh(clustIt->getID());
                if (clustThresh > nextThreshold) {
                    nextThreshold = clustThresh;
                }
            }
            if (numFound == numNew()) {
                return nextThreshold;
            }
        }
        return nextThreshold;
    }

    inline double getNextThresh() {
        unsigned numFound = 0;
        nextThreshold = 0;
        for (vector<ClusterBitset>::reverse_iterator clustIt = currentClusters.rbegin();
             clustIt != currentClusters.rend(); ++clustIt) {
            if (clustIt->isNew()) {
                ++numFound;
                double clustThresh = getThresh(clustIt->getID());
                if (clustThresh > nextThreshold) {
                    nextThreshold = clustThresh;
                }
            }
            if (numFound == numNew()) {
                return nextThreshold;
            }
        }
        return nextThreshold;
    }

    inline double getMaxNewWeight() {
        unsigned numFound = 0;
        maxNewWeight = curWeight;
        for (vector<ClusterBitset>::reverse_iterator clustIt = currentClusters.rbegin();
             clustIt != currentClusters.rend(); ++clustIt) {
            if (clustIt->isNew()) {
                ++numFound;
                if (clustIt->getWeight() > maxNewWeight) {
                    maxNewWeight = clustIt->getWeight();
                }
            }
            if (numFound == numNew()) {
                return maxNewWeight;
            }
        }
        return maxNewWeight;
    }

    inline void resetMaxWeight() {
        unsigned numFound = 0;
        maxNewWeight = curWeight;
        for (vector<ClusterBitset>::reverse_iterator clustIt = currentClusters.rbegin();
             clustIt != currentClusters.rend(); ++clustIt) {
            if (clustIt->isNew()) {
                ++numFound;
                if (clustIt->getWeight() > maxNewWeight) {
                    maxNewWeight = clustIt->getWeight();
                }
            }
            if (numFound == numNew()) {
                return;
            }
        }
        return;
    }

    bool isMinNodeDegreeMet(unsigned cluster1, unsigned cluster2, graph_undirected_bitset & clusterGraph, double density) {
        boost::dynamic_bitset<unsigned long> combination = getElements(cluster1);

        combination |= getElements(cluster2);
        unsigned numCombined = combination.count();
        double denom = numCombined - 1;
        unsigned numChecked = 0;
        for (unsigned i = combination.find_first(); i < combination.size(); i = combination.find_next(i)) {
            boost::dynamic_bitset<unsigned long> interactorsInCombo = combination;
            interactorsInCombo &= clusterGraph.getInteractors(i);
            unsigned numInteractorsInCombo = interactorsInCombo.count();
            if ((numInteractorsInCombo / denom) < density) {
                return false;
            }
            ++numChecked;
            if (numChecked == numCombined) {
                return true;
            }
        }
        return true;
    }

    inline bool wouldClusterCombine(const unsigned long i, graph_undirected_bitset & clusterGraph, double density) {
        for (unsigned long j = 0; j < maxClusterID(); ++j) {
            if ((j != i) && (numElements(j) != 0)) {
                if (isMinNodeDegreeMet(i,j,clusterGraph,density)) {
                    return true;
                }
            }
        }
        return false;
    }


    inline void deleteCluster(const unsigned long & clusterToDelete, vector<string> & nodeIDsToNames, bool printClusterInfo = true) {

        if (currentClusters[clusterToDelete].isAddedToExplain()) {
            removeClusterFromExplanation(clusterToDelete);
        }
        openIDs.push_back(clusterToDelete);
        vector<ClusterBitset>::iterator clusterToDelete_it = currentClusters.begin();
        clusterToDelete_it += clusterToDelete;
        // it iterates through the nodes in the cluster to be deleted.  This allows us
        // to remove the now deleted cluster from the nodeToClusters structure
        for (unsigned i = 0; i < numNodes; ++i) {
            if (clusterToDelete_it->isElement(i)) {
                Utils::eraseInOrder(nodesToClusters[i], clusterToDelete);
            }
        }
        clusterMap.deleteKey(clusterToDelete_it->getElements());
        ++clustersDeleted;
        if (clusterToDelete_it->isNew()) {
            --newClusts;
        }
        *clusterToDelete_it = ClusterBitset();

        return;
    }

    /*inline*/ void deleteClusters(vector<unsigned long> & clustersToDelete, vector<string> & nodeIDsToNames, graph_undirected_bitset & clusterGraph) {

        for (vector<unsigned long>::iterator clustersToDelete_it = clustersToDelete.begin();
             clustersToDelete_it != clustersToDelete.end(); ++clustersToDelete_it) {
            deleteCluster(*clustersToDelete_it, nodeIDsToNames);
        }

        return;
    }

    inline unsigned numNew() {
        return newClusts;
    }


    inline unsigned numClustersWithNode(unsigned nodeID) {
        return nodesToClusters[nodeID].size();
    }

    inline vector<ClusterBitset>::iterator clustersBegin() {
        return currentClusters.begin();
    }

    inline vector<ClusterBitset>::iterator clustersEnd() {
        return currentClusters.end();
    }

    inline unsigned numCurrentClusters() {
        return currentClusters.size() - openIDs.size();
    }

    inline vector<unsigned long>::iterator clustersWithNodeBegin(unsigned nodeID) {
        return nodesToClusters[nodeID].begin();
    }

    inline vector<unsigned long>::iterator clustersWithNodeEnd(unsigned nodeID) {
        return nodesToClusters[nodeID].end();
    }


    /*inline*/ void sortNewClusters(vector<unsigned long> & sortedNewClusters) {//if robustness not desired, considering sorting by weight
        vector<pair<unsigned long, unsigned> > newClustersAndCounts;
        newClustersAndCounts.reserve(numCurrentClusters());
        unsigned numFound = 0;
        for (vector<ClusterBitset>::reverse_iterator clustIt = currentClusters.rbegin();
             clustIt != currentClusters.rend(); ++clustIt) {
            if (clustIt->numElements() != 0) {
                newClustersAndCounts.push_back(make_pair(clustIt->getID(), clustIt->numElements()));
                ++numFound;
            }
        }//is this only new clusters?
        sort(newClustersAndCounts.begin(), newClustersAndCounts.end(), compPairSecondAscending);

        sortedNewClusters.reserve(numCurrentClusters());
        for (vector<pair<unsigned long, unsigned> >::iterator it = newClustersAndCounts.begin();
             it != newClustersAndCounts.end(); ++it) {
            sortedNewClusters.push_back(it->first);
        }
        return;
    }

    inline void clearEdgesToClusters() {
        for (unsigned i = 0; i < numNodes; ++i) {
            for (unsigned j = (i+1); j < numNodes; ++j) {
                edgesToClusters[i][j] = 0;
            }
        }
        return;
    }

    inline void addClustersToExplanations(vector<unsigned long> & sortedNewClusters, graph_undirected_bitset & clusterGraph) {
        for (vector<unsigned long>::iterator newClustIt = sortedNewClusters.begin();
             newClustIt != sortedNewClusters.end(); ++newClustIt) {
            if (!currentClusters[*newClustIt].isAddedToExplain()) {
                addClusterToExplanation(currentClusters[*newClustIt].getElementsVector(), *newClustIt, clusterGraph);
            }
        }
        return;
    }


    /*inline*/ void prepareForValidityCheck(vector<unsigned long> & sortedNewClusters,graph_undirected_bitset & clusterGraph) {
        sortNewClusters(sortedNewClusters);
        addClustersToExplanations(sortedNewClusters, clusterGraph);
        return;
    }

    bool isLargestExplainer(unsigned edgeEnd1, unsigned edgeEnd2, unsigned long id, vector<char> & idsChecked) {
        unsigned smallest = edgeEnd1;
        unsigned other = edgeEnd2;
        if (numClustersWithNode(edgeEnd2) < numClustersWithNode(edgeEnd1)) {
            smallest = edgeEnd2;
            other = edgeEnd1;
        }
        for (vector<unsigned long>::iterator nodeToClustersIt = clustersWithNodeBegin(smallest);
             nodeToClustersIt != clustersWithNodeEnd(smallest); ++nodeToClustersIt) {
            if (!idsChecked[*nodeToClustersIt] && currentClusters[*nodeToClustersIt].isElement(other)) {
                return false;
            }
        }
        return true;
    }

    // Will set isNecessary to true if is only cluster covering an edge
    // Will return true if cluster is only cluster covering an edge which is not previously explained
    inline bool checkClusterFinalValidity(unsigned long id, bool & isNecessary, vector<char> & idsChecked, bool checkForFinal = true) {
        isNecessary = false;
//        vector<pair<unsigned, unsigned> > unexplainedEdges;
        const vector<unsigned> cluster = currentClusters[id].getElementsVector();
//        unexplainedEdges.reserve(cluster.size()*(cluster.size()-1)/2);
        idsChecked[id] = 1;

        if (!checkForFinal) {
            return false;
            for (vector<unsigned>::const_iterator it1 = cluster.begin(); it1 != cluster.end(); ++it1) {
                vector<unsigned>::const_iterator it2 = it1;
                ++it2;
                for ( ; it2 != cluster.end(); ++it2) {
                    if (edgesToClusters[*it1][*it2] == 1) { // what's the point? will return false anyway
                        isNecessary = true;
                        return false;
                    }
                }
            }
        } else {
            for (vector<unsigned>::const_iterator it1 = cluster.begin(); it1 != cluster.end(); ++it1) {
                vector<unsigned>::const_iterator it2 = it1;
                ++it2;
                for ( ; it2 != cluster.end(); ++it2) {
                    if (edgesToClusters[*it1][*it2] == 1) {
                        isNecessary = true; // do I still need this?
                        if (!isThisEdgeExplained(*it1,*it2)) {
                            return true;
                        }
                    }
//                    if (!isThisEdgeExplained(*it1,*it2)) {
//                        return true;
////                        unexplainedEdges.push_back(make_pair(*it1,*it2));
//                    }
                }
            }
//            if (checkForFinal && isNecessary) {
//                //cout << "Necessary but not yet valid" << endl;
//                for (vector<pair<unsigned, unsigned> >::iterator unexplainedIt = unexplainedEdges.begin();
//                     unexplainedIt != unexplainedEdges.end(); ++unexplainedIt) {
//                    if (isLargestExplainer(unexplainedIt->first, unexplainedIt->second, id, idsChecked)) {//ewww?
////                        //cout << "Now valid" << endl;
////                        //cout << id << " explains " << unexplainedIt->first << " " << unexplainedIt->second << " via largest possible" << endl;
//                        return true;
//                    }
//                }
//                return true;
//                //cout << "Not valid" << endl;
//            }
        }
        return false;
    }

    inline unsigned getNumUnexplainedEdges(unsigned long id) {
        unsigned ret = 0;
        const vector<unsigned> cluster = currentClusters[id].getElementsVector();
        for (vector<unsigned>::const_iterator it1 = cluster.begin(); it1 != cluster.end(); ++it1) {
            vector<unsigned>::const_iterator it2 = it1;
            ++it2;
            for ( ; it2 != cluster.end(); ++it2) {
                if (!isThisEdgeExplained(*it1,*it2)) {
                    //if (edgesToClusters[*it1][*it2] == 1) {
                    ++ret;
                    //}
                }
            }
        }
        return ret;
    }

    inline unsigned getNumUniquelyUnexplainedEdges(unsigned long id) {
        return currentClusters[id].getUniquelyExplainedEdges();
    }

    inline bool checkClusterValidity(unsigned long id) {
        const vector<unsigned> cluster = currentClusters[id].getElementsVector();
        for (vector<unsigned>::const_iterator it1 = cluster.begin(); it1 != cluster.end(); ++it1) {
            vector<unsigned>::const_iterator it2 = it1;
            ++it2;
            for ( ; it2 != cluster.end(); ++it2) {
                if (edgesToClusters[*it1][*it2] == 1) {// this edge only appears in one cluster. may use realedges so fewer necessary clusters?
                    setNecessary(id);
                    return true;
                }
            }
        }
        return false;
    }

    inline const boost::dynamic_bitset<unsigned long> & getElements(unsigned long id) {
        return currentClusters[id].getElements();
    }

    inline unsigned numElements(unsigned long id) {
        return currentClusters[id].numElements();
    }

    inline void setOld(unsigned long id) {
        if (currentClusters[id].isNew()){
            --newClusts;
        }
        return currentClusters[id].setOld();
    }

    inline void setNew(unsigned long id) {
        if (!currentClusters[id].isNew()) {
            ++newClusts;
        }
        return currentClusters[id].setNew();
    }

    inline bool isNew(unsigned long id) {
        return currentClusters[id].isNew();
    }

    inline double getClusterWeight(unsigned long id) {
        return currentClusters[id].getWeight();
    }

    inline unsigned long maxClusterID() {
        return currentClusters.size();
    }

    inline double getCurWeight() {
        return curWeight;
    }

    inline double getMinWeightAdded() {
        return minWeightAdded;
    }

    void resetClusterWeight(unsigned long id, nodeDistanceObject & nodeDistances, graph_undirected_bitset & clusterGraph, unsigned & largestCluster) {
//        double minWeight = firstWeight;
        vector <double> newEdgeWeights;//changed; actually doesn't have to be new
        newEdgeWeights.reserve(100000);
        unsigned nNewEdges = 0;
        double sumNewEdgeWeights = 0;
        const vector<unsigned> cluster = currentClusters[id].getElementsVector();
        for (vector<unsigned>::const_iterator it1 = cluster.begin(); it1 != cluster.end(); ++it1) {
            vector<unsigned>::const_iterator it2 = it1;
            ++it2;
            for ( ; it2 != cluster.end(); ++it2) {
//                if (!isThisEdgeExplained(*it1,*it2) && (edgesToClusters[*it1][*it2] == 1)) {
                double thisDistance = nodeDistances.getDistance(*it1,*it2);
                if ( (!isThisEdgeExplained(*it1, *it2) ) && (clusterGraph.isEdge(*it1, *it2))  ) { // second condition seems useless
//                if (clusterGraph.isEdge(*it1, *it2)) {
//                    if (thisDistance < minWeight) {
//                        minWeight = thisDistance;
//                    }
                    newEdgeWeights.push_back(thisDistance);
                    sumNewEdgeWeights = sumNewEdgeWeights + thisDistance;
                    nNewEdges++;
                }
            }
        }
        sort(newEdgeWeights.begin(), newEdgeWeights.end());
        double newEdgeWeightsMedian;
//        cout << nNewEdges << endl;
        if (nNewEdges ==0) {
            newEdgeWeightsMedian = 0;
        }
//        else if (nNewEdges  % 2 == 0)
//        {
////            newEdgeWeightsMedian = (newEdgeWeights[nNewEdges / 2 - 1] + newEdgeWeights[nNewEdges/ 2]) / 2;
//            newEdgeWeightsMedian = newEdgeWeights[nNewEdges / 2];
//        }
        else
        {
            newEdgeWeightsMedian = newEdgeWeights[(nNewEdges-1) / 2];
        }
//        newEdgeWeightsMedian = newEdgeWeights[nNewEdges / 2];
        //        cout << nNewEdges << endl;
//        currentClusters[id].setWeight(newEdgeWeightsMedian); // the cluster weight is set as the min of the real edges;

//        unsigned clusterSize = currentClusters[id].getElements().count();
//        if (clusterSize > largestCluster) {
//            currentClusters[id].setWeight(newEdgeWeightsMedian);
//        }
//        else {
//            currentClusters[id].setWeight(minWeight);
//        }
        double newEdgeWeightsMean = sumNewEdgeWeights / nNewEdges;
        newEdgeWeightsMean = floor(newEdgeWeightsMean*1000+0.5)/1000;
        currentClusters[id].setWeight(newEdgeWeightsMean);
        return;
    }

    bool checkAllForMaximality(graph_undirected_bitset & clusterGraph, vector<string> & nodeIDsToNames) {
        bool ret = false;
        cout << "CHECKING FOR MAXIMALITY" << endl;
        cout << "Num current clusters: " << numCurrentClusters() << endl;
        for (vector<ClusterBitset>::iterator clustIt = currentClusters.begin(); clustIt != currentClusters.end(); ++clustIt) {
            if (clustIt->numElements() != 0) {
                bool isMaximal = true;
                unsigned nextElem = clustIt->getElements().find_first();
                for (unsigned i = 0; i < clustIt->size(); ++i) {
                    if (i == nextElem) {
                        nextElem = clustIt->getElements().find_next(nextElem);
                    } else if (clustIt->getElements().is_subset_of(clusterGraph.getInteractors(i))) {
                        isMaximal = false;
                        cout << "CLUSTER NOT MAXIMAL" << endl;
                        ret = true;
                        for (unsigned j = clustIt->getElements().find_first(); j < clustIt->size(); j = clustIt->getElements().find_next(j)) {
                            cout << nodeIDsToNames[j] << " ";
                        }
                        cout << endl;
                        cout << "SHOULD ALSO CONTAIN: " << nodeIDsToNames[i] << endl;
                    }
                }
                if (isMaximal) {
                    for (unsigned j = clustIt->getElements().find_first(); j < clustIt->size(); j = clustIt->getElements().find_next(j)) {
                        cout << nodeIDsToNames[j] << " ";
                    }
                    cout << endl;
                }
            }
        }
        return ret;
    }

    unsigned numEdgesCovered() {
        graph_undirected_bitset coveredEdges(numNodes);
        for (vector<ClusterBitset>::iterator clustIt = currentClusters.begin(); clustIt != currentClusters.end(); ++clustIt) {
            if (clustIt->numElements() != 0) {
                vector<unsigned> clusterElems = clustIt->getElementsVector();
                for (unsigned i = 0; i < clusterElems.size()-1; ++i) {
                    for (unsigned j = i+1; j < clusterElems.size(); ++j) {
                        if (!coveredEdges.isEdge(clusterElems[i],clusterElems[j])) {
                            coveredEdges.addEdge(clusterElems[i],clusterElems[j]);
                        }
                    }
                }
            }
        }
        return coveredEdges.numEdges();
    }

    unsigned clustersAdded;
    unsigned clustersDeleted;
    unsigned clustersExtended;

    inline vector<pair<double,unsigned> >::iterator historicalWeightsBegin(unsigned long id) {
        return currentClusters[id].historicalWeightsBegin();
    }

    inline vector<pair<double,unsigned> >::iterator historicalWeightsEnd(unsigned long id) {
        return currentClusters[id].historicalWeightsEnd();
    }

    inline void clearHistoricalWeights(unsigned long id) {
        currentClusters[id].clearHistoricalWeights();
    }

    inline void combining() {
        combiningNow = true;
    }

    inline void doneCombining() {
        combiningNow = false;
    }

    inline bool isCombiningNow() {
        return combiningNow;
    }

    inline double getFirstWeight() {
        return firstWeight;
    }

private:
    //Trie clusterTrie;
    clusterMapClass clusterMap;
    vector<ClusterBitset> currentClusters;
    vector<unsigned long> openIDs;

    // Each node has an ID.  The nodesToClusters object can be accessed by that ID,
    // giving a set of iterators to cluster objects in the currentClusters list
    // which can then be derefenced to get a vector<unsigned> for the cluster
    //vector<set<list<ClusterBitset>::iterator, compListClusterBitsetIterator > > nodesToClusters;
    vector<vector<unsigned long> > nodesToClusters;
    vector<vector<unsigned> > edgesToClusters;
    vector<vector<char> > isEdgeExplained;

    unsigned long numClusters;
    unsigned long nextID;
    unsigned newClusts;
    unsigned numNodes;
    double curWeight;
    double minWeightAdded;
    double firstWeight;
    bool combiningNow;
    double alpha;

    // Maximum weight of any new cluster curently in this list
    double maxNewWeight;
    double nextThreshold;

};

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace dagConstruct {

    inline void setIntersection(const vector<unsigned>& i, const vector <unsigned>& j, vector<unsigned> & intersection) {
        intersection.clear();
        intersection.reserve(i.size());
        intersection.reserve(j.size());
        vector<unsigned>::const_iterator i_it = i.begin();
        vector<unsigned>::const_iterator j_it = j.begin();
        while ((i_it != i.end()) && (j_it != j.end())) {
            if (*i_it == *j_it) {
                intersection.push_back(*i_it);
                ++i_it;
                ++j_it;
            } else if (*i_it < *j_it) {
                ++i_it;
            } else if (*j_it < *i_it) {
                ++j_it;
            }
        }
    }

    // newCluster = intersection of currentCluster and neighborsOfNewInteractor + newInteractor
    inline void getNewCluster(const boost::dynamic_bitset<unsigned long>& currentCluster, const boost::dynamic_bitset<unsigned long>& neighborsOfNewInteractor,
                              unsigned newInteractor, boost::dynamic_bitset<unsigned long> & newCluster) {
        newCluster = currentCluster;
        newCluster &= neighborsOfNewInteractor;
        newCluster[newInteractor] = 1;
        return;
    }

    bool isClusterAncestor(const boost::dynamic_bitset<unsigned long> & cluster, const boost::dynamic_bitset<unsigned long> & possibleAncestorCluster, boost::dynamic_bitset<unsigned long> & unaccountedFor) {
        // Keep track of which genes in the ancestor cluster are "accounted for" (i.e. contained in) decsendent
        if (cluster.is_subset_of(possibleAncestorCluster)) {
            unaccountedFor -= cluster;
            return true;
        }
        return false;
    }

    bool isClusterAncestor(const vector<unsigned> & cluster, const vector<unsigned> & possibleAncestorCluster) {
        vector<unsigned>::const_iterator clusterIt = cluster.begin();
        vector<unsigned>::const_iterator ancestorIt = possibleAncestorCluster.begin();
        while ((clusterIt != cluster.end()) && (ancestorIt != possibleAncestorCluster.end())) {
            if (*ancestorIt < *clusterIt) {
                ++ancestorIt;
            } else if (*ancestorIt > *clusterIt) {
                return false;
            } else if (*ancestorIt == *clusterIt) {
                ++ancestorIt;
                ++clusterIt;
            }
        }
        if (clusterIt == cluster.end()) {
            return true;
        }
        return false;
    }

    inline bool checkClusterValidity(const vector<unsigned> & cluster, nodeDistanceObject & nodeDistances, vector<vector<bool> > & pairLCAFound) {
        bool firstPairFound = false;
        double firstPairDistance;
        for (unsigned i = 0; i < (cluster.size() - 1); ++i) {
            for (unsigned j = (i+1); j < cluster.size(); ++j) {
                if (!pairLCAFound[cluster[i]][cluster[j]]) {
                    if (firstPairFound) {
                        if (firstPairDistance != nodeDistances.getDistance(cluster[i],cluster[j])) {
                            return false;
                        }
                    } else {
                        firstPairFound = true;
                        firstPairDistance = nodeDistances.getDistance(cluster[i],cluster[j]);
                    }
                }
            }
        }
        if (firstPairFound) {
            for (unsigned i = 0; i < (cluster.size() - 1); ++i) {
                for (unsigned j = (i+1); j < cluster.size(); ++j) {
                    pairLCAFound[cluster[i]][cluster[j]] = true;
                }
            }
            return true;
        } else {
            return false;
        }
    }

    inline bool checkClusterValidity(const boost::dynamic_bitset<unsigned long> & cluster, nodeDistanceObject & nodeDistances, vector<vector<bool> > & pairLCAFound) {
        bool firstPairFound = false;
        double firstPairDistance;
        for (unsigned i = 0; i < (cluster.size() - 1); ++i) {
            if (cluster[i] == true) {
                for (unsigned j = (i+1); j < cluster.size(); ++j) {
                    if (cluster[j] == true) {
                        if (!pairLCAFound[i][j]) {
                            if (firstPairFound) {
                                if (firstPairDistance != nodeDistances.getDistance(i,j)) {
                                    return false;
                                }
                            } else {
                                firstPairFound = true;
                                firstPairDistance = nodeDistances.getDistance(i,j);
                            }
                        }
                    }
                }
            }
        }
        if (firstPairFound) {
            for (unsigned i = 0; i < (cluster.size() - 1); ++i) {
                if (cluster[i] == true) {
                    for (unsigned j = (i+1); j < cluster.size(); ++j) {
                        if (cluster[j] == true) {
                            pairLCAFound[i][j] = true;
                        }
                    }
                }
            }
            return true;
        } else {
            return false;
        }
    }

    // RETURNS WHETHER ANYTHING WAS DELETED
    // (No longer does as follows) RETURNS THE NUMBER OF EXTENSIONS OF THE LARGEST DELETED CLUSTER.
    // THIS INFO IS PASSED TO THE NEW CLUSTER BEING EXTENDED
    /*inline*/ unsigned checkForDelete(currentClusterClassBitset & currentClusters,
                                       unsigned otherNode, unsigned nodeToUse,
                                       boost::dynamic_bitset<unsigned long> & newCluster,
                                       vector<unsigned long> & clustersToDelete) {
        bool retVal = false;
        for (vector<unsigned long>::iterator nodeToClustersIt = currentClusters.clustersWithNodeBegin(otherNode);
             nodeToClustersIt != currentClusters.clustersWithNodeEnd(otherNode); ++nodeToClustersIt) {
            if (currentClusters.getElements(*nodeToClustersIt).is_proper_subset_of(newCluster)) {
                Utils::insertInOrder(clustersToDelete,*nodeToClustersIt);
                retVal = true;
            }
        }
        return retVal;
        //return maxExtendedDelete;
    }


    inline void performValidityCheck(currentClusterClassBitset & currentClusters, graph_undirected_bitset & clusterGraph,
                                     nodeDistanceObject & nodeDistances, vector<string> & nodeIDsToNames) {
        vector<unsigned long> newClustersSorted; // where is this given?
        currentClusters.prepareForValidityCheck(newClustersSorted, clusterGraph);
        unsigned deleted = 0;
//        cout << "# Perform validity check for necessary clusters: ";
        for (vector<unsigned long>::iterator newClusterIt = newClustersSorted.begin();
             newClusterIt != newClustersSorted.end(); ++newClusterIt) {
            if (!currentClusters.checkClusterValidity(*newClusterIt)) {
                currentClusters.deleteCluster(*newClusterIt, nodeIDsToNames,false);
                ++deleted;
            }
        }
        currentClusters.resetAllUnexplained(); // this only reset per cluster features

        return;
    }


    bool isMinNodeDegreeMet(unsigned cluster1, unsigned cluster2, currentClusterClassBitset &currentClusters,
                            graph_undirected_bitset &clusterGraph, double density, vector<string> & nodeIDsToNames) {
        boost::dynamic_bitset<unsigned long> combination = currentClusters.getElements(cluster1);

//        cout << "# check two clusters to be combined" << endl;
//        printCluster(currentClusters.getElements(cluster1), nodeIDsToNames);
//        printCluster(currentClusters.getElements(cluster2), nodeIDsToNames);

        combination |= currentClusters.getElements(cluster2);
        unsigned numCombined = combination.count();
        double denom = numCombined - 1;
        unsigned numChecked = 0;
        for (unsigned i = combination.find_first(); i < combination.size(); i = combination.find_next(i)) {// how to output genes here
            boost::dynamic_bitset<unsigned long> interactorsInCombo = combination;
            interactorsInCombo &= clusterGraph.getInteractors(i);
            unsigned numInteractorsInCombo = interactorsInCombo.count();
            if ((numInteractorsInCombo / denom) <= density ) {
//                cout << "# two clusters not to be combined " << numInteractorsInCombo / denom << endl;
                return false;
            }
            ++numChecked;
            if (numChecked == numCombined) {
//                printCluster(currentClusters.getElements(cluster1), nodeIDsToNames);
//                cout << endl;
//                printCluster(currentClusters.getElements(cluster2), nodeIDsToNames);
//                cout << endl;
//                cout << "# two clusters can be combined" << endl;
                return true;
            }
        }
        return true;
    }

    // Checks the starting cluster for possible extensions based on the new clusterGraph
    void checkForExtensions(unsigned i, unsigned nextElem, boost::dynamic_bitset<unsigned long> startingCluster, graph_undirected_bitset & clusterGraph,
                            vector<boost::dynamic_bitset<unsigned long> > & newClustersToAdd) {
        for ( ; i < clusterGraph.numNodes(); ++i) {
            if (i == nextElem) {
                nextElem = startingCluster.find_next(nextElem);
            } else if (startingCluster.is_subset_of(clusterGraph.getInteractors(i))) {

                // N(i) contains the startingCluster
                boost::dynamic_bitset<unsigned long> extendedCluster = startingCluster;
                extendedCluster[i] = 1;

                // Is there some other element j where j < i and N(j) includes this extended cluster? (Maximality test)
                bool extend = true;
                for (unsigned j = 0; j < i; ++j) {
                    if ((extendedCluster[j] == 0) && (extendedCluster.is_subset_of(clusterGraph.getInteractors(j)))) {

                        // There is some j where j < i and N(j) contains this extended Cluster
                        extend = false;
                        break;
                    }
                }

                if (extend) {
                    checkForExtensions(i+1, nextElem, extendedCluster, clusterGraph, newClustersToAdd);
                }

            }
        }

        Utils::insertInOrder(newClustersToAdd, startingCluster);
    }

// Core algorithm used before
    void updateCliques(graph_undirected_bitset & clusterGraph,
                       vector<boost::dynamic_bitset<unsigned long> > & newClustersToAdd,
                       boost::dynamic_bitset<unsigned long> startClust, vector<unsigned> & neighborsOfBoth,
                       vector<char> & neighborsOfBothBits, unsigned i, vector<string> & nodeIDsToNames) {
        if (i == neighborsOfBoth.size()) {
            if (!Utils::insertInOrder(newClustersToAdd, startClust)) {
                cout << "# Repeated" << endl;
            } /*else {
	cout << "# Adding ";
	printCluster(startClust, nodeIDsToNames);
	cout << endl;
	}*/
            return;
        }
        //printCluster(startClust, nodeIDsToNames);
        //cout << endl;
        if (startClust.is_subset_of(clusterGraph.getInteractors(neighborsOfBoth[i]))) {
            startClust[neighborsOfBoth[i]] = 1; //changed here. Why no ampersand?
            updateCliques(clusterGraph, newClustersToAdd, startClust, neighborsOfBoth, neighborsOfBothBits, i + 1, nodeIDsToNames);
        } else {
            updateCliques(clusterGraph, newClustersToAdd, startClust, neighborsOfBoth, neighborsOfBothBits, i + 1, nodeIDsToNames);

            // Calculate T[y] = |N(y) intersection with C intersection with N(i)| for y not in C, not i
            vector<unsigned> T(clusterGraph.numNodes(), 0);

            boost::dynamic_bitset<unsigned long> nextClust = startClust;
            nextClust &= clusterGraph.getInteractors(neighborsOfBoth[i]);
            unsigned C_int_Ni_Count = nextClust.count();

            for (unsigned x = nextClust.find_first(); x < clusterGraph.numNodes(); x = nextClust.find_next(x)) {
                for (unsigned y = clusterGraph.getInteractors(x).find_first(); y < clusterGraph.numNodes();
                     y = clusterGraph.getInteractors(x).find_next(y)) {
                    if (( y != neighborsOfBoth[i] ) && (startClust[y] == 0)) {
                        ++T[y];
                        if ((y < neighborsOfBoth[i]) && (T[y] == C_int_Ni_Count)) {
                            return;
                        }
                    }
                }
            }

            // Calculate S[y] = |N(y) intersection with (C - N(i))| for y not in C
            vector<unsigned> S(clusterGraph.numNodes(), 0);
            boost::dynamic_bitset<unsigned long> C_not_Ni = startClust;
            C_not_Ni -= clusterGraph.getInteractors(neighborsOfBoth[i]);
            unsigned C_not_Ni_count = C_not_Ni.count();
            for (unsigned x = C_not_Ni.find_first(); x < clusterGraph.numNodes(); x = C_not_Ni.find_next(x)) {
                for (unsigned y = clusterGraph.getInteractors(x).find_first(); y < clusterGraph.numNodes();
                     y = clusterGraph.getInteractors(x).find_next(y)) {
                    if ((startClust[y] == 0) && (neighborsOfBothBits[y] == 1)) {
                        ++S[y];
                    }
                }
            }

            unsigned k = 0;
            unsigned last_jk = 0;
            for (unsigned jk = C_not_Ni.find_first(); jk < clusterGraph.numNodes(); jk = C_not_Ni.find_next(jk)) {
                boost::dynamic_bitset<unsigned long> Njk_not_C = clusterGraph.getInteractors(jk);
                for (unsigned y = Njk_not_C.find_first(); y < neighborsOfBoth[i]; y = Njk_not_C.find_next(y)) {
                    if ((T[y] == C_int_Ni_Count) && (neighborsOfBothBits[y] == 1)) {
                        if (y >= jk) {
                            --S[y];

                            // Might need to look here
                        } else if (((S[y] + k) == C_not_Ni_count) && (y >= last_jk)) {
                            return;
                        }
                    }
                }
                last_jk = jk;
                ++k;
            }

            unsigned jp = last_jk;
            if (jp < (neighborsOfBoth[i]-1)) {
                return;
            }
            unsigned inNext = nextClust.find_first();
            for (unsigned y = clusterGraph.getInteractors(inNext).find_first(); y < clusterGraph.numNodes();
                 y = clusterGraph.getInteractors(inNext).find_next(y)) {
                if ((y < neighborsOfBoth[i]) && (startClust[y] == 0) && (T[y] == C_int_Ni_Count) && (S[y] == 0) && (jp < y)) {
                    return;
                }
            }
            nextClust[neighborsOfBoth[i]] = 1;
            updateCliques(clusterGraph, newClustersToAdd, nextClust, neighborsOfBoth, neighborsOfBothBits, i + 1, nodeIDsToNames);
        }
    }

    // Bron-Kersborch algorithm, with pivot selection of Tomita et al. 2006
    inline const boost::dynamic_bitset<unsigned long> clique_enum_tomita_pivot(graph_undirected_bitset & clusterGraph, boost::dynamic_bitset<unsigned long> & p, boost::dynamic_bitset<unsigned long> & x, boost::dynamic_bitset<unsigned long> & r) {
        unsigned most = 0;
        unsigned long q = 0;

        boost::dynamic_bitset<unsigned long> nv(clusterGraph.numNodes());
        boost::dynamic_bitset<unsigned long> Q(clusterGraph.numNodes());

        for (unsigned long v = x.find_first(); v < x.size(); v = x.find_next(v)) {
//            cout << p.size() <<";"<< nv.size() << endl;
            nv = clusterGraph.getInteractors(v);
//            cout << p.size() <<";"<< nv.size() << endl;
            boost::dynamic_bitset<unsigned long> common = p & nv;
            unsigned count = common.count() + 1;
//            cout << endl;
            if (count > most)
            {
                most = count;
                q = v;
            }
        }

        for (unsigned long v = p.find_first(); v < p.size(); v = p.find_next(v)) {
            nv = clusterGraph.getInteractors(v);
            boost::dynamic_bitset<unsigned long> common = p & nv;
            unsigned count = common.count() + 1;
            if (count > most)
            {
                most = count;
                q = v;
            }
        }
        nv = clusterGraph.getInteractors(q);
        Q = p - nv; //this should be equivalent to intset_copy_remove(p, nv)
        return Q;
    }

    unsigned long clique_enum_tomita_apply(graph_undirected_bitset & clusterGraph, boost::dynamic_bitset<unsigned long> & p, boost::dynamic_bitset<unsigned long> & x, boost::dynamic_bitset<unsigned long> & r, vector<string> & nodeIDsToNames, vector<boost::dynamic_bitset<unsigned long> > & newClustersToAdd) {
        //p: CAND, ; x: FINI; r:Q
//        ++recursion;
        unsigned long recur = 1;
        boost::dynamic_bitset<unsigned long>  q = clique_enum_tomita_pivot(clusterGraph, p, x, r);//what if no pivot

        if ((p.count() != 0) && (q.count() !=0)) { //figure this out
            boost::dynamic_bitset<unsigned long>  p2(clusterGraph.numNodes());
            boost::dynamic_bitset<unsigned long>  x2(clusterGraph.numNodes());

            boost::dynamic_bitset<unsigned long> nv(clusterGraph.numNodes());

            for (unsigned long v = q.find_first(); v < q.size(); v = q.find_next(v)) {
                nv = clusterGraph.getInteractors(v);
                p2 = p & nv;
                x2 = x & nv;
                r[v] = 1;
                recur = recur + clique_enum_tomita_apply(clusterGraph, p2, x2, r, nodeIDsToNames, newClustersToAdd);
                p[v] = 0;
                r[v] = 0;
                x[v] = 1;
            }
        }
        else if ((p.count()==0) && (x.count() == 0) ) {
            Utils::insertInOrder(newClustersToAdd, r);
        }
        return recur;
    }

    // experimental: clustering coefficient
    double clusterCoefficient(boost::dynamic_bitset<unsigned long> & interactors, graph_undirected_bitset & graph) {
        double cc = 0;
        unsigned numEdges = 0;
        double denom = interactors.count() * (interactors.count()-1) /2;
        if (denom < 1) {
            return 0;
        }
        for (unsigned i = interactors.find_first(); i < interactors.size()-1; i = interactors.find_next(i)) {
            for (unsigned j = interactors.find_next(i); j < interactors.size(); j = interactors.find_next(j)) {
                if (graph.isEdge(i, j)) {
                    ++numEdges;
                }
            }
        }
        cc = numEdges / denom;
//        cout << cc << " ";
        return cc;
    }


    // graph triangulation
    void elinminationGame(graph_undirected_bitset & clusterGraph, graph_undirected_bitset & chordGraph) {

        graph_undirected_bitset helperGraph(clusterGraph.numNodes());
        boost::dynamic_bitset<unsigned long> removed(clusterGraph.numNodes());

        unsigned helperEdges = 0;

        //filter by cluster coeffcient
//        for (unsigned i = 0; i < clusterGraph.numNodes(); ++i) {
//            boost::dynamic_bitset<unsigned long>  ni = clusterGraph.getInteractors(i);
//            double clustCoef = clusterCoefficient(ni, clusterGraph);
////            cout << clustCoef << endl;
//            if (clustCoef < globalDensity) {
//                removed[i] = 1;
//            }
//        }
        cout << "# " << removed.count() << " removed by cluster coefficient filters" << endl;

        //initialize the degree and chordal graph
        for (unsigned i = 0; i < clusterGraph.numNodes()-1; ++i) {
            for (unsigned j = i+1; j < clusterGraph.numNodes(); ++j) {
                if ((!removed[i]) && (!removed[j]) && clusterGraph.isEdge(i, j)) {
                    chordGraph.addEdge(i, j);
                    helperGraph.addEdge(i, j);
                }
            }
        }
//        boost::dynamic_bitset<unsigned long> removed(clusterGraph.numNodes());

        for (unsigned k = 0; k < clusterGraph.numNodes(); k++) { // this is n^3 in worst case
            //choose a v
            unsigned long pivotV = 0;
            unsigned long mindeg = clusterGraph.numNodes();

//            double maxclustCoef = 0;

            for (unsigned long v = 0;  v < clusterGraph.numNodes(); ++v) {
//                cout << v << endl;
                if (!removed[v]) {
                    unsigned long deg = helperGraph.getInteractors(v).count();
                    if ((deg >=2) && (deg < mindeg)) {
                        mindeg = deg;
                        pivotV = v;
                    }


                    // test a new unit, sorted by cluster coefficient. Will this be too slow?
//                    boost::dynamic_bitset<unsigned long>  interactors = helperGraph.getInteractors(v);
//                    double clustCoef = clusterCoefficient(interactors, helperGraph);
//                    cout << clustCoef << endl;
//                    if ((interactors.count() >= 2)  && (clustCoef > maxclustCoef)) {
//                        maxclustCoef = clustCoef;
//                        pivotV = v;
//                    }
                }
            }
//            cout << pivotV << endl;
//            cout << removed[pivotV] << " " << mindeg << endl;
            boost::dynamic_bitset<unsigned long> nv = helperGraph.getInteractors(pivotV);
            if (nv.count() < 2) {
                break;
            }
//            if (maxclustCoef < 0.5) {
//                break;
//            }

//            cout << nv.count() <<endl;
            for (unsigned long i = nv.find_first(); i < nv.size() -1; i = nv.find_next(i)) {  // a little worried
                for (unsigned long j = nv.find_next(i); j < nv.size(); j = nv.find_next(j)) {
//                    cout << i << " " << j << endl;
                    if (!helperGraph.isEdge(i, j)){
                        helperGraph.addEdge(i, j);
                        chordGraph.addEdge(i, j);
                        clusterGraph.addEdge(i, j);
                        ++helperEdges; // this maybe time consuming step
                    }
                }
            }
            // delete mindegV from helperGraph
            removed[pivotV] = 1;
            for (unsigned long i = nv.find_first(); i < nv.size(); i = nv.find_next(i) ) {
                helperGraph.removeEdge(i, pivotV);
            }
        }

        // remove those not meeting maxClustCoef criteria
//        for (unsigned long i = 0; i < removed.size(); ++i) {
//            if (!removed[i]) {
//                boost::dynamic_bitset<unsigned long> nv = chordGraph.getInteractors(i);
//                for (unsigned long j= nv.find_first(); j < nv.size(); j = nv.find_next(j) ) {
//                    chordGraph.removeEdge(i, j);
//                }
//            }
//        }

        cout << "# " << helperEdges << " added during triangulation; Originally has " << clusterGraph.numEdges() << " edges" << endl;
    }

    void maximumCardinalitySearch(graph_undirected_bitset & clusterGraph, vector <unsigned long> & sigma ) {
        vector<unsigned long> label(clusterGraph.numNodes(), 0);
        boost::dynamic_bitset<unsigned long> unnumbered(clusterGraph.numNodes());
        for (unsigned long i = 0; i < clusterGraph.numNodes(); ++i) {
            unnumbered[i] = 1;
        }
        for (unsigned long i = 0; i < clusterGraph.numNodes(); ++i) {
            unsigned long x = clusterGraph.numNodes()- 1- i;
            unsigned long V = unnumbered.find_first();
//            unsigned long maxLabel = 0;
            for (unsigned long v = unnumbered.find_first(); v < clusterGraph.numNodes(); v = unnumbered.find_next(v)) {
                if (label[v] > label[V]) {
                    V = v;
                }
            }
//            unsigned long v = distance(label.begin(), max_element(label.begin(), label.end()));
            sigma[V] = x;
//            cout << V << "\t" << x << endl;
            unnumbered[V] = 0;
            boost::dynamic_bitset<unsigned long> nv = clusterGraph.getInteractors(V);
            for (unsigned long w = nv.find_first(); w < nv.size(); w = nv.find_next(w)) {
                ++label[w];
            }
        }
    }

    void chordMaximalCliques(graph_undirected_bitset & clusterGraph, vector<boost::dynamic_bitset<unsigned long> > & newClustersToAdd, vector<string> & nodeIDsToNames) { //not sure if this is valid

        vector<unsigned long> sigma(clusterGraph.numNodes());
        //triangulate clusterGraph; do this in a copy called chordGraph
        graph_undirected_bitset chordGraph(clusterGraph.numNodes());
        cout << "# Start elimination game:" << endl;
        elinminationGame(clusterGraph, chordGraph);
        cout << "# Finish triangulation" << endl;
        maximumCardinalitySearch(chordGraph, sigma); // get elimination order
        cout << "# Finish vertex ordering" << endl;
        vector<unsigned long> size(clusterGraph.numNodes());
        unsigned long v = 0;
        vector<unsigned long> sigma_arg(clusterGraph.numNodes());
        vector<unsigned long> m(clusterGraph.numNodes());

        for (unsigned long i = 0; i < clusterGraph.numNodes(); ++i) {
            sigma_arg[sigma[i]] = i;
        }
//        boost::dynamic_bitset<unsigned long> X(clusterGraph.numNodes());
        for (unsigned long i = 0; i < clusterGraph.numNodes(); ++i) {
            v = sigma_arg[i];
//            cout << v << endl;
            boost::dynamic_bitset<unsigned long> X(clusterGraph.numNodes());
            boost::dynamic_bitset<unsigned long> nv(clusterGraph.numNodes());
            nv = chordGraph.getInteractors(v);

            if (nv.count() == 0) {
                continue;
            }
            for (unsigned long w = nv.find_first(); w < clusterGraph.numNodes(); w = nv.find_next(w)) {
                //to figure out
                if (sigma[w] > sigma[v]) {
                    X[w] = 1;
                }
            }
            if (X.count() > 0) {
//                cout << size[v] << "\t" << X.count() << endl;
                if (size[v] < X.count()) {

                    X[v] = 1;
                    Utils::insertInOrder(newClustersToAdd, X);
//                    printCluster(X, nodeIDsToNames);
//                    cout << endl;
                    X[v] = 0;
                }
//            }
                unsigned long sigmaw = sigma[X.find_first()];
                for (unsigned long w = X.find_first(); w < X.size(); w = X.find_next(w)) {
                    if (sigma[w] < sigmaw) {
                        m[v] = sigma_arg[sigma[w]];
                    }
                }

                if (X.count() - 1 > size[m[v]]) {
                    size[m[v]] = X.count() - 1;
                }
            }
        }
//        clusterGraph = chordGraph;
    }

    // Return true if there is a cluster which contains this one.  Return false otherwise, does it check older cliques?
    bool findClustsWithSeed(boost::dynamic_bitset<unsigned long> seedClust, graph_undirected_bitset & clusterGraph,
                            vector<boost::dynamic_bitset<unsigned long> > & newClustersToAdd, vector<string> & nodeIDsToNames, bool update) {
        for (unsigned i = 0; i < newClustersToAdd.size(); ++i) {
            if (seedClust.is_subset_of(newClustersToAdd[i])) {
                return true;
            }
        }

        // Make a vector containing all nodes which are neighbors of all nodes contained in the seed cluster (called "Both" but really "All")
        vector<unsigned> neighborsOfBoth;
        vector<char> neighborsOfBothBits(clusterGraph.numNodes(), 0); // this should be dynamic_bitset as well. Is char more memory friendly?
        neighborsOfBoth.reserve(clusterGraph.numNodes());
        for (unsigned i = 0; i < clusterGraph.numNodes(); ++i) {
            if ((seedClust[i] == 0) && (seedClust.is_subset_of(clusterGraph.getInteractors(i)))) {
                neighborsOfBoth.push_back(i);
                neighborsOfBothBits[i] = 1;
            }
        }
//        boost::dynamic_bitset<unsigned long> neighborsOfBothBits_p(clusterGraph.numNodes(), 0);
//        boost::dynamic_bitset<unsigned long> neighborsOfBothBits_x(clusterGraph.numNodes());

        if (update && (neighborsOfBoth.size() > 0)) {
//            boost::dynamic_bitset<unsigned long> neighborsOfBothBits_x(clusterGraph.numNodes(), 0);
            updateCliques(clusterGraph, newClustersToAdd, seedClust, neighborsOfBoth, neighborsOfBothBits, 0, nodeIDsToNames); //what is changed?
//            recursion += clique_enum_tomita_apply(clusterGraph, neighborsOfBothBits_p, neighborsOfBothBits_x, seedClust, nodeIDsToNames, newClustersToAdd);
            return true;
        }

        return false;
    }


    void updateClustersWithEdges(vector<pair<pair<unsigned, unsigned>, double> > & edgesToAdd,
                                 currentClusterClassBitset & currentClusters, graph_undirected_bitset & clusterGraph,
                                 nodeDistanceObject & nodeDistances, unsigned & lastCurrent, vector<string> & nodeIDsToNames,
                                 unsigned & largestCluster, bool useChordal) {

        cout << "# Adding " << edgesToAdd.size() << " edges" << endl;
        unsigned long edgesToAddCounter = 0;
        if (edgesToAdd.size() != 0) {
            currentClusters.resetAllUnexplained();
        }

        while (edgesToAddCounter < edgesToAdd.size()) {// is this while loop useful?

            unsigned long thisRoundCounter = edgesToAddCounter;
//            vector<unsigned long> newEdges(edgesToAdd.size(), 0);

            vector<char> affectedNodes(clusterGraph.numNodes(), 0);
            for (; edgesToAddCounter != edgesToAdd.size() ; ++edgesToAddCounter) {
//                clusterGraph.addEdge(edgesToAdd[edgesToAddCounter].first.first, edgesToAdd[edgesToAddCounter].first.second);

                if (!clusterGraph.isEdge(edgesToAdd[edgesToAddCounter].first.first, edgesToAdd[edgesToAddCounter].first.second)) {
                    clusterGraph.addEdge(edgesToAdd[edgesToAddCounter].first.first,
                                         edgesToAdd[edgesToAddCounter].first.second);
                }
                affectedNodes[edgesToAdd[edgesToAddCounter].first.first] = 1;
                affectedNodes[edgesToAdd[edgesToAddCounter].first.second] = 1;
//                    newEdges[edgesToAddCounter] = 1;
//                }
            }

            vector<boost::dynamic_bitset<unsigned long> > newClustersToAdd;
            newClustersToAdd.reserve(100000); //maybe this is problematic? 5000 not handle human genome size. set to 20000?
            bool update = true;

            if (useChordal) {
                chordMaximalCliques(clusterGraph, newClustersToAdd, nodeIDsToNames);
                update = false;

                // need to remove non-maximal cliques from the last round (consider that)
                for (unsigned long i = 0; i < currentClusters.maxClusterID(); ++i) {
                    if (currentClusters.isNew(i) && findClustsWithSeed(currentClusters.getElements(i), clusterGraph, newClustersToAdd,
                                                                       nodeIDsToNames, update)) {
                        currentClusters.deleteCluster(i, nodeIDsToNames, false);
//                        ++deleted;
                    }
                }
            }
            else {
                unsigned numAff = 0;
                for (vector<char>::iterator thisIt = affectedNodes.begin(); thisIt != affectedNodes.end(); ++thisIt) {
                    if (*thisIt) {
                        ++numAff;
                    }
                }
                cout << "# Nodes affected = " << numAff << endl;

                // For each affected node, find all possibly affected clusters
                // Affected means, the cluster probably can grow with new edges added.
                // this is probably greedy?
                vector<char> affectedClusters(currentClusters.maxClusterID(), 0);

                for (unsigned i = 0; i < affectedNodes.size(); ++i) {
                    if (affectedNodes[i]) {
                        for (vector<unsigned long>::iterator cNodesIt = currentClusters.clustersWithNodeBegin(i);
                             cNodesIt != currentClusters.clustersWithNodeEnd(i); ++cNodesIt) {
                            affectedClusters[*cNodesIt] = 1;
                        }
                    }
                }

                numAff = 0;
                for (vector<char>::iterator thisIt = affectedClusters.begin();
                     thisIt != affectedClusters.end(); ++thisIt) {
                    if (*thisIt) {
                        ++numAff;
                    }
                }
                cout << "# Clusters affected = " << numAff << endl;


//            vector<boost::dynamic_bitset<unsigned long> > newClustersToAdd;
//            newClustersToAdd.reserve(100000); //maybe this is problematic? 5000 not handle human genome size. set to 20000?

                unsigned deleted = 0;
                //            cout <<"# "<< currentClusters.maxClusterID() <<"\t"<< affectedClusters.size() <<endl;

                for (unsigned i = 0; i < affectedClusters.size(); ++i) {
                    if (affectedClusters[i] && currentClusters.isNew(i)) {
                        for (unsigned j = 0; j < newClustersToAdd.size(); ++j) {
                            if (findClustsWithSeed(currentClusters.getElements(i), clusterGraph, newClustersToAdd,
                                                   nodeIDsToNames, update)) {
                                currentClusters.deleteCluster(i, nodeIDsToNames, false);
                                ++deleted;
                            }
                        }
                    }
                }

                cout << "# Num of non-maximal clusters deleted here: " << deleted << endl;

                for (; thisRoundCounter < edgesToAddCounter; ++thisRoundCounter) {
                    boost::dynamic_bitset<unsigned long> seedClust(clusterGraph.numNodes());
                    seedClust[edgesToAdd[thisRoundCounter].first.first] = 1;
                    seedClust[edgesToAdd[thisRoundCounter].first.second] = 1;
                    if (!findClustsWithSeed(seedClust, clusterGraph, newClustersToAdd, nodeIDsToNames, update)) {
                        // seedClust is a maximal clique of just two nodes
                        Utils::insertInOrder(newClustersToAdd, seedClust);
                    }
                }
            }

            cout << "# Found " << newClustersToAdd.size() << " new clusters to add" << endl;
//            cout << "Current BK recursion count:" << recursion << endl;

            for (vector<boost::dynamic_bitset<unsigned long> >::iterator clustersToAddIt = newClustersToAdd.begin();
                 clustersToAddIt != newClustersToAdd.end(); ++clustersToAddIt) {
//                cout << "# adding Clusters" << endl;
//                printCluster(*clustersToAddIt, nodeIDsToNames);
//                cout << endl;
                currentClusters.addCluster(*clustersToAddIt,nodeDistances, nodeIDsToNames, clusterGraph, largestCluster);
            }

            if ((currentClusters.numCurrentClusters() > 4*lastCurrent) || ((currentClusters.numCurrentClusters() > lastCurrent) && ((currentClusters.numCurrentClusters() - lastCurrent) > 1000))) {//invalid clusters are notnecessary. Why doing it now? maybe drop some clusters and free up. So if drop necessary, also need to drop from here.
                performValidityCheck(currentClusters, clusterGraph, nodeDistances, nodeIDsToNames);
                lastCurrent = currentClusters.numCurrentClusters();
                if (lastCurrent < 25) {
                    lastCurrent = 25;
                }
            }

            ++edgesToAddCounter; //just for one edge!
        }
        // just run a plain tomita to compare time of recursion
//        boost::dynamic_bitset<unsigned long> p(clusterGraph.numNodes(), 1);
//        boost::dynamic_bitset<unsigned long> r(clusterGraph.numNodes(), 0);
//        boost::dynamic_bitset<unsigned long> x(clusterGraph.numNodes(), 0);
//        vector<boost::dynamic_bitset<unsigned long> > newClustersToAdd_fake;
//        newClustersToAdd_fake.reserve(100000);
//
//        for (unsigned i = 0; i < clusterGraph.numNodes(); ++i) {
//            p[i] = 1;
//        }
//
//        cout << p.count() << endl;
//        recursion += clique_enum_tomita_apply(clusterGraph, p,x,r, nodeIDsToNames,  newClustersToAdd_fake);
//        cout << "Current BK recursion count global:" << recursion << endl;
//        cout << currentClusters.numCurrentClusters()<<endl;
//        cout << newClustersToAdd_fake.size()<< endl;
//        for (unsigned i = 0; i < newClustersToAdd_fake.size(); ++i) {
//            printCluster(newClustersToAdd_fake[i], nodeIDsToNames);
//            cout << endl;
//        }
    }

    bool combineClusters(vector<pair<pair<unsigned long, unsigned long>, double> > & clustersToCombine,
                         currentClusterClassBitset & currentClusters, graph_undirected_bitset & clusterGraph,
                         unsigned & lastCurrent, vector<string> & nodeIDsToNames,
                         nodeDistanceObject & nodeDistances, graph_undirected_bitset & realEdges, unsigned & largestCluster) {

        time_t start, end;
        time (&start);

        vector<vector<char> > edgeWillBeAdded(nodeDistances.numNodes(), vector<char>(nodeDistances.numNodes(), false));
        vector<pair<pair<unsigned, unsigned>, double> > edgesToAdd;
        edgesToAdd.reserve(clusterGraph.numEdges());

        // Sort clustersToCombine in order of descending weight
        sort(clustersToCombine.begin(), clustersToCombine.end(), compClustersToCombine);//why need to sort

        double weight = clustersToCombine.front().second;
        unsigned beginThisWeight = 0;
        for (vector<pair<pair<unsigned long, unsigned long>, double> >::iterator clustersToCombineIt = clustersToCombine.begin();
             clustersToCombineIt != clustersToCombine.end(); ++clustersToCombineIt) {

            if (clustersToCombineIt->second != weight) {
                if (edgesToAdd.size() != beginThisWeight) {
                    sort(edgesToAdd.begin() + beginThisWeight, edgesToAdd.end(), compEdgesToAdd);
                    beginThisWeight = edgesToAdd.size();
                }
                weight = clustersToCombineIt->second;
            }// is this weight useful?
//            printCluster(currentClusters.getElements())
            boost::dynamic_bitset<unsigned long> onlyIn1 = currentClusters.getElements(clustersToCombineIt->first.first);
            boost::dynamic_bitset<unsigned long> onlyIn2 = currentClusters.getElements(clustersToCombineIt->first.second);
//            printCluster(onlyIn1, nodeIDsToNames);
//            cout <<endl;
//            printCluster(onlyIn2, nodeIDsToNames);
//            cout<<endl;
            onlyIn1 -= currentClusters.getElements(clustersToCombineIt->first.second);
            onlyIn2 -= currentClusters.getElements(clustersToCombineIt->first.first);
//
//            //currentClusters.setCurWeight(clustersToCombineIt->second);
//
//            // ADD EDGES BETWEEN GENES THAT ARE ONLY IN ONE OF THE CLUSTERS TO THE CLUSTER GRAPH
            for (unsigned i = onlyIn1.find_first(); i < onlyIn1.size(); i = onlyIn1.find_next(i)) {
                boost::dynamic_bitset<unsigned long> newEdgesFrom1To2 = onlyIn2;
                newEdgesFrom1To2 -= clusterGraph.getInteractors(i); //excluding old edges (THINK ABOUT IT IS clusterGraph or clusterGraph)
                for (unsigned j = newEdgesFrom1To2.find_first(); j < newEdgesFrom1To2.size(); j = newEdgesFrom1To2.find_next(j)) {
                    if (!edgeWillBeAdded[i][j]) { // don't comment out this line
                        edgesToAdd.push_back(make_pair(make_pair(i,j), weight)); //here you are adding all edges!
                        edgeWillBeAdded[i][j] = true;
                        edgeWillBeAdded[j][i] = true;
//                        nodeDistances.setDistance(i,j,weight); //this is the evil?
                    }
                }
            }
        }

        if (edgesToAdd.size() == 0) {
            return false;
        }
        if ( (!useChordal) && (edgesToAdd.size()> 500000)) {
            useChordal = true;
            cout << "# Start the chordal phase: " << endl;
        }
        updateClustersWithEdges(edgesToAdd, currentClusters, clusterGraph, nodeDistances, lastCurrent, nodeIDsToNames, largestCluster, useChordal);

        return true;
    }

    /////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////
    // dagConstruct::addMissingEdges
    // Takes clusters and combines highly overlapping clusters
    /////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////

    bool addMissingEdges(currentClusterClassBitset & currentClusters, graph_undirected_bitset & clusterGraph, double density, double threshold,
                         unsigned & lastCurrent, vector<string> & nodeIDsToNames,
                         nodeDistanceObject & nodeDistances, graph_undirected_bitset & realEdges,  unsigned & largestCluster) { //real edges are not used anywhere
        // clustersToCombine is a pair.  The first element of the pair is itself a pair, which is the ids of the two clusters to combine.
        // The second element of the pair is the weight of the combined edges
        vector<pair<pair<unsigned long, unsigned long>, double> > clustersToCombine;

        unsigned long maxClusterID = currentClusters.maxClusterID();
        vector<bool> clustersChecked(maxClusterID, false);
        vector<unsigned long> clustersToRecheck;
        for (unsigned long i = 0; i < maxClusterID; ++i) {
            if ((currentClusters.numElements(i) != 0) && currentClusters.isNew(i) &&
                (currentClusters.getThresh(i) >= currentClusters.getCurWeight())) {
                clustersChecked[i] = true; // this is only requiring one cluster to have sufficient weight; that makes sense
//                ++clusterbeenchecked;
                for (unsigned long j = 0; j < maxClusterID; ++j) {
                    if ((j != i) && (currentClusters.numElements(j) != 0) && (!clustersChecked[j]) &&
//                        currentClusters.isNew(j) && //whether require both compared cluster to be new? That's a good question
                        (currentClusters.getThresh(j) >= currentClusters.getCurWeight())) {
//                        !clustersChecked[j] && (currentClusters.getThresh(j) >= currentClusters.getCurWeight())) {
                        if (isMinNodeDegreeMet(i, j, currentClusters, realEdges, density, nodeIDsToNames)) {
                            double newEdgeWeight = currentClusters.getClusterWeight(i);
                            if (currentClusters.getClusterWeight(j) < newEdgeWeight) {
                                newEdgeWeight = currentClusters.getClusterWeight(j);//ewww
                            }
//                            cout << "# Test newEdgeWeight in addMissingEdges 1 "<< newEdgeWeight << endl;
                            clustersToCombine.push_back(make_pair(make_pair(i,j), newEdgeWeight));
                            Utils::insertInOrder(clustersToRecheck, j);//why need recheck? seems odd. why onnly recheck j? shouldn't we recheck the combined thing?
                        }
                    }
                }
            }
        }
        //now there is no point to recheck; since no alteration on clusterWeight; new clusters have not been added here; merging of new cluster will be considered in the outer cycle

//        cout << "# complete initial combine clusters " << clusterbeenchecked << endl;

//        while (clustersToRecheck.size()) { // this while loop is not problematic (recheck happens seems to due to the weight change)
////            cout << clustersToRecheck.size() << endl;
//            vector<unsigned long> clustersToRecheckAgain;
//            for (vector<unsigned long>::iterator it = clustersToRecheck.begin(); it != clustersToRecheck.end(); ++it) {
//                clustersChecked[*it] = true;
////                printCluster(currentClusters.getElements(*it), nodeIDsToNames);
//                for (unsigned long j = 0; j < maxClusterID; ++j) {
//                    if ((*it != j) && (currentClusters.numElements(j) != 0) &&
//                        !clustersChecked[j]) {
//                        if (isMinNodeDegreeMet(*it,j,currentClusters,clusterGraph,density, nodeIDsToNames)) {
//                            double newEdgeWeight = currentClusters.getClusterWeight(*it);
//                            if (currentClusters.getClusterWeight(j) < newEdgeWeight) {
//                                newEdgeWeight = currentClusters.getClusterWeight(j);
//                            }
//                            clustersToCombine.push_back(make_pair(make_pair(*it,j), newEdgeWeight));
//                            Utils::insertInOrder(clustersToRecheckAgain, j); //only successful ones need to be checked again; cluster can be repeatedly checked
//                        }
//                    }
//                }
//            }
//            clustersToRecheck = clustersToRecheckAgain;
//        }

        cout << "# Combining " << clustersToCombine.size() << " pairs of clusters" << endl;

        if (clustersToCombine.size() > 0) {
            double curWeight = currentClusters.getCurWeight();
//            cout << "# Current Weight is " << curWeight << endl;
            bool realCombine = combineClusters(clustersToCombine, currentClusters, clusterGraph, lastCurrent, nodeIDsToNames, nodeDistances, realEdges, largestCluster);
            currentClusters.setCurWeight(curWeight);
            return realCombine;
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    /// Uses a bitset representation of cluster graph                    ///
    /// and clusters to get maximal cliques                              ///
    /// Uses a threshold to deal with imperfect case                     ///
    /// Returns number of top level nodes                                ///
    ////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    unsigned getFuzzyClustersBitsetThreshold(nodeDistanceObject & nodeDistances, map<string,unsigned> & nodeNamesToIDs,
                                             vector<validClusterBitset> & validClusters, double threshold = 0, double density = 1) {
        time_t start, end;
        time (&start);
        double dif;
        unsigned largestCluster = 0;
        unsigned lastLargestCluster = 0;
        double largestClusterWeight = 0;
        double lastLargestClusterWeight = 0;
        unsigned lastCurrent = 10;
        unsigned numRealEdgesAdded = 0;
        unsigned numRealEdgesThisRound = 0;
        unsigned numRealEdgesLastRound = 0;
        unsigned maxNumUniqueUnexplainedEdges = 0;
        unsigned clusterGraphlastRoundEdges = 0;

//        recursion = 0;
        globalDensity = density;

        vector<string> nodeIDsToNames(nodeNamesToIDs.size(), string(""));
        for (map<string,unsigned>::iterator it = nodeNamesToIDs.begin(); it != nodeNamesToIDs.end(); ++it) {
            nodeIDsToNames[it->second] = it->first;
        }
        unsigned numNodes = nodeDistances.numNodes();
        graph_undirected_bitset clusterGraph(numNodes);
        graph_undirected_bitset realEdges(numNodes);
//        graph_undirected_bitset inferredEdges(numNodes);
        double dt = nodeDistances.sortedDistancesBegin()->second; // Current threshold distanceG
        currentClusterClassBitset currentClusters(numNodes, clusterGraph.get_num_blocks(),dt, threshold);
//        bool addAtEnd = false;
        sortedDistanceStruct::iterator distanceIt = nodeDistances.sortedDistancesBegin(); //this may be problematic since
        unsigned totalEdges = numNodes*(numNodes-1) / 2;

        while ((clusterGraph.numEdges() != totalEdges) && (distanceIt != nodeDistances.sortedDistancesEnd()) && (distanceIt->second >= threshold)) {
//            clusterGraph = realEdges; //reset clusterGraph
//            unsigned numRealEdgesThisRound = 0;
            vector<pair<pair<unsigned, unsigned>, double> > edgesToAdd;
            edgesToAdd.reserve(10000000); //Fan: whether this will be slow if exceeded?

            double addUntil = currentClusters.getNextThresh(); //Fan's understanding: if current clusters have lots of inferred edges (maybe determining threshold), does not go down so much as alpha //should be the lowest edge of this round minus alpha
            if (addUntil > dt) {
                addUntil = dt;
            }
            if ((dt - threshold) > addUntil) { // this is to prevent the lowest edge from this round going to low, but without inferred edges, maybe unnecessary
                addUntil = dt - threshold;
            }
            cout << "# Current distance: " << distanceIt->second << "\t" << "Add until: " << addUntil << "\t" << endl;
            while ((distanceIt != nodeDistances.sortedDistancesEnd()) && (distanceIt->second >= addUntil) && (distanceIt->second >= threshold)) {
                unsigned firstNode = distanceIt->first.first;
                unsigned secondNode = distanceIt->first.second;
                realEdges.addEdge(firstNode, secondNode); //realEdges only changed here
                ++numRealEdgesAdded;
                ++numRealEdgesThisRound;
                edgesToAdd.push_back(make_pair(make_pair(firstNode, secondNode), distanceIt->second));
                ++distanceIt;
            }
            cout << "# Num of real edges added: " << numRealEdgesAdded << endl;

            time (&end);
            dif = difftime(end,start);
            cout << "# Time elapsed: " << dif << " seconds" << endl;

            if ( (!useChordal) && ( (edgesToAdd.size()> 200000) || (numRealEdgesAdded > 500000))) {
                useChordal = true;
                cout << "# Start the chordal phase: " << endl;
            }
            updateClustersWithEdges(edgesToAdd, currentClusters, clusterGraph, nodeDistances, lastCurrent, nodeIDsToNames, largestCluster, useChordal);//Fani change distanceIt->second to addUntil maybe?

            time (&end);
            dif = difftime(end,start);
            cout << "# Time elapsed: " << dif << " seconds" << endl;

            double last_dt = dt;
//            if (useChordal) {
//                dt = dt - threshold;
//            }
            if (distanceIt != nodeDistances.sortedDistancesEnd()) {
                dt = distanceIt->second; //dt is already moved to the next level
            } else {
                dt = 0;
            }
            currentClusters.setCurWeight(dt);
//            if ( (!useChordal) && (numRealEdgesAdded > 0.1 * totalEdges) ) {
//            if ( (!useChordal) && (numRealEdgesAdded > 0.05*totalEdges) && (dif > 10000)) {
//                useChordal = true;
//            }

            double maxThresh = currentClusters.getMaxThresh(); // this seems to be redundant, who not check clusters one by one anyway
            cout << "# MaxThresh this round: " << maxThresh << endl;
            if ((maxThresh >= dt) && (numRealEdgesThisRound > numRealEdgesLastRound) && (clusterGraphlastRoundEdges  < totalEdges - clusterGraph.numEdges())) {// this may need to change
                unsigned long numClustersBeforeDelete = currentClusters.numCurrentClusters();

                vector<unsigned long> newClustersSorted;
                if ((density < 1) && (!useChordal)) {
                    performValidityCheck(currentClusters, clusterGraph, nodeDistances, nodeIDsToNames); // drop unnecessary clusters. It seems this always happen before large cluster operations // this about how to change this (clusterGraph actually not used here, so no worries

                    // IN HERE WE NEED TO CHECK FOR MISSING EDGES BY COMBINING CLUSTERS INTO DENSE CLUSTERS
                    cout << "# Adding missing edges...checking " << currentClusters.numCurrentClusters() << " cliques" << endl;
                    bool newEdgesAdded = addMissingEdges(currentClusters, clusterGraph, density, threshold, lastCurrent, nodeIDsToNames, nodeDistances, realEdges, largestCluster);

                    time (&end);
                    dif = difftime(end,start);
                    cout << "# Time elapsed: " << dif << " seconds" << endl;

                    while (newEdgesAdded == true) { //not sure
                        performValidityCheck(currentClusters, clusterGraph, nodeDistances, nodeIDsToNames);
                        newEdgesAdded = addMissingEdges(currentClusters, clusterGraph, density, threshold, lastCurrent, nodeIDsToNames, nodeDistances, realEdges, largestCluster);
                    }// clusters are iteratively merged here before being output, so resetting nodeDistance has the snowball effect

                    currentClusters.sortNewClusters(newClustersSorted);
                } else {
                    currentClusters.prepareForValidityCheck(newClustersSorted, clusterGraph);
//                    currentClusters.sortNewClusters(newClustersSorted);
                }

                time (&end);
                dif = difftime(end,start);
                cout << "# Time elapsed: " << dif << " seconds" << endl;

                vector<char> idsChecked(currentClusters.maxClusterID(), 0);
                cout << "# Current number of clusters:" << currentClusters.maxClusterID() << endl;
//                unsigned maxNumUniqueUnexplainedEdges = 0;
                cout << "# New clusters to evaluate:" << newClustersSorted.size() << endl;
                for (vector<unsigned long>::iterator newClusterIt = newClustersSorted.begin(); //how to sort? sorted by size of clusteri, small terms checked first
                     newClusterIt != newClustersSorted.end(); ++newClusterIt) {
                    bool isNecessary = false;
                    bool checkForFinal = false;
                    bool latesmall = false;
                    double clustWeight = currentClusters.getClusterWeight(*newClusterIt);


                    // define late small
                    double latesmallThres = min( (log(numNodes) - log(currentClusters.getElements(*newClusterIt).count())) / (log(numNodes)-log(2)), lastLargestClusterWeight * (log(numNodes) -log(currentClusters.getElements(*newClusterIt).count())) / (log(numNodes)-log(lastLargestCluster)) );
//                    cout << latesmallThres << endl;
                    if (currentClusters.getThresh(*newClusterIt) < latesmallThres -0.2 ) {
                        latesmall = true;
                    }

                    if (currentClusters.isNew(*newClusterIt) && (currentClusters.getThresh(*newClusterIt) >= dt *(1- double(useChordal) ))) { // should be compared with dt (updated) right? // is large and equal here. Why still no large terms?

                        if (!latesmall) {
                            checkForFinal = true; // if not checked for final, keep it around
                            currentClusters.setCheckedFinal(*newClusterIt);
                        }
                        else {
                            currentClusters.deleteCluster(*newClusterIt, nodeIDsToNames, false);
                            continue;
                        }
                    } // this piece of code is a little redundant
//                    else if (useChordal){
//                        cout << "# Cluster weight not qualified" << endl;
//                    }

                    if (currentClusters.checkClusterFinalValidity(*newClusterIt,isNecessary, idsChecked, checkForFinal)) { // think about the condition here
                        //* some big changes here *//
                        currentClusters.setNumUniquelyUnexplainedEdges(*newClusterIt); // this should consider the inactive clusters
                        unsigned numUniqueUnexplainedEdges = currentClusters.getNumUniquelyUnexplainedEdges(*newClusterIt);
                        if ((numUniqueUnexplainedEdges < maxNumUniqueUnexplainedEdges) && (numUniqueUnexplainedEdges < (1-density) * currentClusters.getElements(*newClusterIt).count() )) {//my secret sauce
                            currentClusters.deleteCluster(*newClusterIt, nodeIDsToNames, false);
//                            if (useChordal) {
//                                cout << "# UnexplainedEdges not qualified" << endl;
//                            }
                            continue;
                        }
                        else if (useChordal && (currentClusters.getElements(*newClusterIt).count() < lastLargestCluster/2.0)) {
                            currentClusters.deleteCluster(*newClusterIt, nodeIDsToNames, false);
                            continue;
                        }
                        else if (numUniqueUnexplainedEdges > maxNumUniqueUnexplainedEdges) {
                            maxNumUniqueUnexplainedEdges = numUniqueUnexplainedEdges;
                        }

                        validClusters.push_back(
                                validClusterBitset(currentClusters.getElements(*newClusterIt), 0, clustWeight));
                        cout << "# Valid cluster:\t";
                        printCluster(currentClusters.getElements(*newClusterIt), nodeIDsToNames);
                        //currentClusters.setClusterValid(currentClusters.getElements(*newClusterIt));
                        currentClusters.setClusterValid(*newClusterIt, clusterGraph);
                        cout << "\t" << clustWeight << "\t"
                             << currentClusters.getNumUniquelyUnexplainedEdges(*newClusterIt) << "\t"
                             << currentClusters.getThresh(*newClusterIt) << "\t" << dt << endl;
                        if (validClusters.back().numElements() > largestCluster) {
                            largestCluster = validClusters.back().numElements();
                            largestClusterWeight = currentClusters.getThresh(*newClusterIt) +threshold;
                        }
                        currentClusters.setOld(*newClusterIt);//important


                    } else if (!isNecessary) {
//                        if (useChordal) {
//                            cout << "# IsNecessary not qualified" << endl;
//                        }
                        currentClusters.deleteCluster(*newClusterIt,nodeIDsToNames,false);

                    } else { // && currentClusters.isNew(*newClusterIt) && !currentClusters.wasNecessary(*newClusterIt) && checkForFinal) {
                        currentClusters.setNecessary(*newClusterIt); //what's the effect of this line, seem to have no effect
                    }
                }

                lastCurrent = currentClusters.numCurrentClusters();
                if (lastCurrent < 25) {
                    lastCurrent = 25;
                }
                lastLargestCluster = largestCluster;
                lastLargestClusterWeight = largestClusterWeight;

                cout << "# dt: " << last_dt << endl;
                cout << "# Next dt: " << dt << endl;
                cout << "# Num current clusters before delete: " << numClustersBeforeDelete << endl;
                cout << "# Num current clusters: " << currentClusters.numCurrentClusters() << endl;
                cout << "# Num valid clusters: " << validClusters.size() << endl;
                cout << "# Largest cluster: " << largestCluster << endl;
                cout << "# Num edges in clusterGraph: " << clusterGraph.numEdges() << endl;
                cout << "# Num real edges: " << numRealEdgesAdded << endl;
//                cout << "# Num edges inferred: " << clusterGraph.numEdges() - clusterGraph.numEdges() << endl;
                time (&end);
                dif = difftime(end,start);
                cout << "# Time elapsed: " << dif << " seconds" << endl;

                numRealEdgesLastRound = numRealEdgesThisRound;
                numRealEdgesThisRound = 0;
                clusterGraphlastRoundEdges = clusterGraph.numEdges();
            }
        } //Fan: while loop ends here


        // Add clusters at end to valid clusters
//        if (addAtEnd) {
//            clusterGraph = clusterGraph;
//            cout << "# Adding at end" << endl;
//            double last_dt = dt;
//            dt = 0;
//            currentClusters.setCurWeight(dt);
//            vector<unsigned long> newClustersSorted;
//
//            if (density < 1) {
//                performValidityCheck(currentClusters, clusterGraph, nodeDistances, nodeIDsToNames);
//
//                // IN HERE WE NEED TO CHECK FOR MISSING EDGES BY COMBINING CLUSTERS INTO DENSE CLUSTERS
//                cout << "# Adding missing edges...checking " << currentClusters.numCurrentClusters() << " clusters" << endl;
//                bool newEdgesAdded = addMissingEdges(currentClusters, clusterGraph, density, threshold, lastCurrent, nodeIDsToNames, clusterGraph, nodeDistances);
//                while (newEdgesAdded == true) {
//                    performValidityCheck(currentClusters, clusterGraph, nodeDistances, nodeIDsToNames);
//                    newEdgesAdded = addMissingEdges(currentClusters, clusterGraph, density, threshold, lastCurrent, nodeIDsToNames, clusterGraph, nodeDistances);
//                }
//                currentClusters.sortNewClusters(newClustersSorted);
//            } else {
//                currentClusters.prepareForValidityCheck(newClustersSorted, clusterGraph);
//            }
//
//            vector<char> idsChecked(currentClusters.maxClusterID(), 0);
//            for (vector<unsigned long>::iterator newClusterIt = newClustersSorted.begin();
//                 newClusterIt != newClustersSorted.end(); ++newClusterIt) {
//                bool isNecessary = false;
//                bool checkForFinal = false;
//                double clustWeight = currentClusters.getClusterWeight(*newClusterIt);
//
//                if (currentClusters.isNew(*newClusterIt) && (currentClusters.getThresh(*newClusterIt) >= 0)) { // should be compared with dt (updated) right?
//                    checkForFinal = true; // if not checked for final, keep it around
//                    currentClusters.setCheckedFinal(*newClusterIt);
//                }
//
//                if ( checkForFinal && currentClusters.checkClusterFinalValidity(*newClusterIt,isNecessary, idsChecked, checkForFinal)) {
//
//                    validClusters.push_back(validClusterBitset(currentClusters.getElements(*newClusterIt), 0, clustWeight));
//                    cout << "# Valid cluster:\t";
//                    printCluster(currentClusters.getElements(*newClusterIt), nodeIDsToNames);
//                    //currentClusters.setClusterValid(currentClusters.getElements(*newClusterIt));
//                    currentClusters.setClusterValid(*newClusterIt, clusterGraph);
//                    cout << "\t" << clustWeight << "\t" << currentClusters.getNumUniquelyUnexplainedEdges(*newClusterIt) << "\t" << currentClusters.getThresh(*newClusterIt) << "\t" << dt << endl;
//                    if (validClusters.back().numElements() > largestCluster) {
//                        largestCluster = validClusters.back().numElements();
//                    }
//                    currentClusters.setOld(*newClusterIt);
//
//                } else  {
//                    currentClusters.deleteCluster(*newClusterIt, nodeIDsToNames, false);
//                }
//            }

//            cout << "# dt: " << last_dt << endl;
//            cout << "# Num current clusters: " << currentClusters.numCurrentClusters() << endl;
//            cout << "# Num valid clusters: " << validClusters.size() << endl;
//            cout << "# Largest cluster: " << largestCluster << endl;
//            cout << "# Num edges in clusterGraph: " << clusterGraph.numEdges() << endl;
//        }
        return currentClusters.numCurrentClusters();
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////
    // Take list of valid clusters and turn it into an ontology, assuming the perfect case
    // where all edges in a cluster are identical
    //////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////

    void clustersToDAG(vector<validClusterBitset> validClusters, DAGraph & ontology, unsigned numTerminalNodes) {
        //unsigned numTerminalNodes = nodeDistances.numNodes();

        sort(validClusters.begin(), validClusters.end());

        ontology.reserveNodes(ontology.numNodes() + validClusters.size() + 1);
        unsigned clustCount = 0;
        vector<validClusterBitset>::iterator clustersIt = validClusters.begin();
        unsigned firstNodeID = ontology.addNode();
        clustersIt->setID(firstNodeID);
        for (unsigned i = 0; i < numTerminalNodes; ++i) {
            if (clustersIt->isElement(i)) {
                ontology.addEdge(firstNodeID, i);
            }
        }
        ontology.setWeight(clustersIt->getWeight(),firstNodeID);
        double geneWeight = clustersIt->getWeight();
        ++clustersIt;
        ++clustCount;
        for ( ; clustersIt != validClusters.end(); ++clustersIt) {
            //cout << clustCount << endl;
            unsigned newNodeID = ontology.addNode();
            clustersIt->setID(newNodeID);

            boost::dynamic_bitset<unsigned long> unaccountedFor = clustersIt->getElements();

            for (vector<validClusterBitset>::reverse_iterator possibleDescendentsIt(clustersIt);
                 possibleDescendentsIt != validClusters.rend(); ++possibleDescendentsIt) {
                if (possibleDescendentsIt->numElements() < clustersIt->numElements()) {
                    // A cluster is a child if it is not already a
                    // descendent of the current cluster via some other cluster,
                    // and all of its genes are contained in the possible ancestor cluster,

                    bool isAlreadyDescendent = ontology.isDescendent(possibleDescendentsIt->getID(), newNodeID);
                    if ((!isAlreadyDescendent) &&
                        (isClusterAncestor(possibleDescendentsIt->getElements(), clustersIt->getElements(), unaccountedFor))) {
                        //cout << "adding edge " << newNodeID << "\t" << possibleDescendentsIt->getID() << endl;
                        ontology.addEdge(newNodeID, possibleDescendentsIt->getID());
                    }
                }
            }

            // Add edges to genes which are not contained in any of the child nodes
            for (unsigned i = 0; i < numTerminalNodes; ++i) {
                if (unaccountedFor[i] == 1) {
                    ontology.addEdge(newNodeID, i);
                }
            }

            // Set weight of cluster
            ontology.setWeight(clustersIt->getWeight(),newNodeID);
            if (geneWeight < clustersIt->getWeight()) {
                geneWeight = clustersIt->getWeight();
            }
            //cout << ontology.getName(newNodeID) << "\t" << ontology.getWeight(newNodeID) << endl;
            ++clustCount;
        }

        if (geneWeight > 1) {
            ontology.setGeneWeight(geneWeight);
        } else {
            ontology.setGeneWeight(1);
        }

        //cout << "Finished creating DAG - adding root node" << endl;
        unsigned firstTopLevelNode;
        bool firstTopLevelFound = false;
        bool secondTopLevelFound = false;
        long rootID = -1;

        unsigned numTopLevel = 0;
        for (vector<DAGNode>::iterator nodeIt = ontology.nodesBegin(); nodeIt != ontology.nodesEnd(); ++nodeIt) {
            if ((nodeIt->numParents() == 0) && (nodeIt->getID() != rootID)) {
                //cout << nodeIt->getID() << " " << nodeIt->getName() << " is top level" << endl;
                ++numTopLevel;
                //cout << numTopLevel << endl;
                if (firstTopLevelFound == false) {
                    firstTopLevelFound = true;
                    firstTopLevelNode = nodeIt->getID();
                } else if (secondTopLevelFound == false) {
                    secondTopLevelFound = true;
                    unsigned curItPos = nodeIt->getID();

                    rootID = ontology.addNode();
                    // Adding node may invalidate nodeIt, so fix it
                    nodeIt = ontology.nodesBegin();
                    nodeIt += curItPos;

                    ontology.setWeight(0, rootID);
                    ontology.addEdge(rootID, firstTopLevelNode);
                    ontology.addEdge(rootID, nodeIt->getID());
                } else {
                    if (rootID != nodeIt->getID()) {
                        ontology.addEdge(rootID, nodeIt->getID());
                    }
                }
            }
        }
        return;
    }


    /////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////
    // dagConstruct::constructDAG(...)
    // Main clustering function - cluster input graph into Ontology
    /////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////
    void constructDAG(graph_undirected & input_graph, DAGraph & ontology, nodeDistanceObject & nodeDistances, double threshold, double density) {
        //ontology = DAGraph();
        map<string,unsigned> geneNamesToIDs;

        // Add all nodes in the input network to the ontology as genes
        for (vector<Node>::iterator nodesIt = input_graph.nodesBegin(); nodesIt != input_graph.nodesEnd(); ++nodesIt) {
            geneNamesToIDs[nodesIt->getName()] = nodesIt->getID();
            ontology.addNode(nodesIt->getName(), geneNamesToIDs);
        }

        nodeDistances = nodeDistanceObject(input_graph);

        vector<validClusterBitset> validClusters;
        dagConstruct::getFuzzyClustersBitsetThreshold(nodeDistances, geneNamesToIDs, validClusters, threshold, density); //Fan: all important stuff in here

        // Got the clusters - build ontology assuming any clusters wholly contained in others are descendents
        cout << "# Num clusters: " << validClusters.size() << endl;
        dagConstruct::clustersToDAG(validClusters, ontology, geneNamesToIDs.size());
        return;
    }
}
#endif //DAG_CONSTRUCT
