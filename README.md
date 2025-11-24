# LS XGT Protocol C++ Driver

A modern and easy-to-use C++ implementation of the **LS Electric XGT FEnet protocol**.  
This library provides read/write access to LS PLC memory registers using TCP (Winsock2).

Licensed under **MIT License** — free for personal and commercial use.

---

## ✨ Features

- ✔ Supports XGT / XGB PLCs  
- ✔ Bit / Byte / Word / DWord / LWord read & write  
- ✔ Multiple headdevices in one command  
- ✔ Pure C++17, no external dependencies  
- ✔ Clean API — easy to integrate into your PLC control programs  
- ✔ Robust TCP implementation (Winsock2)

---

## 📁 Project Structure

```
lsxgt_protocol/
│
├── include/
│   └── LS_plc.hpp
│
├── src/
│   └── LS_plc.cpp
│
└── examples/
    └── main.cpp
```

---

## 🔧 Requirements

- Windows  
- C++17 or later  
- Winsock2 (Windows SDK)

---

## 🛠️ How to Build (Without CMake)

You can compile using **MinGW g++** or **MSVC**.

### ▶ Build with MinGW

```bash
g++ -std=c++17 -O2 examples/main.cpp src/LS_plc.cpp -Iinclude -lws2_32 -o main.exe
```

### ▶ Build with MSVC (Developer Command Prompt)

```bat
cl /std:c++17 /EHsc examples\main.cpp src\LS_plc.cpp /I include ws2_32.lib
```

---

# 🚀 Example Code — `examples/main.cpp`

```cpp
#include <iostream>
#include "LS_plc.hpp"

int main()
{
    try
    {
        // -----------------------------------------------------
        // 1) Connect to PLC
        // -----------------------------------------------------
        PlcLS plc("192.168.1.1", 2004);
        std::cout << "[INFO] Connected to LS XGT PLC\n";

        // -----------------------------------------------------
        // 2) Read a BIT (M00510)
        // -----------------------------------------------------
        auto bitValue = plc.command(
            "XGT",
            "read",
            "bit",
            "M00510"
        );

        std::cout << "[READ] M00510 = " << bitValue[0] << std::endl;

        // -----------------------------------------------------
        // 3) Write a BIT (M00510 = 1)
        // -----------------------------------------------------
        plc.command(
            "XGT",
            "write",
            "bit",
            "M00510",
            "1"
        );
        std::cout << "[WRITE] M00510 <- 1\n";

        // -----------------------------------------------------
        // 4) Read multiple WORDs
        // -----------------------------------------------------
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

        // -----------------------------------------------------
        // 5) Write multiple WORDs
        // -----------------------------------------------------
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
```

---

## 📌 API Summary

### `command(companyId, request, dtype, headdevice, values)`

| Parameter    | Description                                  |
|--------------|----------------------------------------------|
| `companyId`  | `"XGT"` or `"XGB"`                           |
| `request`    | `"read"` or `"write"`                        |
| `dtype`      | `"bit"`, `"byte"`, `"word"`, `"dword"`, `"lword"` |
| `headdevice` | e.g., `"M00510"`, `"D00010,D00012"`          |
| `values`     | For write requests: `"1"`, `"10,20"`, etc.   |

---

## 📝 License

MIT License — free for personal and commercial use.

---

## 🤝 Contributing

PRs and issues are welcome.  
Feel free to extend the driver: block read, IO monitoring, jog/homing helper classes, etc.
