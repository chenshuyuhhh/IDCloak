#pragma once

#ifndef PLAYER
#define PLAYER

#include "cryptoTools/Common/CLP.h"
#include <vector>
#include "cryptoTools/Network/IOService.h"
#include "coproto/Socket/AsioSocket.h"
#include <assert.h>
#include "cryptoTools/Crypto/PRNG.h"
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Common/CuckooIndex.h"
#include "volePSI/GMW/Gmw.h"
#include "volePSI/SimpleIndex.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Common/BitVector.h"
#include "MPC/ShareOp.h"
#include <string>
#include <vector>
#include "Network/Communication.h"
#include "MPC/SmallShare.h"
#include <iostream>
#include <fstream>
#include <thread>
#include "MulPSILeader.h"
#include "MulPSINormal.h"
#include "MulPSIShare.h"
#include "MulPSIOkvs.h"
#include "RsMulpsi.h"
#include "MPC/AddShare.h"
#include "MPC/BlongShare.h"

using namespace oc;

namespace volePSI
{
    class Party
    {
    public:
        // struct Sharing
        // {
        //     // The sender's share of the bit vector indicating that
        //     // the i'th row is a real row (1) or a row (0).
        //     // oc::BitVector mFlagBits;

        //     // Secret share of the values associated with the output
        //     // elements. These values are from the sender.
        //     Matrix<u8> mValues;

        //     // The mapping of the receiver's input rows to output rows.
        //     std::vector<u64> mMapping;

        //     // The mapping of the senders input rows to output rows.
        //     // Each input row might have been mapped to one of three
        //     // possible output rows.
        //     Matrix<u8> mFlag;

        //     // The share of the flag which indicates whether the bucket has intersection ele
        //     // the i'th bucket has intersection element (1)
        //     std::vector<u64> mMapRev;
        // };
    private:
        int connect_type = 0;
        // protected:
    protected:
        // Configurable params
        CLP cmd;
        u64 ssp;  // The statistical security parameter.
        bool mal; // Malicious Security.
        int numThread;
        u64 keyBitLength;
        oc::CuckooParam params;
        // PRNG prng;
        block cuckooSeed;
        ShareInfo shareinfo;

        // data
        size_t setSize;
        size_t cols; // the column of value
        size_t vcols;
        size_t col_sum;
        std::vector<size_t> pcol;
        std::vector<size_t> prow;
        bool comm;
        int etype;
        bool DEBUG_LOG;
        // key-value: mset-values
        std::vector<block> mset;
        // u8 *valuearray;
        std::vector<block> Tx; // after hashing
        // Share ss;` is a commented-out line in the code snippet you provided. It seems to be a declaration of a variable named `ss` of type `Share`, but it is currently commented out and not being used in the code.
        Share ss;
        oc::Matrix<u64> shufflePlain; // sizeetimation
        Mat2d shufflePlainadd;        // sizeetimation
        BlongShare flagrand;
        AShare flagshare;
        std::vector<SmallShare> valueshare;
        // AShare shuffleIndex;
        AShare pieInv;
        AShare indexPool;
        oc::Matrix<u8> mFlag;
        // Matrix<u64> values;

        AShare idflag; // result of id intersection
        AddShare idflagadd;
        size_t c;   // result of size estimation
        AShare *fs; // result of feature alignment
        AShare f;
        AddShare fadd;
        AShare data; // result of data selection
        Mat2d dadd;
        std::string data_file;

        size_t shu_method;
        int srow;
        int scol;
        bool offline = 0;

        bool afterpsi = false; // record finishing psi process
        // std::vector<coproto::Socket> psocks; // connection psocks
        // int *sock_map;                      // i(party) ---> sock_id

        // ShareOp *shareop = new ShareOp(); // process secret share operation

    public:
        MulPSIOkvs mokvs;
        Party();
        ~Party() = default;
        Party(size_t size, std::vector<block> mset, MatrixView<u8> mvalues); // has data

        void setValues(std::vector<u8> mvalues);
        void setSet(std::vector<block> mset);
        void setkvs(std::vector<block> mset, std::vector<u8> mvalues);
        // std::vector<u8> getValues() { return values; };
        std::vector<block> getSet() { return mset; };

        void init(const CLP &cmd); // using cmd to init data and network

        void testShare();
        void testAddShare();

    private:
        void readConfig();
        void init();      // init data and network
        void configCmd(); // using info of cmd to confige param
        coproto::AsioSocket setSock(std::string ip, bool server);
        coproto::AsioSocket setSock(std::string ip, unsigned short port, bool server);
        void connect();
        void delConnect();
        void getFileData();
        void mergeF();
        Proto SyncPCo();
        void SyncP();
        virtual void printValues() = 0;

    public:
        virtual void getFakeData();
        void RecordIDSf();
        void ReadIDSf();
        void u8tou64(const Matrix<u8> &values, Matrix<u64> &result);
        void convert64(const Matrix<u8> &values, Matrix<u64> &result);
        bool thrJudge(size_t c);
        void IndexPool();
        Proto valueShareCo(size_t id);
        Proto valueShareCo();
        void valueShare();
        void printLocalData();
        void printIndexPool();
        void printValueShare();
        virtual void printMergeData();
        virtual void writeMergeData();
        virtual void printHash() = 0;
        // void GenIndex();
        virtual void HashingInputID() = 0;
        virtual void HashingInputF() = 0;
        virtual int64_t IdIntersection() = 0;
        virtual int64_t IdIntersectionDynamic() = 0;
        virtual int64_t IdIntersectionRing() = 0;
        int64_t SizeEstimation();
        virtual int64_t FeatureAlign() = 0;
        // virtual int64_t FeatureAlignRing() = 0;
        int64_t BaseFeatureAlign();
        int64_t DataSelection();
        int64_t DataSelection2();
        int64_t ExpShuffle();
        int64_t ExpShuffleNew();
        int64_t ExpMul();
        int64_t ExpMulWithoutOpt();
        void SecVPre();
        // virtual void mulCircuitPSI() = 0;
        virtual void testPSI() = 0;

        size_t CommSize();
        size_t SingleCommSize();
    };

}
#endif