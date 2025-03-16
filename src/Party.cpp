#include "Party.h"
// #include "coproto/Socket/AsioSocket.h"
#include "MPC/AddShare.h"
#include "Network/EigenSocket.h"
#include "volePSI/fileBased.h"
#include <cstdint>
#include <set>
#include <string>
#include <bitset>
#include "MPC/BlongShare.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <unistd.h>
#include <map>
#include <vector>
#include "Network/CoproSocket.h"
#include <typeinfo>
// #include "Network/EigenSocket.h"
// #define DEBUG_INFO

using namespace std;
// #define EIGEN_CONNECT ;
// #include <algorithm>

namespace volePSI
{
    std::vector<coproto::AsioSocket> psocks; // connection psocks
    size_t nParties;                         // the party num
    size_t myIdx;                            // the number of party
    size_t lIdx;                             // the number of lead party
    bool normal;                             // leader party(0), normal party(1)
    PRNG pPrng;
    std::string basepath = "./";
    Party::Party()
    {
    }

    // Party::Party(size_t size, std::vector<block> mset, MatrixView<u8> mvalues)
    // {
    //     this->setSize = size;
    //     this->mset = mset;
    //     this->mvalues = mvalues;

    //     // configure params
    // }

    void Party::setSet(std::vector<block> mset)
    {
        assert(mset.size() == setSize);
        this->mset = mset;
    }

    // void Party::setValues(std::vector<u8> mvalues)
    // {
    //     assert(mvalues.size() == setSize);
    //     values = mvalues;
    // }

    // void Party::setkvs(std::vector<block> mset, std::vector<u8> mvalues)
    // {
    //     assert(mset.size() == setSize && mvalues.size() == setSize);
    //     this->mset = mset;
    //     this->values = mvalues;
    // }

    void Party::testShare()
    {
        std::cout << "-----------------------------\n1.Bool convert to arith\n";
        int size = 4;
        std::string tmp = "";
        for (size_t i = 0; i < size; i++)
        {
            tmp += std::to_string(rand() % 2);
        }
        oc::BitVector data(tmp);
        BoolShare bshare(data);
        AShare ashare;
        // bshare.print();
        bshare.Print();
        bshare.Convert2A(ashare);
        bshare.RevealPrint();
        ashare.RevealPrint();

        std::cout << "-----------------------------\n2.Sum\n";
        int sum_size = 1;
        AShare sum;
        ashare.Sum(sum);
        sum.RevealPrint();

        std::cout << "-----------------------------\n3.Plain compare\n";
        sum.RevealPrint();
        oc::Matrix<bool> cmp(sum_size, 1);
        u64 c = 3;
        sum.BiggerPlain(c, cmp);
        std::cout << "cmp " << c << ": ";
        for (size_t i = 0; i < sum_size; i++)
        {
            std::cout << cmp(i) << " ";
        }
        std::cout << std::endl;
        c = 79;
        sum.BiggerPlain(c, cmp);
        std::cout << "cmp " << c << ": ";
        for (size_t i = 0; i < sum_size; i++)
        {
            std::cout << cmp(i) << " ";
        }
        std::cout << std::endl;

        std::cout << "-----------------------------\n4.Shuffle\n";
        AShare shuffleshare;
        AShare pieinv;
        ashare.Shuffle(shuffleshare, pieinv);
        std::cout << "origin: ";
        ashare.RevealPrint();
        std::cout << "shuffle: ";
        shuffleshare.RevealPrint();
        pieinv.RevealPrint();

        std::cout << "----------------------------\n5.Secret share\n";
        if (myIdx == trustp)
        {
            shuffleshare.Print();
        }
        shuffleshare.SecretShare(trustp);
        // ashare.Print();
        // ashare.Print();
        shuffleshare.RevealPrint();

        std::cout << "-----------------------------\n6.Equal zero\n";
        AShare zeros;
        shuffleshare.EqualZero(zeros);
        // std::cout << zeros.size() << " ";
        zeros.RevealPrint();

        std::cout << "-----------------------------\n7.Small share\n";
        SmallShare smallshare;
        oc::Matrix<u8> v(size, 1);
        if (myIdx == trustp)
        {
            for (size_t i = 0; i < size; i++)
            {
                v(i) = rand() % 256;
                std::cout << +v(i) << " ";
            }
            std::cout << std::endl;
            smallshare.SetShare(v);
        }
        // SmallShare smallshare(v);
        smallshare.SecretShareSize(trustp);
        // smallshare.Print();
        smallshare.RevealPrint();

        std::cout << "-----------------------------\n8.Bool share and\n";
        size_t bsize = 10;
        std::string tmp1 = "";
        std::string tmp2 = "";
        for (int i = 0; i < bsize; i++)
        {
            tmp1 += std::to_string(rand() % 2);
            tmp2 += std::to_string(rand() % 2);
        }
        // std::string tmp1 = "0110111111";
        // std::string tmp2 = "0100111010";

        BoolShare b1(tmp1);
        BoolShare b2(tmp2);
        BoolShare b12 = b1 & b2;
        std::cout << "b1 b2\n";
        b1.RevealPrint();
        b2.RevealPrint();
        std::cout << "b1 & b2\n";
        b12.RevealPrint();

        std::cout << "-----------------------------\n9.Blong share\n";
        std::cout << "-----------------------------\n9.1 - minus\n";
        BlongShare blongshare1;

        oc::Matrix<u64> btemp1(size, 1);
        // oc::Matrix<u64> btemp2(size, 1);
        btemp1(0) = 0;
        for (int i = 1; i < size; i++)
        {
            btemp1(i) = rand() % LONG_LONG_MAX;
            // btemp2(i) = rand() % LONG_LONG_MAX;
            std::cout << btemp1(i) << " "; // << btemp2(i) << std::endl;
        }
        std::cout << std::endl;
        blongshare1.SetShare(btemp1);
        // blongshare2.SetShare(btemp2);
        std::cout << "bs1\n";
        blongshare1.RevealPrint();
        std::cout << "-bs1\n";
        BlongShare blongshare2 = -blongshare1;
        blongshare2.RevealPrint();

        std::cout << "-----------------------------\n9.2 iszero\n";
        AShare zeros9;
        blongshare1.isZero(zeros9);
        blongshare1.RevealPrint();
        std::cout << "zeros\n";
        // zeros9.Print();
        zeros9.RevealPrint();
        // std::cout << "bs1\n";
        // blongshare1.RevealPrint();
        // std::cout << "bs2\n";
        // blongshare2.RevealPrint();
        // std::cout << "bs1 + bs2 =\n";
        // (blongshare1 + blongshare2).RevealPrint();

        // blongshare2.RevealPrint();
        // // AShare ones;
        // // blongshare.isZero(ones);
        // std::cout << "-----------------------------" << std::endl
        //           << "8.1 Blong and" << std::endl;
    }

