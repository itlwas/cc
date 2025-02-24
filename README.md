# 🚀 cc - Ultimate, Ultra-Optimized Cat Replacement

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/itlwas/cc/actions)
[![License](https://img.shields.io/badge/license-Unlicense-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.1-blue.svg)](https://github.com/itlwas/cc/releases)
[![Stars](https://img.shields.io/github/stars/itlwas/cc.svg)](https://github.com/itlwas/cc/stargazers)
[![Forks](https://img.shields.io/github/forks/itlwas/cc.svg)](https://github.com/itlwas/cc/network)

---

## 🌟 Overview

**cc** is an ultra-optimized, resource-efficient replacement for the classic Unix `cat` command. Designed with performance in mind, **cc** provides enhanced text processing capabilities and additional features that experienced developers have long desired, such as follow mode for real-time file updates.

Whether you need to quickly concatenate files or monitor log files in real time, **cc** delivers high speed, minimal CPU and memory usage, and a suite of options to tailor its behavior to your workflow.

> **Note:** This project is solely a personal experiment by [itlwas](https://github.com/itlwas) and is not intended for continuous development. It serves as a proof-of-concept rather than a fully evolving product.

---

## ✨ Features

- **Fast Raw Output:** Directly output file content with minimal overhead.
- **Enhanced Text Formatting:**
  - **Line Numbering:** Number all lines (`-n`) or only nonblank lines (`-b`).
  - **Blank Line Squeezing:** Suppress repeated blank lines (`-s`).
  - **End-of-Line Markers:** Append markers at the end of each line (`-e`).
  - **Tab Visualization:** Display TAB characters as `^I` (`-T`).
  - **Nonprinting Characters:** Convert nonprinting characters to a readable format (`-v`).
  - **Combined Flag:** `-A` is equivalent to `-v -T -e`.
- **Follow Mode:** Continuously output appended data in real time (similar to `tail -f`) with the `-f` flag.
- **Memory Mapping:** Uses memory mapping for files larger than 1MB to minimize data copying and boost performance.
- **Optimized Resource Usage:** Minimal allocations and efficient data processing for extremely large files.

---

## 📖 Usage

### Basic Examples

- **Concatenate and Display a File:**
  ```bash
  ./cc file.txt
  ```

- **Display a File with Line Numbers and Squeeze Blank Lines:**
  ```bash
  ./cc -n -s file.txt
  ```

- **Monitor a Log File in Real Time:**
  ```bash
  ./cc -f logfile.log
  ```

- **Enhanced Formatting with All Transformations:**
  ```bash
  ./cc -A file.txt
  ```

---

## 🔧 Installation & Compilation

### Prerequisites

- A C compiler supporting C99 (or later) such as `gcc` or `clang`.
- Standard development tools on your platform (e.g., GNU Make, if using a Makefile).

### Building from Source

1. **Clone the Repository:**
   ```bash
   git clone https://github.com/itlwas/cc.git
   cd cc
   ```

2. **Compile the Code:**
   For maximum optimization and resource efficiency, compile with the following flags:
   ```bash
   gcc -O3 -march=native -fdata-sections -pthread -ffunction-sections -Wl,--gc-sections -s cc.c -o cc
   ```
   On Windows, adjust the command accordingly to produce `cc.exe`.

3. **Run the Application:**
   ```bash
   ./cc [OPTIONS] [FILE]...
   ```

---

## 📄 License

This project is released into the public domain under the terms of the **Unlicense**.  
See the [LICENSE](LICENSE) file for details.

---

Made with ❤️ for Windows command-line enthusiasts. Enjoy!
