# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Codebase Overview

This is the **correlator** subdirectory of the C3X project - a Python tool designed to correlate functions between different versions of the Civilization III: Conquests executable (GOG vs Steam versions). The correlator helps with reverse engineering by matching functions across different builds using multiple similarity metrics.

### Main Components

- **correlator.py**: Primary Python script that analyzes exported function data from Ghidra
- **gog.json** & **steam.json**: Large JSON files (90MB+) containing function exports from Ghidra analysis
- Located within the larger C3X project, which is a comprehensive mod for Civilization III: Conquests

## How to Run

### Basic Function Lookup
```bash
python correlator.py <function_address>
```
Example: `python correlator.py 00100400`

### Test Mode (Validate Similarity Algorithm)
```bash
python correlator.py --test
```

### Address Analysis Mode
```bash
python correlator.py --analyze
```
Analyzes address differences between functions with identical names in both executables.

### Custom Similarity Weights
```bash
python correlator.py <address> <size_weight> <ref_weight> <addr_weight> <opcode_weight>
python correlator.py --test <size_weight> <ref_weight> <addr_weight> <opcode_weight>
```
Examples:
- `python correlator.py 00100400 0.4 0.3 0.1 0.2`
- `python correlator.py --test 0.4 0.3 0.1 0.2`

## Core Architecture

### Input Data Format
The tool expects JSON files exported from Ghidra containing function metadata:
- `name`: Function name (real names vs auto-generated "FUN_" names)
- `address`: Memory address as hex string
- `raw_bytes`: Array of function bytecode
- `disassembly`: Array of instruction objects with addresses and instruction strings
- `reference_count`: Number of references to the function

### Similarity Algorithm
The correlator uses a weighted combination of four metrics:
- **Size similarity**: Based on raw byte count (default weight: 0.35)
- **Reference similarity**: Based on reference count (default weight: 0.27)
- **Address similarity**: Based on address proximity (default weight: 0.03)
- **Opcode similarity**: Based on instruction pattern matching using n-grams (default weight: 0.35)

### Key Functions
- `find_most_similar_function()`: Core matching algorithm
- `test_name_matching()`: Validation using functions with identical real names
- `load_ghidra_export()`: JSON data loader with feature preprocessing
- `analyze_address_differences()`: Address offset analysis
- `calculate_opcode_similarity()`: Instruction pattern matching using 2-grams and 3-grams

### Performance Optimizations
- **Opcode preprocessing**: N-grams are precomputed during JSON loading to avoid repeated calculations
- **Progress reporting**: Shows preprocessing progress for large datasets
- **Cached features**: Stores `_bigrams` and `_trigrams` sets in function objects

## Context - Parent C3X Project

This correlator is part of the larger C3X mod project, which:
- Modifies the Civilization III: Conquests executable
- Uses TCC (Tiny C Compiler) for code injection
- Requires reverse engineering to match functions across game versions
- The correlator specifically helps identify corresponding functions between GOG and Steam builds

## Dependencies

- Python 3 (standard library only - json, sys, os, typing, collections)
- No external packages required
- Requires pre-generated JSON exports from Ghidra analysis

## Usage Notes

- JSON files are extremely large (90MB+) - initial loading includes preprocessing that takes 10-30 seconds
- Test mode validates the similarity algorithm against functions with identical real names (~82% accuracy with default weights)
- Address lookups are case-insensitive and handle "0x" prefixes
- The tool normalizes addresses by converting to integers to ignore leading zeros
- Real function names exclude auto-generated patterns: "FUN_", "thunk_", "Unwind@", "__", "FID_conflict:"
- Address analysis shows average absolute difference of ~86,852 bytes between matching functions

## Current Performance

With optimized default weights (0.35/0.27/0.03/0.35), the correlator achieves:
- ~82% correct matches on functions with real names
- Significantly improved accuracy through opcode pattern matching
- Balanced approach using both structural (size/references) and algorithmic (opcodes) similarity