    void Party::testAddShare()
    {
        // std::cout << "-----------------------------\n";
        std::cout << "1.add -----------------------------\n";
        size_t rows = 4;
        Mat2d data(rows, 2);
        if (myIdx == 0)
        {
            data << 1, 2, 3, 4, 5, 6, 7, 8;
        }
        else
        {
            data = Mat2d::Zero(rows, 2);
        }
        // std::cout << data << std::endl;
        AddShare x(data);
        // std::cout << "x:\n";
        x.Reveal("x");

        Mat2d c(rows, 2);
        c << 1, 2, 3, 4, 5, 6, 7, 8;
        AddShare z = x + c;
        z.Reveal("z=x+c");

        std::cout << "2.self mul -----------------------------\n";
        Mat2d data2(rows, 1);
        if (myIdx == 0)
        {
            data2 << 1, 3, 5, 7;
        }
        else
        {
            data2 = Mat2d::Zero(rows, 1);
        }
        AddShare y(data2);
        y.Reveal("y");
        x *= y;
        x.Reveal("x");

        std::cout << "3.shuffle -----------------------------\n";
        AddShare xshu;
        std::vector<AddShare> net;
        x.ShuffleOFFTrick(net);
        x.Shuffle(xshu, net, false);
        xshu.Reveal("x shuffle");
    }

    coproto::AsioSocket Party::setSock(std::string ip, unsigned short port, bool server)
    {
        coproto::AsioSocket sock;
        if (cmd.isSet("tls"))
        {
            std::string certDir = cmd.getOr<std::string>("cert", "./thirdparty/libOTe/thirdparty/coproto/tests/cert/");
            std::string CACert = cmd.getOr("CA", certDir + "/ca.cert.pem");
            auto privateKey = cmd.getOr("sk", certDir + "/" + (normal ? "client" : "server") + "-0.key.pem");
            auto publicKey = cmd.getOr("pk", certDir + "/" + (normal ? "client" : "server") + "-0.cert.pem");

            if (!volePSI::exist(CACert) || !volePSI::exist(privateKey) || !volePSI::exist(privateKey))
            {
                if (!volePSI::exist(CACert))
                    std::cout << "CA cert " << CACert << "does not exist" << std::endl;
                if (!volePSI::exist(privateKey))
                    std::cout << "private key " << privateKey << "does not exist" << std::endl;
                if (!volePSI::exist(privateKey))
                    std::cout << "public key " << publicKey << "does not exist" << std::endl;

                std::cout << "Please correctly set -CA=<path> -sk=<path> -pk=<path> to the CA cert, user private key "
                          << " and public key respectively. Or run the program from the volePSI root directory to use the "
                          << Color::Red << "insecure " << Color::Default
                          << " sample certs/keys that are provided by coproto. " << std::endl;
            }

#ifdef COPROTO_ENABLE_OPENSSL
            // boost::asio::ssl::context ctx(client ? boost::asio::ssl::context::tlsv13_client : boost::asio::ssl::context::tlsv13_server);

            // // Require that the both parties verify using their cert.
            // ctx.set_verify_mode(
            //     boost::asio::ssl::verify_peer |
            //     boost::asio::ssl::verify_fail_if_no_peer_cert);
            // ctx.load_verify_file(CACert);
            // ctx.use_private_key_file(privateKey, boost::asio::ssl::context::file_format::pem);
            // ctx.use_certificate_file(publicKey, boost::asio::ssl::context::file_format::pem);

            // // Perform the TCP/IP and TLS handshake.
            // sock = coproto::sync_wait(
            //     client ? macoro::make_task(coproto::AsioTlsConnect(ip, coproto::global_io_context(), ctx)) : macoro::make_task(coproto::AsioTlsAcceptor(ip, coproto::global_io_context(), ctx)));
#else
            throw std::runtime_error("COPROTO_ENABLE_OPENSSL must be define (via cmake) to use TLS sockets. " COPROTO_LOCATION);
#endif
        }
        else
        {
#ifdef COPROTO_ENABLE_BOOST
            // std::cout << "server" << server << std::endl;
            sock = coproto::asioConnect(ip, port, server);
#else
            throw std::runtime_error("COPROTO_ENABLE_BOOST must be define (via cmake) to use tcp sockets. " COPROTO_LOCATION);
#endif
        }
        return sock;
    }

