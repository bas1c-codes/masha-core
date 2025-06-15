# 🛡️ Masha Antivirus

**Masha** is a lightweight, C++-based antivirus engine built from scratch. It currently supports signature-based detection using SHA-256 hashes and provides a simple CLI interface to scan files for known malware. The project is still in early development.

## ⚠️ Disclaimer

**Masha is under active development and is not intended for real-world production use yet. It is currently a research and learning project.So please give early feedbacks and suggestions about the code**

## 🚀 Features

- Signature-based detection using SHA-256
- Modular core engine written in modern C++
- Built with CMake and CLI11
- Simple command-line interface

## 📦 Project Structure

core/       # Core antivirus engine 
cli/        # CLI for interacting with core
kernel-driver/ # Code for minifilter driver for realtime scanning
CMakeLists.txt # Build file

## 🔧 Requirements

- C++17 or higher
- CMake 3.14+
- CLI11 (can be included or fetched)

## 🧪 Usage
**Hash file link** - https://drive.google.com/file/d/1gE14XaqJn3kaQv8YDrG9dObdnsCX2jD_/view?usp=sharing
### Build the project:
```bash
mkdir build
cd build
cmake --build .
```
Scan a file:

`.\build\cli\cli.exe -s <path_to_file>`

📋 Example

`.\Debug\cli.exe -s C:\Users\You\Desktop\suspect.exe`

If the file’s SHA-256 hash matches a known malware signature, it will be flagged as MALWARE.

## 🔍 How It Works

Loads SHA-256 hashes from a file/database.

Computes the SHA-256 of the scanned file.

Compares it with known signatures.

Reports if malware is detected.


## 🛠️ Planned Features

#Quarantine engine

YARA-based detection

Real-time scanning

Sandbox-based detection


## 🧠 Why Masha?

Masha is designed to be:

Lightweight

Open-source

Easy to understand and extend


## 📄 License

MIT License

## 🙋‍♂️ Contributing

Contributions, suggestions, and improvements are welcome! Feel free to fork the project and open a pull request.

## 🔎 SEO Tags

C++ antivirus, open source antivirus, signature-based malware scanner, lightweight AV engine, sha256 antivirus, YARA C++, sandbox antivirus, real-time file scanner, quarantine malware, cli antivirus, hash-based detection, Masha antivirus
