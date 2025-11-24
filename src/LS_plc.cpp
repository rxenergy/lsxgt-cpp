#include "LS_plc.hpp"

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cstring>

bool PlcLS::winsockInitialized_ = false;

// Utility: split string by separator
namespace {
    std::vector<std::string> split(const std::string& s, char sep) {
        std::vector<std::string> result;
        std::string cur;
        for (char c : s) {
            if (c == sep) {
                result.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
        result.push_back(cur);
        return result;
    }
}

// ---------------------------------------------------------------------------
// Winsock Initialization / Constructor / Destructor
// ---------------------------------------------------------------------------

void PlcLS::ensureWinsock() {
    if (!winsockInitialized_) {
        WSADATA wsaData;
        int r = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (r != 0) throw std::runtime_error("WSAStartup failed");
        winsockInitialized_ = true;
    }
}

PlcLS::PlcLS(const std::string& host, uint16_t port)
    : host_(host), port_(port), socket_(INVALID_SOCKET) {
    ensureWinsock();
    connectInit();
}

PlcLS::~PlcLS() {
    close();
}

PlcLS::PlcLS(PlcLS&& other) noexcept
    : host_(std::move(other.host_)),
      port_(other.port_),
      socket_(other.socket_) {
    other.socket_ = INVALID_SOCKET;
}

PlcLS& PlcLS::operator=(PlcLS&& other) noexcept {
    if (this != &other) {
        close();
        host_ = std::move(other.host_);
        port_ = other.port_;
        socket_ = other.socket_;
        other.socket_ = INVALID_SOCKET;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Socket Connection
// ---------------------------------------------------------------------------

void PlcLS::connectInit() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }

    socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET)
        throw std::runtime_error("socket() failed");

    int timeout = 3000;
    setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    BOOL flag = TRUE;
    setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (::inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) != 1)
        throw std::runtime_error("inet_pton failed");

