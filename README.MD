# Cleaner — Enterprise-Grade Layered C System

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![C Standard](https://img.shields.io/badge/C-17-blue)
![Python](https://img.shields.io/badge/Python-3.14-yellowgreen)

---

## 🏗 Overview

**Cleaner** is a **highly disciplined, enterprise-grade C project** following strict architectural principles and quality standards. Its design ensures:

- Deterministic core logic
- Clear separation between **Core**, **System**, and **Platform** layers
- Optional static analysis, sanitizers, and automated formatting
- Enforced architecture using **AST-based Python checks**
- Fully automated quality gates

This project is suitable as a **reference for best practices** in professional C development.

---

## 📐 Architecture Principles

- **Core Layer**:  
  - Pure, deterministic C logic  
  - No OS/Platform dependencies
- **System Layer**:  
  - Orchestrates Core  
  - Uses Platform abstractions only  
- **Platform Layer**:  
  - Wraps OS-specific APIs  
  - No business logic
- **Executable Layer**:  
  - Links only System  
  - System links Core + Platform

> ✅ Tests are separated into **unit**, **integration**, and **benchmark** categories.

---

### 1️⃣ Prerequisites

- **CMake ≥ 3.25**
- **MinGW** on Windows
- **Python ≥ 3.14** (for AST checks)
- Optional: `clang-format`, `clang-tidy`, `cppcheck`, `gcovr`, `Doxygen`

### 2️⃣ Configure Project

### bash
    cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

### ⚙️ Build & Tooling
    cmake --build build

### Architecture Enforcement
    cmake --build build --target layer_checks

### Quality & Analysis Targets
    The project includes 10 enterprise-quality targets:

    Target	Description
    layer_checks	AST-based architecture enforcement
    format	Auto-format source using clang-format
    format_check	Check formatting compliance
    static_analysis	Run clang-tidy with full compile commands
    cppcheck	Run static analysis via cppcheck
    docs	Generate documentation with Doxygen
    coverage	Generate test coverage reports via gcovr
    graph	Generate CMake dependency graph (Graphviz)
    distclean	Clean build artifacts
    all_quality	Run all quality gates: layer_checks, format_check, static_analysis, cppcheck

    Use cmake --build build --target all_quality to enforce full project quality in one command.

###  Testing
    If ENABLE_TESTING is ON:

    cmake --build build --target run_tests

###  Installation
    cmake --install build

    Installs:

    Executable → bin/

    Static library → lib/

    Headers → include/