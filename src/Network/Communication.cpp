#include "Communication.h"

volePSI::Proto SendVector(const std::vector<u8> &values, coproto::Socket &chl)
{
    MC_BEGIN(volePSI::Proto, values, chl);
    MC_AWAIT(chl.send(std::move(values)));
    MC_END();
}

volePSI::Proto RecvVector(std::vector<u8> &values, coproto::Socket &chl)
{
    MC_BEGIN(volePSI::Proto, values, chl);
    MC_AWAIT(chl.recv(values));
    std::cout << "recv" << std::endl;
    for (size_t i = 0; i < values.size(); i++)
    {
        std::cout << +values[i] << " ";
    }
    MC_END();
    std::cout << "recv" << std::endl;
    for (size_t i = 0; i < values.size(); i++)
    {
        std::cout << +values[i] << " ";
    }
}