    coproto::AsioSocket Party::setSock(std::string ip, bool server)
    {
        // if (this->cmd == nullptr)
        //     return;
        coproto::AsioSocket sock;
        if (cmd.isSet("tls"))
        {
            std::string certDir = cmd.getOr<std::string>("cert", "./thirdparty/libOTe/thirdparty/coproto/tests/cert/");
            std::string CACert = cmd.getOr("CA", certDir + "/ca.cert.pem");
            auto privateKey = cmd.getOr("sk", certDir + "/" + (normal ? "client" : "server") + "-0.key.pem");
            auto publicKey = cmd.getOr("pk", certDir + "/" + (normal ? "client" : "server") + "-0.cert.pem");

            if (!volePSI::exist(CACert) || !volePSI::exist(privateKey) || !volePSI::exist(privateKey))
            {
                if (!volePSI::exist(CACert))
                    std::cout << "CA cert " << CACert << "does not exist" << std::endl;
                if (!volePSI::exist(privateKey))
                    std::cout << "private key " << privateKey << "does not exist" << std::endl;
                if (!volePSI::exist(privateKey))
                    std::cout << "public key " << publicKey << "does not exist" << std::endl;

                std::cout << "Please correctly set -CA=<path> -sk=<path> -pk=<path> to the CA cert, user private key "
                          << " and public key respectively. Or run the program from the volePSI root directory to use the "
                          << Color::Red << "insecure " << Color::Default
                          << " sample certs/keys that are provided by coproto. " << std::endl;
            }

#ifdef COPROTO_ENABLE_OPENSSL
            // boost::asio::ssl::context ctx(client ? boost::asio::ssl::context::tlsv13_client : boost::asio::ssl::context::tlsv13_server);

            // // Require that the both parties verify using their cert.
            // ctx.set_verify_mode(
            //     boost::asio::ssl::verify_peer |
            //     boost::asio::ssl::verify_fail_if_no_peer_cert);
            // ctx.load_verify_file(CACert);
            // ctx.use_private_key_file(privateKey, boost::asio::ssl::context::file_format::pem);
            // ctx.use_certificate_file(publicKey, boost::asio::ssl::context::file_format::pem);

            // // Perform the TCP/IP and TLS handshake.
            // sock = coproto::sync_wait(
            //     client ? macoro::make_task(coproto::AsioTlsConnect(ip, coproto::global_io_context(), ctx)) : macoro::make_task(coproto::AsioTlsAcceptor(ip, coproto::global_io_context(), ctx)));
#else
            throw std::runtime_error("COPROTO_ENABLE_OPENSSL must be define (via cmake) to use TLS sockets. " COPROTO_LOCATION);
#endif
        }
        else
        {
#ifdef COPROTO_ENABLE_BOOST
            // Perform the TCP/IP.
            // if (localPort > 0)
            // {
            //     sock = coproto::asioConnect(ip, localPort, server);
            // }
            // else
            // {
            sock = coproto::asioConnect(ip, server);
            // }
#else
            throw std::runtime_error("COPROTO_ENABLE_BOOST must be define (via cmake) to use tcp sockets. " COPROTO_LOCATION);
#endif
        }
        return sock;
    }
    // u32 getSourcePort(coproto::Socket &sock)
    // {
    //     auto fd = sock.getNative()

    //     struct sockaddr_in addr;
    //     socklen_t addrLen = sizeof(addr);
    //     if (getsockname(fd, (struct sockaddr *)&addr, &addrLen) < 0)
    //     {
    //         perror("getsockname");
    //         exit(EXIT_FAILURE);
    //     }
    //     return ntohs(addr.sin_port);
    // }

    void Party::connect()
    {
        if (etype == 2 || etype == 3)
        {
            socket_init();
#ifdef DEBUG_INFO
        std::cout << "socket1 finished" << std::endl;
#endif
        }
        
        psocks.resize(nParties);
        for (size_t i = 0; i < myIdx; i++)
        {
            u32 port = 1200 + myIdx * 100 + i;
            u32 sport = 1200 + i * 100 + myIdx;
            std::string ip = cmd.getOr<std::string>("ip" + std::to_string(myIdx) + std::to_string(i), "localhost:" + std::to_string(port));
            // cout << i << endl;
            psocks[i] = setSock(ip, sport, false);
            // psocks[i] = setSock(ip,  false);
        }
        for (size_t i = myIdx + 1; i < nParties; i++)
        {
            u32 port = 1200 + i * 100 + myIdx;
            u32 sport = 1200 + myIdx * 100 + i;
            std::string ip = cmd.getOr<std::string>("ip" + std::to_string(i) + std::to_string(myIdx), "localhost:" + std::to_string(port));
            // cout << i << endl;
            psocks[i] = setSock(ip, sport, true);
            // psocks[i] = setSock(ip,  true);
        }
        // for (size_t i = 0; i < myIdx; i++)
        // {
        //     u32 sport = 1200 + myIdx * 100 + i;
        //     u32 port = 1200 + i * 100 + myIdx;
        //     std::string ip = cmd.getOr<std::string>("ip" + std::to_string(myIdx) + std::to_string(i), "localhost:" + std::to_string(port));
        //     psocks[i] = setSock(ip,sport, true);
        // }
        // for (size_t i = myIdx + 1; i < nParties; i++)
        // {
        //     u32 sport = 1200 + i * 100 + myIdx;
        //     u32 port = 1200 + myIdx * 100 + i;
        //     std::string ip = cmd.getOr<std::string>("ip" + std::to_string(i) + std::to_string(myIdx), "localhost:" + std::to_string(port));
        //     psocks[i] = setSock(ip,sport, false);
        // }
#ifdef DEBUG_INFO
        std::cout << "socket2 finished" << std::endl;
#endif
        SyncP();
    }

    void Party::delConnect()
    {
        if (etype == 2 ||  etype == 3)
        {
        //     connect_type = 1;
            socket_del();
        //     return;
        }
        // socket_del();

        // Close all sockets in psocks
        for (size_t i = 0; i < psocks.size(); i++)
        {
            if (i != myIdx)
            {
                // if (psocks[i]!=-1)
                // {
                // close(psocks[i]);
                try
                {
                    // coproto::Socket* baseSock = &psocks[i];
                    // if (typeid(*baseSock) == typeid(coproto::AsioSocket)) {
                    //     auto asioSocket = dynamic_cast<coproto::AsioSocket*>(baseSock);

                    //     if (asioSocket) {
                    //         asioSocket->mSock->close(); // Access Sock's close method
                    //     } else {
                    //         std::cerr << "Failed to cast Socket to AsioSocket" << std::endl;
                    //     }
                    // }
                    macoro::sync_wait(psocks[i].close());
                    macoro::sync_wait(psocks[i].mSock->close());
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error closing socket: " << e.what() << std::endl;
                }
            }
            // }
        }
        psocks.clear();
    }

