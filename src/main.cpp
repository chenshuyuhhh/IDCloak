#include "cryptoTools/Network/IOService.h"
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
// #include "libdivide.h"
#include "Party.h"
#include "PartyL.h"
#include "PartyN.h"
#include "volePSI/RsCpsi.h"
// #include "volePSI/RsMulpsi.h"
#include "volePSI/RsPsi.h"
#include "volePSI/SimpleIndex.h"
#include "volePSI/fileBased.h"

using namespace std;

int readN() {
  ifstream infile("configure.json");
  // cout << basepath << endl;
  if (!infile.is_open()) {
    cerr << "Error: Failed to open the file: configure.json\n";
    return 0;
  }

  map<std::string, size_t> variables;
  string line;
  while (getline(infile, line)) {
    // istringstream iss(line);
    string key;
    size_t value;
    string vtemp;
    line = line.substr(line.find_first_not_of(" \t"),
                       line.find_last_not_of(" \t") + 1);
    if (line.find(":") != std::string::npos) {
      key = line.substr(1, line.find(":") - 2);
      value = std::stoi(line.substr(line.find(":") + 1));
      // key.erase(remove(key.begin(), key.end(), '\"'), key.end());
      // value.erase(remove(value.begin(), value.end(), '\"'), value.end());
#ifdef DEBUG_INFO
      std::cout << key << std::endl;
#endif
      // key = key.substr(key.find_first_of('\"') + 1, key.length() - 1);
      // stringstream ss(vtemp);
      if (key == "nParties") {
        infile.close();
#ifdef DEBUG_INFO
        std::cout << value << std::endl;
#endif
        return value;
      }
      // ss >> value;
      variables[key] = value;
    }
  }
#ifdef DEBUG_INFO
  for (auto &kv : variables) {
    cout << kv.first << ": " << kv.second << endl;
  }
#endif
  return variables["nParties"];
}

void Exp(oc::CLP &cmd) {
  srand(time(0));
  try {
    // configure variable
    int nP = readN(); // the party num
    // Party::readConfig();Dqzgslb22
    // std::cout << nP << std::endl;
    int myIdx = cmd.get<int>("p"); // the number of party
    // std::cout << myIdx << std::endl;
    bool normal = cmd.getOr<bool>(
        "l",
        (myIdx == (nP - 1)) ? false
                            : true); // set pn-1 is leader(receiver), and the
                                     // other parties are sender. size_t t = 2;
                                     // std::cout << nP << " " << myIdx << " "
                                     // << normal << std::endl;
#ifdef DEBUG_INFO
    // std::cout << nP << " " << myIdx << " " << normal << std::endl;
#endif

    volePSI::Party *party;
    if (normal)
      party = new volePSI::NormalParty();
    else
      party = new volePSI::LeaderParty();
#ifdef DEBUG_INFO
    std::cout << "Begin init\n";
#endif
    party->init(cmd); // config data and connection
#ifdef DEBUG_INFO
    std::cout << "End init\n-------------------------------\n";
#endif
    party->SecVPre();
    // std::cout << "(End)" << std::endl;
#ifdef DEBUG_INFO
    std::cout << "(End)" << std::endl;
#endif
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  // std::cout << "exp" << std::endl;
  return;
  // std::cout << "exp" << std::endl;
}

int main(int argc, char *argv[]) {
  oc::CLP cmd(argc, argv);
  Exp(cmd);
}