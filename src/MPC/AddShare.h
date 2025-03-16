#ifndef ADD_SHARE
#define ADD_SHARE

#include "cryptoTools/Common/CLP.h"
#include "vector"
#include "volePSI/Defines.h"
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Common/MatrixView.h"
#include "cryptoTools/Common/block.h"
#include "vector"
#include "cryptoTools/Crypto/PRNG.h"
#include "ShareOp.h"
#include <Eigen/Core>
#include <Eigen/Dense>
#include "../Network/EigenSocket.h"

// #define DEBUG_INFO

namespace volePSI
{

    // typedef Eigen::Matrix<oc::u64, -1, -1> Mat2d;

    class AddShare
    {
    public:
        Mat2d data;
        int size;
        int row;
        int col;

        AddShare();
        AddShare(int row, int col);
        AddShare(Mat2d data);

        void resize(int row, int col);

        volePSI::Proto RevealCo(Mat2d &result);
        volePSI::Proto RevealCo(std::string str);
        volePSI::Proto RevealCo();

        void ShuffleOFF(std::vector<AddShare> &shuffle_net);
        void ShuffleOFFTrick(std::vector<Mat2d> &mask, std::vector<Mat2d> &shumask1, std::vector<Mat2d> &shumask2);
        void ShuffleOFFTrick(std::vector<AddShare> &shuffle_net);
        void Shuffle(AddShare &result, std::vector<AddShare> &shuffle_net, bool off);
        void Shuffle(AddShare &result, std::vector<Mat2d> &mask, std::vector<Mat2d> &shumask1, std::vector<Mat2d> &shumask2, int seed);
        Mat2d Reveal();
        void RevealThr(Mat2d &result);
        void Reveal(Mat2d &result);
        void Reveal(std::string str);
        void RevealCovoid();

        void PrintSize();

        AddShare &operator*=(const AddShare &c);
        AddShare operator*(const Mat2d &c) const;
        AddShare operator+(const Mat2d &share) const;
        AddShare operator+(const AddShare &share) const;
        AddShare &operator+=(const AddShare &share);
        // AddShare operator-() const;
        AddShare operator-(const Mat2d &share) const;
        AddShare operator-(const AddShare &share) const;
        AddShare &operator-=(const AddShare &share);
    };
}

#endif