    void Party::getFakeData()
    {
        // Use dummy set
        // init key and value
        mset.resize(setSize);
        std::vector<u8> values;
        // std::cout << values.size() << std::endl;
        if (normal)
        {
            // u8 valuesTmp[setSize];
            for (size_t i = 0; i < setSize; ++i)
            {
                // sendSet[i] = block(rand() % 0x7fffffff, rand() % 0x7fffffff);
                mset[i] = block(0, i + 3);
                // values[i] = i * 10;
            }
        }
        else
        {
            for (size_t i = 0; i < setSize; ++i)
            {
                mset[i] = block(0, i + 1);
                // values[i] = i * 10;
            }
        }

        // mvalues.reshape(setSize, vcols);
#ifdef DEBUG_INFO
        std::cout << "set " << setSize << std::endl;
        for (size_t i = 0; i < setSize; i++)
        {
            std::cout << mset[i] << " ";
        }
        std::cout << std::endl;
#endif
    }

    void Party::readConfig()
    {
        ifstream infile(basepath + "configure.json");
        // cout << basepath << endl;
        if (!infile.is_open())
        {
            cerr << "Error: Failed to open the file: " << basepath << "configure.json\n";
            return;
        }

        map<string, size_t> variables;
        string line;
        while (getline(infile, line))
        {
            // istringstream iss(line);
            string key;
            size_t value;
            string vtemp;
            line = line.substr(line.find_first_not_of(" \t"), line.find_last_not_of(" \t") + 1);
            if (line.find(":") != std::string::npos)
            {
                key = line.substr(1, line.find(":") - 2);
                if (key == "dir")
                {
                    line = line.substr(line.find(":") + 1);
                    size_t start = line.find("\"");
                    size_t end = line.find("\"", start + 1);
                    data_file = line.substr(start + 1, end - start - 1);
                    // cout << data_file << endl;
                }
                else
                {
                    value = std::stoi(line.substr(line.find(":") + 1));
                    // key.erase(remove(key.begin(), key.end(), '\"'), key.end());
                    // value.erase(remove(value.begin(), value.end(), '\"'), value.end());
                    // std::cout << key << std::endl;
                    // key = key.substr(key.find_first_of('\"') + 1, key.length() - 1);
                    // std::cout << key << std::endl;
                    // stringstream ss(vtemp);
                    // ss >> value;
                    variables[key] = value;
                }
            }
        }
#ifdef DEBUG_INFO
        for (auto &kv : variables)
        {
            cout << kv.first << ": " << kv.second << endl;
        }
#endif

        // ifstream ifs(basepath + "configure.json");
        // Json::Reader reader;
        // Json::Value variables;
        // reader.parse(ifs, variables);
        nParties = variables["nParties"];
        if (variables["setSize"] <= 24)
        {
            this->setSize = 1ull << variables["setSize"];
        }
        else
        {
            this->setSize = variables["setSize"];
        }
        this->numThread = variables["numThread"];
        this->ssp = variables["ssp"];
        // this->cols = variables["cols"];
        this->etype = variables["e"];
        this->col_sum = 0;
        this->shu_method = variables["shuffle"];
        this->srow = variables["srow"];
        this->scol = variables["scol"];
        this->offline = variables["offline"];
        this->comm = variables["comm"] == 1 ? true : false;
        this->DEBUG_LOG = variables["debug"] == 1 ? true : false;
        size_t temp;
        // this->pcol.resize(nParties);

        if (variables["setSize"] == 12)
        {
            shareinfo.numBins = 5256;
        }
        else if (variables["setSize"] == 16)
        {
            shareinfo.numBins = 85196;
        }
        else if (variables["setSize"] == 20)
        {
            shareinfo.numBins = 1380626;
        }
        else if (variables["setSize"] == 24)
        {
            shareinfo.numBins = 22369622;
        }
        else
        {
            shareinfo.numBins = this->setSize;
        }

        for (int i = 0; i < nParties; i++)
        {
            temp = variables["col" + to_string(i)];
            pcol.push_back(temp);

            int row = variables["row" + to_string(i)];
            prow.push_back(row);

            col_sum += temp;
        }

        lIdx = nParties - 1;
    }

    void Party::configCmd()
    {
        // this->cmd = cmd;
        // nParties = cmd.getOr("pn", 3);
        readConfig();

        myIdx = cmd.get<int>("p");

        // this->setSize = 1ull << cmd.getOr("nn", 10);
        // this->ssp = cmd.getOr("ssp", 40);
        // this->numThread = cmd.getOr("nt", 1);
        // auto f = cmd.isSet("f"); // whether to exchange features (have value)
        this->cols = pcol[myIdx]; // the column of feature
                                  // #ifdef DEBUG_INFO
                                  //         std::cout << nParties << " " << this->setSize << " " << myIdx << " " << cols << std::endl;
                                  // #endif
        this->vcols = cols << 3;
        // #ifdef DEBUG_INFO
        //         std::cout << cols << " " << vcols << " " << myIdx << std::endl;
        // #endif
        // this->mal = cmd.isSet("malicious");
        normal = cmd.getOr<bool>("l", (myIdx == (nParties - 1)) ? false : true); // set pn-1 is leader(receiver), and the other parties are sender.
        params = oc::CuckooIndex<>::selectParams(setSize, ssp, 0, 3);
        this->keyBitLength = ssp + oc::log2ceil(params.numBins()); // oc::log2ceil == bitwidth  y||j
        shareinfo.numBins = params.numBins();
        // shareinfo.numBins += (shareinfo.numBins % 2);
        // this->params = oc::CuckooIndex<>::selectParams(setSize, ssp, 0, 3);

        // std::cout << nParties << " " << normal << "  setSize: " << setSize << "  cols:" << cols << std::endl;
    }

