#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

class OscSender {
public:
    OscSender(const std::string& ip, int port) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);

        sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons((u_short)port);
        inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
    }

    ~OscSender() {
        if (sock_ != INVALID_SOCKET) closesocket(sock_);
        WSACleanup();
    }

    void sendInt(const std::string& address, int32_t value) {
        std::vector<uint8_t> buf;
        appendPaddedString(buf, address);
        appendPaddedString(buf, ",i");
        appendInt32BE(buf, value);

        sendto(sock_, (const char*)buf.data(), (int)buf.size(), 0,
               (sockaddr*)&addr_, sizeof(addr_));
    }

    void sendFloat(const std::string& address, float value) {
        std::vector<uint8_t> buf;
        appendPaddedString(buf, address);
        appendPaddedString(buf, ",f");
        appendFloat32BE(buf, value);

        sendto(sock_, (const char*)buf.data(), (int)buf.size(), 0,
               (sockaddr*)&addr_, sizeof(addr_));
    }

private:
    static void appendPaddedString(std::vector<uint8_t>& buf, const std::string& s) {
        size_t start = buf.size();
        buf.insert(buf.end(), s.begin(), s.end());
        buf.push_back('\0');
        while ((buf.size() - start) % 4 != 0) buf.push_back('\0');
    }

    static void appendInt32BE(std::vector<uint8_t>& buf, int32_t v) {
        uint32_t u = (uint32_t)v;
        buf.push_back((u >> 24) & 0xFF);
        buf.push_back((u >> 16) & 0xFF);
        buf.push_back((u >> 8) & 0xFF);
        buf.push_back(u & 0xFF);
    }

    static void appendFloat32BE(std::vector<uint8_t>& buf, float f) {
        uint32_t u;
        static_assert(sizeof(float) == 4, "float must be 4 bytes");
        std::memcpy(&u, &f, 4);
        buf.push_back((u >> 24) & 0xFF);
        buf.push_back((u >> 16) & 0xFF);
        buf.push_back((u >> 8) & 0xFF);
        buf.push_back(u & 0xFF);
    }

    SOCKET sock_{INVALID_SOCKET};
    sockaddr_in addr_{};
};