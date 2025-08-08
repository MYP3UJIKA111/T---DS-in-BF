<!-- markdownlint-disable MD022 MD012 MD032 MD031 MD047-->

# BinaryList Project

## Overview
This project implements a **doubly linked list** stored in a binary file, providing persistent storage for data of various types (`int`, `std::string`, and a custom `Person` structure). The implementation uses C++ templates to support generic types, with a specialized version for `std::string` to handle variable-length data. The program includes an interactive console menu to perform operations like adding, removing, sorting, and iterating over elements.

## Features
- **Generic Template (`BinaryList<T>`)**: Supports fixed-size POD types (e.g., `int`, `Person`) with direct file-based operations.
- **Specialized `BinaryList<std::string>`**: Handles variable-length strings by serializing length and data.
- **File-Based Storage**: Data is stored in a binary file, preserving the list between program runs.
- **Operations**:
  - `push_back`: Add an element to the end.
  - `insert(index)`: Insert an element at a specified index.
  - `erase(index)`: Remove an element at a specified index.
  - `get(index)`: Retrieve an element by index.
  - `update(index)`: Update an element at a specified index.
  - `pop_back`: Remove the last element.
  - `pop_front`: Remove the first element.
  - `clear`: Clear the entire list and reset the file.
  - `print`: Display all elements.
  - `size`: Get the number of elements.
  - `sort`: Sort the list (bubble sort for POD types, vector-based for strings).
  - `iterator`: Sequential access to elements via an iterator.
- **Interactive Menu**: Console-based interface for managing lists of `int`, `std::string`, or `Person`.

## File Structure
- **Header (`FileHeader`)**: Stores the position of the first node (`head`), last node (`tail`), and the number of nodes (`size`).
- **Node Format** (for POD types):
  - `[int prev][int next][T data]`
- **Node Format for `std::string`**:
  - `[int prev][int next][int length][char data[length]]`
- **Person Structure**: A POD type with a fixed-size `name` (char array, 40 bytes) and an `age` (int), supporting lexicographic sorting by name and age.

## Requirements
- **C++ Compiler**: C++11 or later (uses `std::vector`, `std::string`, `std::sort`).
- **Standard Libraries**: `<iostream>`, `<fstream>`, `<string>`, `<cstring>`, `<cstdio>`, `<cstdlib>`, `<vector>`, `<algorithm>`.
- **Operating System**: Windows (uses `system("cls")` and `system("pause")`). For Unix-like systems, replace with `system("clear")` or remove.
- **File System Access**: Program reads/writes binary files (e.g., `intList.bin`, `strList.bin`, `personList.bin`).

## Build Instructions
1. Compile the code using a C++ compiler:
   ```bash
   g++ -o binary_list main.cpp
   ```
2. Run the executable:
   ```bash
   ./binary_list
   ```

## Usage
1. Run the program to access the main menu.
2. Choose a data type (`int`, `string`, or `Person`).
3. Enter a filename for the binary file (e.g., `list.bin`).
4. Use the sub-menu to perform operations on the list.
5. Input validation ensures non-negative ages for `Person` and valid indices.
6. Strings should not contain spaces (limitation of `std::cin` input).

## Notes
- **Sorting**:
  - For POD types (`int`, `Person`), bubble sort is performed directly in the file.
  - For `std::string`, sorting is done in memory using `std::vector` and `std::sort` to handle variable-length strings.
- **Persistence**: Data remains in the binary file between runs unless cleared.
- **Limitations**:
  - `std::string` input via `std::cin` does not support spaces.
  - File operations assume the program has write permissions.
- **Portability**: Replace Windows-specific `system("cls")` and `system("pause")` for cross-platform compatibility.

## Example
```bash
==== Главное Меню ====
1. int
2. string
3. Person
0. Выход
======================
Ваш выбор: 3
Введите имя файла (например, personList.bin): personList.bin

=== Меню (Person) ===
1. push_back
...
Ваш выбор: 1
Введите имя (без пробелов) и возраст: Alice 25
```