    void Party::getFileData()
    {
        string filename = data_file + "/" + to_string(nParties) + "_" + to_string(myIdx) + ".csv";
        ifstream file(filename);
        char cwd[1024];
        // if (getcwd(cwd, sizeof(cwd)) != nullptr)
        // {
        //     std::cout << "Current path is: " << cwd << std::endl;
        // }
        if (file)
        {
            std::cout << filename << std::endl;
            string line;
            u64 id;
            string cell;
            std::vector<u8> values;
            getline(file, line);

            while (getline(file, line))
            {
                // cout << line << endl;
                stringstream lineStream(line);
                getline(lineStream, cell, ',');
                // std::cout << id << std::endl;
                id = stol(line.substr(0, line.find(","))); // id
                // cout << id << endl;
                // id = stol(cell); // id
                mset.emplace_back(block(0, id));

                // features
                // while (getline(lineStream, cell, ','))
                // {
                //     values.emplace_back(stoi(cell));
                // }
            }
            // mvalues = MatrixView<u8>(values.begin(), values.end(), cols);
        }
        else
        {
            getFakeData();
        }
    }

    void Party::printLocalData()
    {
        std::cout << "SetSize:" << setSize << "  ValueSzie:" << cols << std::endl;
        for (size_t i = 0; i < setSize; i++)
        {
            std::cout << mset[i] << ":\n";
            // for (size_t j = 0; j < mvalues.cols(); j++)
            // {
            //     std::cout << +mvalues(i, j) << " ";
            // }
            std::cout << std::endl;
        }
    }

    void Party::init()
    {
        pPrng.SetSeed(ZeroBlock);
        if (mset.empty())
        {
            getFileData();
            // getFakeData();
        }
#ifdef DEBUG_INFO
        std::cout << "0.Data created\n";
#endif

        connect();
#ifdef DEBUG_INFO
        std::cout << "0.Connected\n----------------------\n";
#endif
    }

    void Party::init(const CLP &cmd)
    {
        this->cmd = cmd;
        configCmd();
#ifdef DEBUG_INFO
        std::cout << "configure variables\n";
#endif
        init();
    }

    bool Party::thrJudge(size_t c)
    {
        /**
        // ss.print();
        // AShare ashare;
        // Matrix<u64> flag;
        // convertAShare(ss.mFlag, flag);
        // AShare ashare(ss.mLongFlag);
        // ashare.RevealPrint();
        ss.GenflagShare(flagrand);
        // flagrand.RevealPrint();
        // AShare zeros;
        flagrand.isZero(flagshare);
        // flagshare = zeros;
        // flagshare.RevealPrint();
        // flagshare.RevealPrint();
        AShare nshare; // the num of candidates
        flagshare.Sum(nshare);
        // nshare.RevealPrint();
        Matrix<bool> result(1, 1);
        nshare.BiggerPlain(c, result);
        return result(0);
        */

        // AShare ashare;
        // bshare.Convert2A(ashare);
        // ashare.RevealPrint();
        // AShare nshare; // the num of candidates
        // ashare.Sum(nshare);
        // Matrix<bool> result(1, 1);
        // nshare.BiggerPlain(c, result);
        // return result(0);
    }

    /**
     * get the indexPool
     * every col of indexPool is a one-hot
     */
    void Party::IndexPool()
    {
        // shuffle
        AShare shuffleIndex;
        flagshare.Shuffle(shuffleIndex, pieInv);
        // reveal
        shuffleIndex.Reveal(shufflePlain);
        std::vector<u64> ps;
        u64 sum = 0;
        for (size_t j = 0; j < shufflePlain.size(); j++)
        {
            if (shufflePlain(j))
            {
                sum++;
                ps.push_back(j);
            }
            // std::cout << shufflePlain(j) << " ";
        }
        // std::cout << std::endl;

        // expand shuffleplain
        Matrix<u64> indexs(shufflePlain.size(), sum, osuCrypto::AllocType::Zeroed);
        for (size_t j = 0; j < sum; j++)
        {
            indexs(ps[j], j) = 1;
        }

        // for (int i = 0; i < indexs.rows(); i++)
        // {
        //     for (size_t j = 0; j < indexs.cols(); j++)
        //     {
        //         std::cout << indexs(i, j) << " ";
        //     }
        //     std::cout << std::endl;
        // }

        // deshuffle
        indexPool = pieInv * indexs;
        // indexPool.RevealPrint();
    }

    void Party::printIndexPool()
    {
        Matrix<u64> indexs;
        indexPool.Reveal(indexs);
        int temp;
        for (size_t j = 0; j < indexs.cols(); j++)
        {
            temp = 0;
            for (size_t k = 0; k < indexs.rows(); k++)
            {
                temp += indexs(k, j);
                if (indexs(k, j) == 1)
                {
                    std::cout << k << " ";
                }
            }
            if (temp != 1)
            {
                std::cout << "Index failed\n";
                return;
            }
        }
        std::cout << "\n";
    }

    void Party::printMergeData()
    {
        Matrix<u8> plain[nParties];
        for (size_t i = 0; i < nParties; i++)
        {
            // std::cout << i << ":";
            valueshare[i].Reveal(plain[i]);
            // for (size_t j = 0; j < plain.rows(); j++)
            // {
            //     std::cout << j << ") ";
            //     for (size_t k = 0; k < plain.cols(); k++)
            //     {
            //         std::cout << +plain(j, k) << " ";
            //     }
            //     std::cout << " |";
            // }
            // std::cout << std::endl;
        }

        Matrix<u64> indexs;
        indexPool.Reveal(indexs);
        int temp;
        vector<u64> ps;
        for (size_t j = 0; j < indexs.cols(); j++)
        {
            temp = 0;
            for (size_t k = 0; k < indexs.rows(); k++)
            {
                temp += indexs(k, j);
                if (indexs(k, j) == 1) // k is the index of hash table and T[k] is the intersection element.
                {
                    ps.push_back(k);
                }
            }
            if (temp != 1)
            {
                std::cout << "Index failed\n";
                return;
            }
        }
    }

    void Party::writeMergeData()
    {
        Matrix<u8> plain[nParties];
        for (size_t i = 0; i < nParties; i++)
        {
            // std::cout << i << ":";
            valueshare[i].Reveal(plain[i]);
        }

        Matrix<u64> indexs;
        indexPool.Reveal(indexs);
        int temp;
        vector<u64> ps;
        for (size_t j = 0; j < indexs.cols(); j++)
        {
            temp = 0;
            for (size_t k = 0; k < indexs.rows(); k++)
            {
                temp += indexs(k, j);
                if (indexs(k, j) == 1) // k is the index of hash table and T[k] is the intersection element.
                {
                    ps.push_back(k);
                }
            }
            if (temp != 1)
            {
                std::cout << "Index failed\n";
                return;
            }
        }
    }

