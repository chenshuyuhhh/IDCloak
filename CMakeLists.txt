cmake_minimum_required(VERSION 3.15)
project(IDCloak)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Update submodules
execute_process(COMMAND git submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Include directories
# include_directories(include)

find_package(OpenSSL REQUIRED)

# volePSI
execute_process(COMMAND python3 build.py -DVOLE_PSI_ENABLE_BOOST=ON -DVOLE_PSI_ENABLE_OPPRF=ON -DCMAKE_CXX_STANDARD=20 -DVOLE_PSI_ENABLE_RELIC=ON
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libs/volePSI)


# default location for miracl
# set(Miracl_Dirs "${CMAKE_SOURCE_DIR}/thirdparty/linux/miracl")
# include_directories("${Miracl_Dirs}") 
# link_directories("${Miracl_Dirs}/miracl/source/")

# message(STATUS "Miracl library directory: ${Miracl_Dirs}/miracl/source/")

# default location for NTL
# set(NTL_Dirs "${CMAKE_SOURCE_DIR}/thirdparty/linux/ntl")
# include_directories("${NTL_Dirs}/include/")
# link_directories("${NTL_Dirs}/src/") 

# message(STATUS "NTL library directory: ${NTL_Dirs}/src/")

add_executable(IDCloak
    src/main.cpp
    src/Party.cpp
    src/PartyL.cpp
    src/PartyN.cpp
    src/MulPSILeader.cpp
    src/MulPSINormal.cpp
    src/MulPSIOkvs.h
    src/MulPSIShare.cpp
    src/RsMulpsi.cpp
    src/MPC/ShareOp.cpp
    src/MPC/SmallShare.cpp
    src/MPC/BlongShare.cpp
    src/MPC/AddShare.cpp
    src/Network/EigenSocket.cpp
    src/Network/CoproSocket.cpp
)

set(volePSI_DIR libs/volePSI/)
find_package(volePSI HINTS ${volePSI_DIR} REQUIRED)

# Link libraries
target_link_libraries(IDCloak PRIVATE visa::volePSI OpenSSL::SSL OpenSSL::Crypto)

# target_include_directories(ucpsi PUBLIC ${MultipartyPSI_DIR})
target_include_directories(IDCloak PUBLIC ${volePSI_DIR})