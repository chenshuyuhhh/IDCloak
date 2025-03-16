#ifndef SHARE_OP
#define SHARE_OP

#define OFFLINE false

#include "volePSI/Defines.h"
#include "cryptoTools/Network/Channel.h"
#include "coproto/Socket/AsioSocket.h"
#include "cryptoTools/Common/MatrixView.h"
#include "cryptoTools/Common/block.h"
#include "vector"
#include "cryptoTools/Crypto/PRNG.h"
#include <set>
// #define DEBUG_INFO

namespace volePSI
{
    extern std::vector<coproto::AsioSocket> psocks; // connection psocks
    extern size_t nParties;                     // the party num
    extern size_t myIdx;                        // the number of party
    extern size_t lIdx;                         // the number of lead party
    extern bool normal;                         // leader party(0), normal party(1)
    extern PRNG pPrng;
    // class Share
    // {
    //     // std::vector<oc::u8> shares;
    //     // size_t size;

    // public:
    //     // ShareOp();
    // };
    class AShare;

    const int trustp = 0; // using party 0 to monitor
    class BoolShare
    {
    private:
        oc::BitVector bshare;
        u64 size;
        Proto Reveal();

    public:
        BoolShare();
        BoolShare(u64 size);
        // std::vector<oc::u8> to8u();
        BoolShare(std::string bshare);
        BoolShare(oc::BitVector bshare);
        void resize(u64 newSize, u8 val);
        void SetShare(oc::BitVector bshare);
        oc::BitVector GetShare() const { return bshare; };
        Proto GetRand(oc::BitVector &result) const;
        // u64 getSize() { return size; }

        Proto Convert2ACo(AShare &ashare);
        Proto AndReveal(const BoolShare &bshare2, BoolShare &result) const;

        void GenShare(const oc::BitVector share, std::vector<oc::BitVector> &shares) const;
        // and(BoolShare bshare);
        void RevealPrint();
        void Print();

        void Convert2A(AShare &ashare);

        oc::BitVector orBitVec(const oc::BitVector &c) const;

        BoolShare operator&(const oc::BitVector &c) const;
        BoolShare operator&(const BoolShare &y) const;

        BoolShare operator^(const oc::BitVector &c) const;
        BoolShare operator^(const BoolShare &c) const;
        BoolShare operator~() const;
        bool operator[](const u64 i) const;
    };

    class AShare
    {
    private:
        oc::Matrix<u64> ashare;
        u64 size;
        u64 col, row;

        Proto SecretShareCo(size_t id);
        Proto RevealCo(oc::Matrix<u64> &plain);
        Proto RevealCo();
        // Proto EqualZeroCo(oc::Matrix<bool> &result);
        Proto BiggerPlainCo(u64 c, oc::Matrix<bool> &result);
        Proto ShuffleOFFCo(AShare &r, AShare &pie, AShare &pie_inv, AShare &pier);
        Proto ShuffleOnCo(AShare &result, const AShare &r, const AShare &pie, const AShare &pier);
        Proto ShuffleCo(AShare &result, AShare &pie_inv);
        void ShuffleDimOFF(AShare &r, AShare &pie, AShare &pie_inv, AShare &pier);
        void ShuffleNetOFF(AShare &r, AShare &pier, AShare &pie_net);
        Proto SecretShareSizeCo(size_t id, std::set<size_t> ids);
        Proto SecretShareSizeCo(size_t id, std::vector<u64> &info);
        Proto SecretShareCo(size_t id, std::set<size_t> ids);

    public:
        AShare(){};
        AShare(u64 rows, u64 columns, oc::AllocType type = oc::AllocType::Zeroed)
        {
            ashare.resize(rows, columns, type);
            row = rows;
            col = columns;
            size = ashare.size();
        };

        AShare(u64 size, oc::AllocType type = oc::AllocType::Zeroed)
        {
            row = size;
            col = 1;
            ashare.resize(size, 1, type);
            size = ashare.size();
        };
        AShare(const oc::Matrix<u64> ashare);
        void setAshare(const oc::Matrix<u64> ashare);
        oc::Matrix<u64> getShare();
        void resize(u64 rows, u64 columns, oc::AllocType type = oc::AllocType::Zeroed)
        {
            ashare.resize(rows, columns, type);
            row = rows;
            col = columns;
            size = ashare.size();
        }

        u64 getSize() const{ return size; }
        u64 rows() const { return row; }
        u64 cols() const { return col; }

        void Reveal(oc::Matrix<u64> &plain);
        void RevealPrint();
        void Print();

        void GenSecrets(std::vector<oc::Matrix<u64>> &secrets);
        void GenSecrets(std::vector<oc::Matrix<u64>> &secrets, size_t n);
        void SecretShare(size_t id);
        void SecretShareSize(size_t id, std::set<size_t> ids);
        void SecretShare(size_t id, std::set<size_t> ids);
        void SecretShareSize(size_t id);
        void ShuffleDim(AShare &result, AShare &pie_inv);
        void ShuffleNetOFF(std::vector<AShare> &pie);
        void ShuffleNet(AShare &result, std::vector<AShare> &pie);
        void DeShuffleNet(AShare &result, std::vector<AShare> &pie);
        // void ToShuffle(AShare &result, const AShare plain);
        void ToShuffle(AShare &result, const oc::Matrix<u64> plain);
        // void ToShuffleInv(AShare &result, const AShare plain);
        void ToShuffleInv(AShare &result, const oc::Matrix<u64> plain);

        u64 operator()(u64 i) const;
        AShare operator-(const AShare &share) const;
        AShare operator+(const AShare &share) const;
        AShare &operator+=(const AShare &share);
        AShare &operator-=(const AShare &share);
        AShare operator*(const oc::Matrix<u64> &c) const;
        AShare operator*(const AShare &c) const;
        AShare &operator*=(const AShare &c);
        void DotProduct(AShare &result, const AShare &c);

        void ShuffleOFF(AShare &r, AShare &pie, AShare &pie_inv, AShare &pier);
        void EqualZero(AShare &result);
        // void EqualZero(BoolShare &result);
        // judge weather every element of ashare is bigger than c
        void BiggerPlain(u64 c, oc::Matrix<bool> &result);
        void Sum(AShare &result);
        void ShuffleOn(AShare &result, const AShare &r, const AShare &pie, const AShare &pier);
        void Shuffle(AShare &result, AShare &pieinv);
    };
}

#endif