    /**
     * ss value of if
     */
    Proto Party::valueShareCo(size_t id)
    {
        MC_BEGIN(Proto, this, id, i = size_t{}, lIdx = lIdx, idxs = std::vector<u64>{});
        // valueshare.resize(nParties);
        // idxs.resize(nParties);
        // leader share self values to other
        if (id == lIdx)
        {
            // std::cout << "------leader value-----" << std::endl;
            if (myIdx == lIdx)
            {
                valueshare[lIdx].SetShare(ss.mValues[lIdx]);
                // valueshare[lIdx].Print();
            }
            MC_AWAIT(valueshare[lIdx].SecretShareSizeCo(lIdx));
        }
        else
        {
            // std::cout << "------" << id << " value-----" << myIdx << std::endl;
            if (myIdx == id)
            {
                // share self value
                // gen share from ss.mvalues
                // std::cout << "self set share\n";
                valueshare[id].SetShare(ss.mValues[id]);
                ss.mValues[id].release();
            }
            else
            {
                if (myIdx == lIdx) // leader share
                {
                    // std::cout << "leader set share\n";
                    valueshare[id].SetShare(ss.mValues[id]); // gen share from ss.mvalues
                    ss.mValues[id].release();
                }

                for (size_t j = 0; j < nParties; j++) // send to idxs[i]
                {
                    if (j != id && j != lIdx)
                        idxs.push_back(j);
                }
                // std::cout << "get share " << id << " from " << lIdx << std::endl;
                MC_AWAIT(valueshare[id].SecretShareSizeCo(lIdx, idxs));
            }
        }

        MC_END();
    }
    Proto Party::valueShareCo()
    {
        // macoro::sync_wait(valueShareCo());
        MC_BEGIN(Proto, this, i = size_t{});
        valueshare.resize(nParties);
        for (i = 0; i < nParties; i++)
        {
            MC_AWAIT(valueShareCo(i));
        }
        MC_END();
    }

    void Party::valueShare()
    {
        // macoro::sync_wait(valueShareCo());
        // std::cout << "begin secret share\n";
        macoro::sync_wait(valueShareCo());
        // std::cout << "end secret share\n";
        // for (size_t i = 0; i < nParties; i++)
        // {
        //     std::cout << i << ":";
        //     valueshare[i].RevealPrint();
        // }
    }

    void Party::printValueShare()
    {
        Matrix<u8> plain;
        for (size_t i = 0; i < nParties; i++)
        {
            std::cout << i << ":";
            valueshare[i].Reveal(plain);
            for (size_t j = 0; j < plain.rows(); j++)
            {
                std::cout << j << ") ";
                for (size_t k = 0; k < plain.cols(); k++)
                {
                    std::cout << +plain(j, k) << " ";
                }
                std::cout << " |";
            }
            std::cout << std::endl;
        }
    }

    void Party::u8tou64(const Matrix<u8> &values, Matrix<u64> &result)
    {
        // std::cout << values.cols() << std::endl;
        assert(values.cols() % 8 == 0);
        size_t col = values.cols() / 8;
        result.resize(values.rows(), col);
        Matrix<u64>::iterator retIter = result.begin();
        //  Matrix<u8>::iterator retIter = ret.begin();
        for (size_t i = 0; i < values.rows(); ++i)
        {
            auto t = retIter;
            auto v = (u64 *)&values(i, 0);
            for (size_t j = 0; j < col; ++j)
            {
                t[j] = v[j];
            }
            retIter += col;
        }
    }

    void Party::convert64(const Matrix<u8> &values, Matrix<u64> &result)
    {
        // Matrix<u64> result;
        result.resize(values.rows(), 1);
        // Matrix<u8>::iterator u8it = values.begin();
        // Matrix<u64>::iterator u64it = result.begin();
        u64 col = values.cols();
        u64 len = col > 8 ? 8 : col;
        u64 temp;
        // u64 col = values.cols();
        // std::cout << len << std::endl;
        for (u64 i = 0; i < values.rows(); ++i)
        {
            temp = 0;
            for (u64 j = 0; j < len; j++)
            {
                // std::cout << std::bitset<8>(values(i, j)) << " ";
                temp = (temp << 8) + values(i, j);
                // std::cout << std::bitset<8>(temp) << "  | ";
            }
            result(i, 0) = temp;
            // std::cout << std::endl;
        }
        // ashare.setAshare(result);
    }

