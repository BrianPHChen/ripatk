#include<iostream>
#include<fstream>
#include <nlohmann/json.hpp>
#include <set>

// for convenience
using json = nlohmann::json;
using namespace std;

struct Node {
  string node_public_key;
//   string ip;
//   string version;
//   int uptime;
//   int inbound_count;
//   int outbound_count;
//   string rowkey;
//   string country;
//   string country_code;
//   string latitude;
//   string longitude;
//   string timezone;
};

struct Link {
    string source;
    string target;
};

void from_json(const nlohmann::json& j, Node& n) {
    j.at("node_public_key").get_to(n.node_public_key);
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

void BronKerbosch(set<int> R, set<int> P, set<int> X, vector<vector<bool>> &adj, unordered_map<int, int> &cliquesNum) {
    
    if (P.size() == 0 && X.size() == 0) {
        //cout << "maximal size: " << R.size() << endl;
        if (cliquesNum.find(R.size()) == cliquesNum.end()) {
            cliquesNum[R.size()] = 1;
        } else {
            cliquesNum[R.size()]++;
        }
        // cout << "set: { ";
        // for(auto& e : R) {
        //     cout << e << " ";
        // }
        // cout << "}" << endl;
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
            BronKerbosch(R, intersection(P, neighbor(i, adj)), intersection(X, neighbor(i, adj)), adj, cliquesNum);
            R.erase(i);
            P.erase(i);
            X.insert(i);
        }
    }
}

int main(int argc, char *argv[]){
    // int indent = 4;
    ifstream file;
    json rippleJson;
    map<string, int> nodeMap;
    file.exceptions (ifstream::failbit | ifstream::badbit );

    try {
        file.open("../doc/Ripple.json");
        file >> rippleJson;
        file.close();
    } catch (fstream::failure &e) {
        cerr << "Exception opening/reading/closing file\n";
    }

    vector<Node> nodes = rippleJson.at("nodes").get<vector<Node>>();
    vector<Link> links = rippleJson.at("links").get<vector<Link>>();
    cout << "nodes number: " << nodes.size() << endl;
    cout << "links number: " << links.size() << endl;
    cout << "\n";
    
    for (int idx = 0; idx < int(nodes.size()); idx++) {
        pair<map<string, int>::iterator, bool> res;
        res = nodeMap.insert(pair<string, int>(nodes[idx].node_public_key, idx));
        if (res.second == false) {
            cerr << "pubkey: " << nodes[idx].node_public_key << " has already existed";
            exit(1);
        }
    }

    // adjacency matrix
    vector< vector<bool> > adj(nodes.size(), vector<bool>(nodes.size(), false));

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
    unordered_map<int, int> cliquesNum;
    set<int> R, P, X;
    for (int idx = 0; idx < int(nodes.size()); idx++) { 
        P.insert(idx);
    }
    BronKerbosch(R, P, X, adj, cliquesNum);
    for (auto iter = cliquesNum.begin(); iter != cliquesNum.end(); ++iter) {
        cout << "There are " << iter->second << " " << iter->first << "-nodes cliques.\n";
    }
    return 0;
}