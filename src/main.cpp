#include <iostream>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <set>

// for convenience
using json = nlohmann::json;
using namespace std;

typedef pair<int, int> I2I;

struct Node {
    string node_public_key;
    string ip_address;
    // string version;
//   int uptime;
    int inbound_count;
    int outbound_count;
//   string rowkey;
//   string country;
//   string country_code;
//   string latitude;
//   string longitude;
//   string timezone;

    // customize field
    set<int> inCliques;
};

struct Link {
    string source;
    string target;
};

struct NetworkInfo {
    vector<Node> nodes;
    vector<Link> links;
    map<string, int> nodeMap; // id of pubkey
    vector<set<int>> cliques;
    unordered_map<int, int> cliquesSizeNumber;
    vector<I2I> nodesHasCliquesVec;
};

struct ASNInfo {
    string asn;
    set<int> nodes;
};

void printSet(set<int> &s) {
    cout << "{";
    for(auto &e: s) {
        cout << " " << e;
    }
    cout << " }\n";
}

void from_json(const nlohmann::json& j, Node& n) {
    try {
        j.at("node_public_key").get_to(n.node_public_key);
        j.at("ip").get_to(n.ip_address);
        j.at("inbound_count").get_to(n.inbound_count);
        j.at("outbound_count").get_to(n.outbound_count);
    } catch (json::exception& e) {
        cout << "message: " << e.what() << '\n' << "exception id: " << e.id << endl;
    }
}

void from_json(const nlohmann::json& j, Link& l) {
    j.at("source").get_to(l.source);
    j.at("target").get_to(l.target);
}

set<int> neighbor(int v, vector<vector<bool>> &adj) {
    set<int> result;
    for(int i = 0; i < int(adj.size()); i++) {
        if (adj[v][i]) {
            result.insert(i);
        }
    }
    return result;
}

set<int> intersection(set<int> set_a, set<int> set_b) {
    set<int> result;
    set_intersection(set_a.begin(), set_a.end(), set_b.begin(), set_b.end(), inserter(result, result.begin()));
    return result;
}

void BronKerbosch(set<int> R, set<int> P, set<int> X, vector<vector<bool>> &adj, NetworkInfo& ninfo) {
    if (P.size() == 0 && X.size() == 0) {
        // cout << "set: { ";
        // for(auto& e : R) {
        //     cout << e << " ";
        // }
        // cout << "}" << endl;
        ninfo.cliques.push_back(R); // make each cliques a unique id
        for (auto& e : R) { // make each node can track its belonged cliques
            ninfo.nodes[e].inCliques.insert(ninfo.cliques.size()-1);
        }
        // statistics the cliques number
        if (ninfo.cliquesSizeNumber.find(R.size()) == ninfo.cliquesSizeNumber.end()) {
            ninfo.cliquesSizeNumber[R.size()] = 1;
        } else {
            ninfo.cliquesSizeNumber[R.size()]++;
        }
        return;
    }

    int pivot = 0;
    for(pivot = 0; pivot < int(adj.size()); pivot++) {
        if(P.count(pivot) || X.count(pivot)) {
            // cout << "pivot: " << pivot << endl;
            break;
        }
    }

    for(int i = 0; i < int(adj.size()); i++) {
        if(P.count(i) && !adj[pivot][i]) {
            R.insert(i);
            BronKerbosch(R, intersection(P, neighbor(i, adj)), intersection(X, neighbor(i, adj)), adj, ninfo);
            R.erase(i);
            P.erase(i);
            X.insert(i);
        }
    }
}

bool isSymmetric(vector<vector<bool>> &matrix) {
    for (int idx_row = 0; idx_row < int(matrix.size()); idx_row++) {
        for (int idx_col = idx_row; idx_col < int(matrix.size()); idx_col++) {
            if (matrix[idx_row][idx_col] != matrix[idx_col][idx_row]) {
                return false;
            }
        }
    }
    return true;
}

void readRippleFromFile(
    vector<Node> &nodes,
    vector<Link> &links
) {
    ifstream file;
    json rippleJson;
    file.exceptions (ifstream::failbit | ifstream::badbit );

    try {
        file.open("../doc/Ripple.json");
        file >> rippleJson;
        file.close();
    } catch (fstream::failure &e) {
        cerr << "Exception opening/reading/closing file\n";
    }
    nodes = rippleJson.at("nodes").get<vector<Node>>();
    links = rippleJson.at("links").get<vector<Link>>();
    cout << "nodes number: " << nodes.size() << endl;
    cout << "links number: " << links.size() << endl;
}