    int64_t Party::SizeEstimation()
    {
        if (shu_method == 2)
        {
            idflagadd = AddShare(shareinfo.numBins, 1);
            // idflagadd = AddShare(srow, scol);
        }
        else
        {
            idflag = AShare(shareinfo.numBins, 1);
        }
        // std::cout << "2.Size Estimation\n";
        if (shu_method == 2)
        {
            AddShare idflagshuffle;
            // size_t size = idflag.getSize();
            // std::cout << size << "\n";
            // AShare r(size, 1);
            // AShare pie(size, size);
            std::vector<AddShare> pieInv;
            // AShare pier(size, 1);
            // idflag.ShuffleOFF(r, pie, pieInv, pier);

            // std::cout << "off finish\n";
// #ifdef DEBUG_INFO
//             std::cout << size << "\n";
// #endif
            if (offline == 0)
            {
                idflagadd.ShuffleOFFTrick(pieInv);
            }
            // std::cout << "off finish\n";
            // idflagadd.PrintSize();
            auto start = std::chrono::system_clock::now();
            // idflag.ShuffleOn(idflagshuffle, r, pie, pier);
            idflagadd.Shuffle(idflagshuffle, pieInv, offline);
            // idflagshuffle.RevealPrint();
            // std::cout << "shuffle\n";
#ifdef DEBUG_INFO
            std::cout << "shuffle\n";
#endif
            idflagshuffle.Reveal(shufflePlainadd);
#ifdef DEBUG_INFO
            // idflagshuffle.RevealPrint();
            std::cout << "reveal\n";
#endif
            // if (e == 2)
            // {
            c = 0;
            for (size_t i = 0; i < shufflePlainadd.size(); ++i)
            {
                if (shufflePlainadd(i) == 0)
                    ++c;
            }
            // }
            auto done = std::chrono::system_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count();
        }
        else
        {
            AShare idflagshuffle;
            size_t size = idflag.getSize();
            // std::cout << size << "\n";
            // AShare r(size, 1);
            // AShare pie(size, size);
            std::vector<AShare> pieInv;
            // AShare pier(size, 1);
            // idflag.ShuffleOFF(r, pie, pieInv, pier);

            // std::cout << "off finish\n";
#ifdef DEBUG_INFO
            std::cout << setSize << "\n";
#endif
            auto start = std::chrono::system_clock::now();
            // idflag.ShuffleOn(idflagshuffle, r, pie, pier);
            idflag.ShuffleNet(idflagshuffle, pieInv);
            // idflagshuffle.RevealPrint();
#ifdef DEBUG_INFO
            std::cout << "shuffle\n";
#endif
            idflagshuffle.Reveal(shufflePlain);
#ifdef DEBUG_INFO
            idflagshuffle.RevealPrint();
            std::cout << "reveal\n";
#endif
            c = 0;
            for (size_t i = 0; i < shufflePlain.size(); ++i)
            {
                if (shufflePlain(i) == 0)
                    ++c;
            }
            auto done = std::chrono::system_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count();
        }
        // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count() << "ms" << std::endl;
    }

    void Party::RecordIDSf()
    {
        if (myIdx == 0)
        {
            ofstream outfile;
            outfile.open(basepath + "plaintext/data/" + "shuffle.txt", ios::out | ios::trunc);
            outfile << c << "\n";
            outfile << shufflePlain.size() << "\n";
            for (size_t i = 0; i < shufflePlain.size(); ++i)
            {
                outfile << shufflePlain(i) << "\n";
            }
        }
    }

    void Party::ReadIDSf()
    {
        ifstream infile;
        infile.open(basepath + "plaintext/data/" + "shuffle.txt", ios::in);
        string line;

        getline(infile, line);
        c = std::stoull(line);
#ifdef DEBUG_INFO
        std::cout << c << std::endl;
#endif

        getline(infile, line);
        size_t size = std::stoull(line);
#ifdef DEBUG_INFO
        std::cout << setSize << std::endl;
#endif

        shufflePlain.resize(size, 1);
        size_t i = 0;
        while (getline(infile, line))
        {
            shufflePlain(i) = std::stoull(line);
            ++i;
        }
        Matrix<u64> pieInvMat(size, size, osuCrypto::AllocType::Zeroed);
        if (myIdx == 0)
        {
            for (size_t i = 0; i < size; i++)
            {
                pieInvMat(i, i) = 1;
            }
        }
        pieInv.setAshare(pieInvMat);
    }

    void Party::mergeF()
    {
        Matrix<u64> data(shareinfo.numBins, col_sum);
        size_t p = 0;
        for (size_t i = 0; i < nParties; ++i)
        {
            Matrix<u64> temp = fs[i].getShare();
            for (size_t j = 0; j < pcol[i]; ++j)
            {
                for (size_t k = 0; k < shareinfo.numBins; ++k)
                {
                    data(k, p) = temp(k, j);
                }
                ++p;
            }
        }
        f = AShare(data);
    }

    Proto Party::SyncPCo()
    {
        MC_BEGIN(Proto, this, i = size_t{}, sdata = std::vector<u64>{}, rdata = std::vector<u64>{});
        sdata.push_back(0);
        rdata.resize(1);
        for (i = 0; i < nParties; i++)
        {
            if (i != myIdx)
            {
                MC_AWAIT(psocks[i].send(sdata));
                MC_AWAIT(psocks[i].recv(rdata));
            }
        }
        MC_END();
    }

    void Party::SyncP()
    {
        macoro::sync_wait(SyncPCo());
        // std::cout << "snyc\n";
    }

    int64_t Party::DataSelection()
    {
        // for (size_t i = 0; i < shufflePlain.size(); i++)
        // {
        //     std::cout << shufflePlain(i) << "|";
        // }
        // std::cout << std::endl;

        // step1: gen index pool
        // data = new AShare[nParties];
        // expand shuffleplain
        auto start = std::chrono::system_clock::now();
        std::vector<u64> ps;
        u64 sum = 0;
        Matrix<u64> indexs(shufflePlain.size(), c, osuCrypto::AllocType::Zeroed);
        size_t i = 0;
        for (size_t j = 0; j < shufflePlain.size(); j++)
        {
            if (shufflePlain(j) == 0)
            {
                indexs(j, i) = 1;
                ++i;
            }
            // std::cout << shufflePlain(j) << " ";
        }
        // Matrix<u64> indexs(shufflePlain.size(), c, osuCrypto::AllocType::Zeroed);
        // for (size_t j = 0; j < c; j++)
        // {
        //     indexs(ps[j], j) = 1;
        // }
        // for (size_t i = 0; i < indexs.rows(); i++)
        // {
        //     for (size_t j = 0; j < indexs.cols(); j++)
        //     {
        //         std::cout << indexs(i, j) << " ";
        //     }
        //     std::cout << std::endl;
        // }
        // pieInv.RevealPrint();
        // std::cout << "before\n";
        AShare indexPool = pieInv * indexs;
        std::cout << "idexpool\n";
        // indexPool.RevealPrint();

        // step2: one-hot select
        mergeF();
        indexPool.DotProduct(data, f);
        // data = indexPool * f;
        auto done = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count();
        // data = new AShare[nParties];
        // std::vector<std::thread> threads;
        // for (size_t i = 0; i < nParties; i++)
        // {
        //     threads.emplace_back([this, i, &indexPool]()
        //                          { data[i] = indexPool * fs[i]; });
        // }

        // for (auto &thread : threads)
        // {
        //     thread.join();
        // }
    }

