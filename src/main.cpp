#include<iostream>
#include<fstream>
#include <nlohmann/json.hpp>
#include <set>

// for convenience
using json = nlohmann::json;
using namespace std;

struct Node {
    string node_public_key;
    // string ip_address;
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
    unordered_map<int, int> number;
};

void from_json(const nlohmann::json& j, Node& n) {
    try {
        j.at("node_public_key").get_to(n.node_public_key);
        // j.at("ip").get_to(n.ip_address);
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
        ninfo.cliques.push_back(R);
        if (ninfo.number.find(R.size()) == ninfo.number.end()) {
            ninfo.number[R.size()] = 1;
        } else {
            ninfo.number[R.size()]++;
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
    }
    // if (!isSymmetric(adj)) {
    //     printf("the matrix is not symmetric");
    //     exit(1);
    // }
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
    for (auto iter = ninfo.number.begin(); iter != ninfo.number.end(); ++iter) {
        cout << "There are " << iter->second << " " << iter->first << "-nodes cliques.\n";
    }
    return 0;
}