void mappingPubkeyToId(
    vector<Node> &nodes,
    map<string, int> &nodeMap
) {
    for (int idx = 0; idx < int(nodes.size()); idx++) {
        pair<map<string, int>::iterator, bool> res;
        res = nodeMap.insert(pair<string, int>(nodes[idx].node_public_key, idx));
        if (res.second == false) {
            cerr << "pubkey: " << nodes[idx].node_public_key << " has already existed";
            exit(1);
        }
    }
}

void adjMatrixCreate(
    vector<Link> &links,
    map<string, int> &nodeMap,
    vector<vector<bool>> &adj
) {
    for (int idx = 0; idx < int(links.size()); idx++) {
        int srcNodeIdx, trgNodeIdx;
        if (nodeMap.find(links[idx].source) != nodeMap.end() && nodeMap.find(links[idx].target) != nodeMap.end()) {
            srcNodeIdx = nodeMap[links[idx].source];
            trgNodeIdx = nodeMap[links[idx].target];
        } else {
            cout << "the nodes can't be found in the link\n";
            cout << "links " << idx << ": " << links[idx].source << " and " << links[idx].target << endl;
            exit(1);
        }
        adj[srcNodeIdx][trgNodeIdx] = true;
        // make the adj matrix is symmetric.
        adj[trgNodeIdx][srcNodeIdx] = true;
    }
    // if (!isSymmetric(adj)) {
    //     printf("the matrix is not symmetric");
    //     exit(1);
    // }
}

// a compare operation for pair<int, int>
struct CmpByValue {
  bool operator()(const I2I& lhs, const I2I& rhs) {
    return lhs.second > rhs.second;
  }
};

// generate a pair of <node-id, number of belonged cliques>
// find out a list of influential nodes
void sortInfluentialNode(
    NetworkInfo& ninfo
) {
    map<int, int> nodeHasCliques;
    for (int idx = 0; idx < int(ninfo.nodes.size()); idx++) {
        nodeHasCliques[idx] = ninfo.nodes[idx].inCliques.size();
    }
    copy(nodeHasCliques.begin(), nodeHasCliques.end(), back_inserter(ninfo.nodesHasCliquesVec));
    sort(ninfo.nodesHasCliquesVec.begin(), ninfo.nodesHasCliquesVec.end(), CmpByValue());
    // for (int idx = 0; idx < 20; idx++) {
    //     cout << ninfo.nodesHasCliquesVec[idx].first << " belong to " << ninfo.nodesHasCliquesVec[idx].second << " cliques" << endl;
    // }
}

void outputNodeInfo(
    NetworkInfo& ninfo
) {
    ofstream csvfile;
    csvfile.exceptions (ofstream::failbit | ofstream::badbit );
    try {
        csvfile.open("../doc/nodes.csv");
    } catch (fstream::failure &e) {
        cerr << "Exception opening/reading/closing file\n";
    }
    csvfile << "id,ip_address,hasCliques\n";

    for(int idx = 0; idx < int(ninfo.nodesHasCliquesVec.size()); idx++) {
        int node_id = ninfo.nodesHasCliquesVec[idx].first;
        string ip_address = (ninfo.nodes[node_id].ip_address.substr(0, 7) == "::ffff:") ? 
            ninfo.nodes[node_id].ip_address.substr(7) : ninfo.nodes[node_id].ip_address;
        csvfile << node_id << "," << ip_address << "," << ninfo.nodesHasCliquesVec[idx].second << "\n";
    }
    csvfile.close();
}

