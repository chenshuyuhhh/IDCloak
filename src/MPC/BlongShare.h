#ifndef BOOL_LONG_SHARE
#define BOOL_LONG_SHARE

#include "volePSI/Defines.h"
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Common/MatrixView.h"
#include "cryptoTools/Common/block.h"
#include "vector"
#include "cryptoTools/Crypto/PRNG.h"
#include "ShareOp.h"

namespace volePSI
{
    extern std::vector<coproto::AsioSocket> psocks; // connection psocks
    extern size_t nParties;                     // the party num
    extern size_t myIdx;                           // the number of party
    extern size_t lIdx;                            // the number of lead party
    extern bool normal;                         // leader party(0), normal party(1)
    extern PRNG pPrng;

    class BlongShare
    {
    private:
        oc::Matrix<u64> bshare; // bool share
        u64 size;
        u64 col, row;

    public:
        BlongShare();
        BlongShare(oc::Matrix<u64> bshare);
        void SetShare(oc::Matrix<u64> bshare);
        oc::Matrix<u64> GetShare()const {return bshare;}

        void AndBit(BlongShare share, size_t bit);
        const u64& operator()(u64 rowIdx, u64 colIdx) const;
        BlongShare operator-() const;
        BlongShare operator~() const;
        // BlongShare operator+(const BlongShare share2) const;
        BlongShare operator++() const;

        // Proto RevealCo(oc::Matrix<u8> &plain);
        Proto RevealCo();
        // void Reveal(oc::Matrix<u8> &plain);
        void RevealPrint();
        // void Print();
        BoolShare MSB();
        void MSB(BoolShare &result);
        void isZero(AShare &ashare);
    };
}

#endif