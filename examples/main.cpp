#include <iostream>
#include "LS_plc.hpp"

int main()
{
    try
    {
        // ----------------------------------------
        // 1) Connect to PLC
        // ----------------------------------------
        PlcLS plc("192.168.1.1", 2004);  
        std::cout << "[INFO] Connected to LS XGT PLC\n";

        // ----------------------------------------
        // 2) Read a BIT (M00510)
        // ----------------------------------------
        auto bitValue = plc.command(
            "XGT",      // PLC family
            "read",     // Read request
            "bit",      // Data type
            "M00510"    // Address
        );

        std::cout << "[READ] M00510 = " << bitValue[0] << std::endl;

        // ----------------------------------------
        // 3) Write a BIT (M00510 = 1)
        // ----------------------------------------
        plc.command(
            "XGT",
            "write",
            "bit",
            "M00510",
            "1"
        );
        std::cout << "[WRITE] M00510 <- 1\n";

        // ----------------------------------------
        // 4) Read multiple WORDs
        // ----------------------------------------
        auto words = plc.command(
            "XGT",
            "read",
            "word",
            "D00010,D00012,D00014"
        );

        std::cout << "[READ] WORD values:\n";
        for (size_t i = 0; i < words.size(); i++)
        {
            std::cout << "  value[" << i << "] = " << words[i] << "\n";
        }

        // ----------------------------------------
        // 5) Write multiple WORDs
        // ----------------------------------------
        plc.command(
            "XGT",
            "write",
            "word",
            "D00010,D00012,D00014",
            "100,200,300"
        );
        std::cout << "[WRITE] D00010,D00012,D00014 <- 100,200,300\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