void checkFork(
    NetworkInfo& ninfo
) {
    ofstream csvfile;
    csvfile.exceptions (ofstream::failbit | ofstream::badbit );

    try {
        csvfile.open("../doc/fork.csv");
    } catch (fstream::failure &e) {
        cerr << "Exception opening/reading/closing file\n";
    }
    int potentialFork = 0;
    vector<bool> removeNodes(ninfo.nodes.size(), false);
    for(int idx = 0; idx < int(ninfo.nodesHasCliquesVec.size()); idx++) {
        int node_id = ninfo.nodesHasCliquesVec[idx].first;
        removeNodes[node_id] = true;
        for(auto iter1 = ninfo.nodes[node_id].inCliques.begin(); iter1 != ninfo.nodes[node_id].inCliques.end(); iter1++) {
            for(auto iter2 = next(iter1, 1); iter2 != ninfo.nodes[node_id].inCliques.end(); iter2++) {
                set<int> c1 = ninfo.cliques[*iter1];
                set<int> c2 = ninfo.cliques[*iter2];
                int largerSize = (c1.size() > c2.size()) ? c1.size() : c2.size();
                set<int> intersectionSet = intersection(c1, c2);
                for (auto iterInterSection = intersectionSet.begin(); iterInterSection != intersectionSet.end(); iterInterSection++) {
                    if(removeNodes[*iterInterSection]) {
                        intersectionSet.erase(iterInterSection);
                    }
                }
                if(int(intersectionSet.size()) < largerSize / 5) {
                    potentialFork++;
                    // cout << "node: " << node_id << endl;
                    // printSet(c1);
                    // printSet(c2);
                    // exit(1);
                }
            }
        }
        cout << node_id << ", " << potentialFork << endl;
        csvfile << potentialFork << "\n";
        if (idx == 100) {
            csvfile.close();
            exit(1);
        }
    }
    csvfile.close();
}

void updateNodesInfluentialSequenceBy(
    vector<ASNInfo>& asninfo
) {
    ifstream csvfile;
    csvfile.exceptions (ifstream::failbit | ifstream::badbit );
    try {
        csvfile.open("../doc/sortedNewNodes.csv");
    } catch (fstream::failure &e) {
        cerr << "Exception opening/reading/closing file\n";
    }

    string line;
    getline( csvfile, line);
    try {
        while (getline( csvfile, line)) {
            istringstream templine(line);
            string id_str, ip, cliques, asn;
            getline( templine, id_str, ',');
            getline( templine, ip, ',');
            getline( templine, cliques, ',');
            getline( templine, asn);
            int id = stoi(id_str);
            // Because there is a CR(13) in the end of line
            // Debug very long time...
            asn = asn.substr(0, asn.length()-1);
            ASNInfo tmpASNInfo;
            tmpASNInfo.asn = "anonymous";
            set<int> tmpSet = {id};
            tmpASNInfo.nodes = tmpSet;

            if (asn.substr() == "anonymous") {
                asninfo.push_back(tmpASNInfo);
                continue;
            } 
            if (asninfo.size() > 0 && asninfo.back().asn == asn) {
                asninfo.back().nodes.insert(id);
            } else {
                tmpASNInfo.asn = asn;
                asninfo.push_back(tmpASNInfo);
            }
        }
    } catch (fstream::failure &e) {
        if ( csvfile.eof() ) {
            ;
        } else {
            cerr << e.what() << endl;
        }
    }
    csvfile.close();
    for (int i =0; i <10; i++) {
        cout << asninfo[i].asn << ", "<< asninfo[i].nodes.size() << endl;
    }
}

int main(int argc, char *argv[]){
    NetworkInfo ninfo;

    readRippleFromFile(ninfo.nodes, ninfo.links);
    mappingPubkeyToId(ninfo.nodes, ninfo.nodeMap);

    // adjacency matrix
    vector<vector<bool>> adj(ninfo.nodes.size(), vector<bool>(ninfo.nodes.size(), false));
    adjMatrixCreate(ninfo.links, ninfo.nodeMap, adj);

    // Run Maximal Cliqiues Algorithm
    set<int> R, P, X;
    for (int idx = 0; idx < int(ninfo.nodes.size()); idx++) {
        P.insert(idx);
    }
    BronKerbosch(R, P, X, adj, ninfo);
    for (auto iter = ninfo.cliquesSizeNumber.begin(); iter != ninfo.cliquesSizeNumber.end(); ++iter) {
        cout << "There are " << iter->second << " " << iter->first << "-nodes cliques.\n";
    }
    cout << "cliques number: ";
    cout << ninfo.cliques.size() << endl;

    // sortInfluentialNode(ninfo);
    // outputNodeInfo(ninfo);
    // checkFork(ninfo);

    vector<ASNInfo> asninfo;
    updateNodesInfluentialSequenceBy(asninfo);
    return 0;
}