    int64_t Party::BaseFeatureAlign()
    {
        auto start = std::chrono::system_clock::now();
        // MulPSINormal ln;
        // ln.init(setSize, setSize, vcols, ssp, ZeroBlock, numThread);
        fs = new AShare[nParties];
        std::vector<std::thread> threads;

        for (size_t i = 0; i < nParties; i++)
        {
            fs[i] = AShare(shareinfo.numBins, pcol[i]);

            threads.emplace_back([this, i]()
                                 {
                // cout << i << endl;
                std::set<size_t> ids;
                for (size_t j = 0; j < nParties; j++)
                {
                    if (j != i)
                        ids.insert(j);
                }
                fs[i].SecretShare(i, ids); });
        }

        for (auto &thread : threads)
        {
            thread.join();
        }

        auto done = std::chrono::system_clock::now();
        // std::cout << myIdx << " Normal done: " << std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count() << "ms" << std::endl; // test right
        return std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count();
    }

    int64_t Party::DataSelection2()
    {
        AddShare shufadd;
        std::vector<AddShare> pieInv;

#ifdef DEBUG_INFO
        std::cout << setSize << "\n";
#endif
        // mergeF();
        if (offline == 0)
        {
            fadd.ShuffleOFFTrick(pieInv);
        }
        std::cout << fadd.row << " " << fadd.col << std::endl;
        // std::cout << "off finish\n";
        auto start = std::chrono::system_clock::now();
        // idflag.ShuffleOn(idflagshuffle, r, pie, pier);
        // fadd.PrintSize();
        // pieInv[0].PrintSize();
        fadd.Shuffle(shufadd, pieInv, offline);
        // idflagshuffle.RevealPrint();
        // #ifdef DEBUG_INFO
        // std::cout << "shuffle\n";
        // #endif

        dadd.resize(c, col_sum);
        size_t k = 0;
        // std::cout << shufflePlain.size() << " " << fadd.row << std::endl;
        for (size_t i = 0; i < shufflePlain.size(); ++i)
        {
            if (shufflePlain(i) == 0)
            {
                dadd.row(++k) = shufadd.data.row(i);
            }
        }
        auto done = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count();
    }

    int64_t Party::ExpShuffle()
    {
        if (shu_method == 2)
        {
            AddShare data(srow, scol + 1);
            AddShare shudata;
            std::vector<AddShare> pieInv;

#ifdef DEBUG_INFO
            std::cout << setSize << "\n";
#endif
            if (offline == 0)
            {
                data.ShuffleOFFTrick(pieInv);
            }
            // data.PrintSize();
            auto start = std::chrono::system_clock::now();
            data.Shuffle(shudata, pieInv, offline);
            AddShare data1(srow, 1);
            data1.Reveal();

            auto done = std::chrono::system_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count();
        }
    }

    int64_t Party::ExpShuffleNew()
    {
        AddShare data(srow, scol + 1);
        AddShare shudata(srow,scol + 1);
        shudata.data.setZero();
        
        std::vector<Mat2d> mask(nParties);
        std::vector<Mat2d> shumask1(nParties);
        std::vector<Mat2d> shumask2(nParties);
        for(int i = 0 ; i < nParties ; i++){
            mask[i] = Mat2d(srow, scol + 1);
            mask[i].setZero();

            shumask1[i] = Mat2d(srow, scol + 1);
            shumask1[i].setZero();

            shumask2[i] = Mat2d(srow, scol + 1);
            shumask2[i].setZero();
        }
        

        auto start = std::chrono::system_clock::now();
        data.Shuffle(shudata,mask,shumask1,shumask2,1234+myIdx);

        auto done = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count();
    }

    int64_t Party::ExpMul()
    {
        // std::cout << srow << " " << scol <<std::endl;
        AddShare data(srow, scol);
        AddShare id(srow, 1);

#ifdef DEBUG_INFO
        std::cout << setSize << "\n";
#endif

        auto start = std::chrono::system_clock::now();
        data *= id;

        auto done = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count();
    }

    int64_t Party::ExpMulWithoutOpt()
    {
        AddShare data(srow, scol);
        AddShare id(srow, scol);

#ifdef DEBUG_INFO
        std::cout << setSize << "\n";
#endif

        auto start = std::chrono::system_clock::now();
        data *= id;

        auto done = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count();
    }

    size_t Party::CommSize()
    {
        size_t result = 0;

        // coproto socket
        for (size_t i = 0; i < nParties; i++)
        {
            if (i != myIdx)
            {
                result += psocks[i].bytesSent();
            }
        }
        result += get_comm_size();
        return result;
    }

    size_t Party::SingleCommSize()
    {
        size_t result = 0;
        for (size_t i = 0; i < nParties; i++)
        {
            if (i != myIdx)
            {
                result += psocks[i].bytesReceived();
                result += psocks[i].bytesSent();
            }
        }
        result += get_comm_size_one();
        return result;
    }

    void Party::SecVPre()
    {
        // int e = cmd.get<int>("e");
        // std::cout << "e:" << etype << std::endl;
        int64_t t;
        if (etype == 1) // optpsi
        {
            HashingInputID();
            SyncP();
            t = IdIntersectionDynamic();
        }
        else if (etype == 3) // shuffle
        {
            // std::cout << "5.Shuffle: ";
            t = ExpShuffle();
            // std::cout << t5 << std::endl;
        }
        else if (etype == 2) // me-fa
        {
            HashingInputF();
            SyncP();
            t = FeatureAlign();
        }
        else
        {
            std::cout << "error experiment variables" << std::endl;
        }

        std::cout << "n:" << nParties << "  m:" << setSize << "  cols:" << col_sum << "  scol:" << scol << "  srow:" << srow << "  e:" << etype << "  t:" << t << std::endl;

        SyncP();
#ifdef COMM_TEST
        std::cout << "comm size: " << CommSize() << std::endl;
        std::cout << "one size: " << SingleCommSize() << std::endl;
#endif
        delConnect();
        return;
    }
}