    if (::connect(socket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        throw std::runtime_error("connect() failed");
}

void PlcLS::reconnect() {
    close();
    connectInit();
}

void PlcLS::close() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

// ---------------------------------------------------------------------------
// Send / Receive
// ---------------------------------------------------------------------------

void PlcLS::sendAll(const std::vector<uint8_t>& buffer) {
    size_t sent = 0;
    while (sent < buffer.size()) {
        int r = ::send(socket_, (const char*)(buffer.data() + sent),
                       (int)(buffer.size() - sent), 0);
        if (r <= 0) throw std::runtime_error("send failed");
        sent += (size_t)r;
    }
}

std::vector<uint8_t> PlcLS::recvOnce(int maxSize) {
    std::vector<uint8_t> buf(maxSize);
    int r = ::recv(socket_, (char*)buf.data(), maxSize, 0);
    if (r <= 0) throw std::runtime_error("recv failed");
    buf.resize(r);
    return buf;
}

// ---------------------------------------------------------------------------
// XGT Frame Builder
// ---------------------------------------------------------------------------

std::vector<uint8_t> PlcLS::makeHeader(const std::string& companyId) {
    std::vector<uint8_t> header;

    if (companyId == "XGT" || companyId == "XGB") {
        const char company[] = "LSIS-XGT";
        header.insert(header.end(), company, company + 8);
        header.push_back(0x00);
        header.push_back(0x00);
    } else {
        const char company[] = "LGIS-GLOFA";
        header.insert(header.end(), company, company + 10);
    }

    header.push_back(0x00); // PLC header
    header.push_back(0x00);
    header.push_back(0x00); // CPU header
    header.push_back(0x33); // Source frame header
    header.push_back(0x00); // Invoke ID
    header.push_back(0x00);

    return header;
}

void PlcLS::updateHeader(std::vector<uint8_t>& header,
                         const std::vector<uint8_t>& appInstruction) {

    uint8_t lenLow = (uint8_t)(appInstruction.size() & 0xFF);
    header.push_back(lenLow);
    header.push_back(0x00);

    header.push_back(0x00); // FEnet header
    header.push_back(0x00); // Reserved
}

std::string PlcLS::swapPairs(const std::string& dtype,
                             const std::string& input) {

    std::string s = input;
    size_t n = s.size();
    int group = 2;

    if (dtype == "word") group = 4;
    else if (dtype == "dword") group = 8;
    else if (dtype == "lword") group = 16;

    if (n % group != 0) {
        size_t pad = group - (n % group);
        s.insert(0, pad, '0');
        n += pad;
    }

    std::string result = "2000";
    for (int i = (int)n - 3; i > 0; i -= 4) {
        result.push_back(s[i + 1]);
        result.push_back(s[i + 2]);
        result.push_back(s[i - 1]);
        result.push_back(s[i]);
    }
    return result;
}

std::vector<uint8_t> PlcLS::makeValue(const std::string& dtype,
                                      const std::string& value) {

    std::vector<uint8_t> out;

    if (dtype == "bit") {
        out.push_back(0x01);
        out.push_back(0x00);
        out.push_back((uint8_t)std::stoi(value));
        return out;
    }

    unsigned long long v = std::stoull(value);
    std::ostringstream oss;
    oss << std::hex << std::uppercase << v;
    std::string part = oss.str();

    if (part.size() % 2 == 1)
        part.insert(part.begin(), '0');

    std::string swapped = swapPairs(dtype, part);
    for (size_t i = 0; i < swapped.size(); i += 2) {
        uint8_t b = (uint8_t)std::stoi(swapped.substr(i, 2), nullptr, 16);
        out.push_back(b);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Response Parser
// ---------------------------------------------------------------------------

std::vector<uint64_t> PlcLS::parseReadValues(
    const std::vector<uint8_t>& resp,
    const std::string& headdevice) {

    std::vector<std::string> heads = split(headdevice, ',');
    int count = (int)heads.size();

    if (resp.size() <= 30) return {};

    uint8_t num = resp[30];
    if (num == 0) return {};

    std::vector<uint64_t> out;
    out.reserve(count);

    for (int i = count; i >= 1; --i) {
        uint64_t val = 0;
        for (int j = 1; j <= num; ++j) {
            int idx = (int)resp.size() - j - (i - 1) * (num + 2);
            if (idx < 0 || idx >= (int)resp.size())
                throw std::runtime_error("parse error");

            uint8_t b = resp[idx];

            int exp = 2 * (num - j);
            uint64_t mul = 1;
            for (int e = 0; e < exp; ++e) mul *= 16;

            val += (uint64_t)b * mul;
        }
        out.push_back(val);
    }

    return out;
}

// ---------------------------------------------------------------------------
// Build Instruction Block
// ---------------------------------------------------------------------------

std::vector<uint8_t> PlcLS::makeInstruction(
    const std::string& request,
    const std::string& dtypeRaw,
    const std::string& headdevice,
    const std::string& values) {

    std::vector<std::string> heads = split(headdevice, ',');
    int count = (int)heads.size();

    std::string dtype = dtypeRaw;
    std::transform(dtype.begin(), dtype.end(), dtype.begin(), ::tolower);

    std::vector<uint8_t> dtypeInst(2);
    char dtypeWord;

    if (dtype == "bit") { dtypeInst = {0x00,0x00}; dtypeWord='X'; }
    else if (dtype == "byte") { dtypeInst={0x01,0x00}; dtypeWord='B'; }
    else if (dtype == "word") { dtypeInst={0x02,0x00}; dtypeWord='W'; }
    else if (dtype == "dword") { dtypeInst={0x03,0x00}; dtypeWord='D'; }
    else if (dtype == "lword") { dtypeInst={0x04,0x00}; dtypeWord='L'; }
    else { dtypeInst={0x14,0x00}; dtypeWord='B'; }

    std::vector<uint8_t> reserved{0x00,0x00};
    std::vector<uint8_t> numInst{(uint8_t)(count & 0xFF), 0x00};

    std::vector<uint8_t> dataInst;
    for (auto& h : heads) {
        std::vector<uint8_t> di;
        di.push_back('%');
        di.push_back(h[0]);
        di.push_back(dtypeWord);

        std::vector<uint8_t> vi(h.begin() + 1, h.end());

        uint8_t len = (uint8_t)((di.size() + vi.size()) & 0xFF);
        dataInst.push_back(len);
        dataInst.push_back(0x00);

        dataInst.insert(dataInst.end(), di.begin(), di.end());
        dataInst.insert(dataInst.end(), vi.begin(), vi.end());
    }

    std::vector<uint8_t> out;

    if (request == "read") {
        out.insert(out.end(), {0x54,0x00});
        out.insert(out.end(), dtypeInst.begin(), dtypeInst.end());
        out.insert(out.end(), reserved.begin(), reserved.end());
        out.insert(out.end(), numInst.begin(), numInst.end());
        out.insert(out.end(), dataInst.begin(), dataInst.end());
    } else {
        out.insert(out.end(), {0x58,0x00});
        out.insert(out.end(), dtypeInst.begin(), dtypeInst.end());
        out.insert(out.end(), reserved.begin(), reserved.end());
        out.insert(out.end(), numInst.begin(), numInst.end());
        out.insert(out.end(), dataInst.begin(), dataInst.end());

        auto vals = split(values, ',');
        for (auto& v : vals) {
            if (v.empty()) continue;
            auto bytes = makeValue(dtype, v);
            out.insert(out.end(), bytes.begin(), bytes.end());
        }
    }

    return out;
}

// ---------------------------------------------------------------------------
// Public Command API
// ---------------------------------------------------------------------------

std::vector<uint64_t> PlcLS::command(const std::string& companyId,
                                     const std::string& request,
                                     const std::string& dtype,
                                     const std::string& headdevice,
                                     const std::string& values) {

    auto header = makeHeader(companyId);
    auto appInst = makeInstruction(request, dtype, headdevice, values);

    updateHeader(header, appInst);
    header.insert(header.end(), appInst.begin(), appInst.end());

    sendAll(header);

    auto resp = recvOnce(1024);
    if (request == "read")
        return parseReadValues(resp, headdevice);

    return {};
}
