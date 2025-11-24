#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <string>
#include <vector>

class PlcLS {
public:
    PlcLS(const std::string& host, uint16_t port);
    ~PlcLS();

    // Non-copyable, movable
    PlcLS(const PlcLS&) = delete;
    PlcLS& operator=(const PlcLS&) = delete;

    PlcLS(PlcLS&& other) noexcept;
    PlcLS& operator=(PlcLS&& other) noexcept;

    /**
     * @brief Send a request to LS XGT PLC.
     *
     * @param companyId "XGT", "XGB", etc.
     * @param request   "read" or "write"
     * @param dtype     "bit", "byte", "word", "dword", "lword"
     * @param headdevice PLC address such as "M00510" or "M00510,M00511"
     * @param values     Write values such as "1", "10,20"
     *
     * @return For "read": vector of parsed values.
     *         For "write": empty vector.
     */
    std::vector<uint64_t> command(const std::string& companyId,
                                  const std::string& request,
                                  const std::string& dtype,
                                  const std::string& headdevice,
                                  const std::string& values = "0");

    /// @brief Reconnect to PLC server.
    void reconnect();

    /// @brief Close the socket.
    void close();

private:
    // Initialize Winsock only once in the process.
    static void ensureWinsock();

    // Create and connect a socket.
    void connectInit();

    // Send the entire buffer.
    void sendAll(const std::vector<uint8_t>& buffer);

    // Receive up to maxSize bytes.
    std::vector<uint8_t> recvOnce(int maxSize);

    // Build XGT frame header.
    static std::vector<uint8_t> makeHeader(const std::string& companyId);

    // Append frame length and reserved headers.
    static void updateHeader(std::vector<uint8_t>& header,
                             const std::vector<uint8_t>& appInstruction);

    // Build application instruction block.
    static std::vector<uint8_t> makeInstruction(const std::string& request,
                                                const std::string& dtypeRaw,
                                                const std::string& headdevice,
                                                const std::string& values);

    // Convert write value according to XGT format.
    static std::vector<uint8_t> makeValue(const std::string& dtype,
                                          const std::string& value);

    // Swap byte/word pairs according to XGT endian rule.
    static std::string swapPairs(const std::string& dtype,
                                 const std::string& input);

    // Parse PLC response for read command.
    static std::vector<uint64_t> parseReadValues(const std::vector<uint8_t>& response,
                                                 const std::string& headdevice);

private:
    std::string host_;
    uint16_t port_;
    SOCKET socket_;

    static bool winsockInitialized_;
};
