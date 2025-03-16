#include "BlongShare.h"
#include "bitset"

namespace volePSI
{
    BlongShare::BlongShare()
    {
    }

    BlongShare::BlongShare(oc::Matrix<u64> bshare)
    {
        this->bshare = bshare;
        this->size = bshare.size();
        this->col = bshare.cols();
        this->row = bshare.rows();
    }

    void BlongShare::SetShare(oc::Matrix<u64> bshare)
    {
        this->bshare = bshare;
        this->size = bshare.size();
        this->col = bshare.cols();
        this->row = bshare.rows();
    }

    Proto BlongShare::RevealCo()
    {
        MC_BEGIN(Proto, this, p = myIdx, socks = psocks, i = int{}, size = this->bshare.size(), recvdata = std::vector<oc::Matrix<u64>>{});
        // std::cout << p << std::endl;
        recvdata.resize(nParties);
        for (i = 0; i < nParties; i++)
        {
            if (i != p)
            {
                MC_AWAIT(socks[i].send(bshare));
                recvdata[i].resize(row, col);
                MC_AWAIT(socks[i].recv(recvdata[i]));
                // std::cout << "i " << i << ": ";
            }
        }
        // reveal
        recvdata[p] = oc::Matrix<u64>(bshare);
        for (i = 1; i < nParties; i++)
        {
            for (int j = 0; j < size; j++)
            {
                recvdata[0](j) ^= recvdata[i](j);
            }
            // std::cout << "i " << i << " ";
        }

        if (recvdata[0].cols() == 1)
        {
            std::cout << "size " << size << ":\n";
            for (int j = 0; j < recvdata[0].size(); j++)
            {
                std::cout << std::bitset<64>(recvdata[0](j)) << " " << recvdata[0](j) << "\n";
            }
            std::cout << std::endl;
        }
        else
        {
            for (int i = 0; i < recvdata[0].rows(); i++)
            {
                for (int j = 0; j < recvdata[0].cols(); j++)
                {
                    std::cout << std::bitset<64>(recvdata[0](i, j)) << " ";
                }
                std::cout << std::endl;
            }
        }
        MC_END();
    }

    void BlongShare::RevealPrint()
    {
        macoro::sync_wait(RevealCo());
    }

    BlongShare BlongShare::operator~() const
    {
        if (myIdx == 0)
        {
            oc::Matrix<u64> result(row, col);
            for (size_t i = 0; i < row; i++)
            {
                for (size_t j = 0; j < col; j++)
                {
                    result(i, j) = ~bshare(i, j);
                }
            }
            return BlongShare(result);
        }
        else
        {
            return BlongShare(bshare);
        }
    }

    void BlongShare::AndBit(BlongShare share, size_t bit)
    {
    }

    const u64 &BlongShare::operator()(u64 rowIdx, u64 colIdx) const
    {
        return bshare(rowIdx, colIdx);
    }

    // BlongShare BlongShare::operator+(const BlongShare share2) const
    // {
    //     oc::Matrix<u64> result = share2.GetShare();
    //     for (size_t i = 0; i < row; i++)
    //     {
    //         for (size_t j = 0; j < col; j++)
    //         {
    //             std::cout << i << " " << j << "\n"
    //                       << std::bitset<64>(result(i, j)) << " " << result(i, j) << "\n"
    //                       << std::bitset<64>(bshare(i, j)) << " " << bshare(i, j) << "\n";
    //             result(i, j) += bshare(i, j);
    //         }
    //     }
    //     return BlongShare(result);
    // }

    BlongShare BlongShare::operator++() const
    {
        oc::Matrix<u64> result(row, col);
        oc::Matrix<u64> bits(bshare);

        // lowest
        u64 mask = 1ULL;
        std::string temp = "";
        for (u64 j = 0; j < bshare.size(); j++)
        {
            temp += (bshare(j) & mask) ? "1" : "0";
        }
        oc::BitVector bv(temp);
        // std::cout << 0 << ":" << temp << std::endl;
        BoolShare andshare(bv);
        BoolShare orshare(~bv); // x xor 1 = ~x
        // std::cout << "orshare: ";
        // orshare.RevealPrint();
        // std::cout << "andshare: ";
        // andshare.RevealPrint();

        // split
        for (u64 j = 0; j < bshare.size(); j++)
        {
            if (orshare[j])
                result(j) += mask;
            // result(j) += orshare[j] ? mask : 0;
        }

        BoolShare headshare;
        for (u64 i = 1; i < 64; i++)
        {
            mask <<= 1;
            // merge
            temp = "";
            for (u64 j = 0; j < bshare.size(); j++)
            {
                temp += (bshare(j) & mask) ? "1" : "0";
            }
            // std::cout << i << ":" << temp << " " << mask << std::endl;
            headshare.SetShare(temp);

            orshare = andshare ^ headshare;
            andshare = andshare & headshare;
            // std::cout << "orshare: ";
            // orshare.RevealPrint();
            // std::cout << "andshare: ";
            // andshare.RevealPrint();

            // split
            for (u64 j = 0; j < bshare.size(); j++)
            {
                result(j) += orshare[j] ? mask : 0;
            }
            // mask <<= 1;
        }
        return BlongShare(result);
    }

    BlongShare BlongShare::operator-() const
    {
        // std::cout << "minus\n";
        // BlongShare temp = ~*this;
        // std::cout << "~\n";
        // temp.RevealPrint();
        // std::cout << "++\n";
        // temp = ++temp;
        // temp.RevealPrint();
        // -x = ~x + 1
        return ++(~*this);
    }

    BoolShare BlongShare::MSB()
    {
        u64 mask = 1L << 63;
        std::string str = "";
        for (u64 j = 0; j < bshare.size(); j++)
        {
            str += bshare(j) & mask ? '1' : '0';
        }
        return BoolShare(str);
    }
    void BlongShare::MSB(BoolShare &result)
    {
        u64 mask = 1L << 63;
        std::string str = "";
        for (u64 j = 0; j < bshare.size(); j++)
        {
            str += bshare(j) & mask ? '1' : '0';
        }
        result = BoolShare(str);
    }

    void BlongShare::isZero(AShare &ashare)
    {
        BlongShare neg = -*this;
        BoolShare result = ~(neg.MSB() ^ this->MSB());
        // result.RevealPrint();
        result.Convert2A(ashare);
        // std::cout << "-\n";
        // neg.RevealPrint();
    }
}