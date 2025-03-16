#include "MulPSIShare.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <bitset>

using namespace std;

namespace volePSI
{
    extern size_t nParties;
    extern size_t myIdx;
 
    // void convert64(const Matrix<u8> &values, Matrix<u64> &result)
    // {
    //     // Matrix<u64> result;
    //     result.resize(values.rows(), 1);
    //     // Matrix<u8>::iterator u8it = values.begin();
    //     // Matrix<u64>::iterator u64it = result.begin();
    //     u64 col = values.cols();
    //     u64 len = col > 8 ? 8 : col;
    //     u64 temp;
    //     // u64 col = values.cols();
    //     // std::cout << len << std::endl;
    //     for (u64 i = 0; i < values.rows(); ++i)
    //     {
    //         temp = 0;
    //         for (u64 j = 0; j < len; j++)
    //         {
    //             // std::cout << std::bitset<8>(values(i, j)) << " ";
    //             temp = (temp << 8) + values(i, j);
    //             // std::cout << std::bitset<8>(temp) << "  | ";
    //         }
    //         result(i, 0) = temp;
    //         // std::cout << temp << std::endl;
    //     }
    //     // ashare.setAshare(result);
    // }

}