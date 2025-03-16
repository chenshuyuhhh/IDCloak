#pragma once

#include "volePSI/Defines.h"
#include "cryptoTools/Network/IOService.h"
#include "cryptoTools/Network/Channel.h"
#include <vector>

using namespace oc;

volePSI::Proto SendVector(const std::vector<u8> &values, coproto::Socket &chl);
volePSI::Proto RecvVector(std::vector<u8> &values, coproto::Socket &chl);