#!/usr/bin/env python3

import json
import sys
import os
from typing import List, Dict, Any

def load_ghidra_export(filepath: str) -> List[Dict[str, Any]]:
    """
    Load function data exported from Ghidra by ExportProgForPython.py
    
    The expected JSON format is:
    [
        {
            "name": "function_name",            # String: Name of the function
            "address": "00100400",              # String: Address of function entry point
            "raw_bytes": [72, 131, 236, ...],   # List[int]: Raw bytes of the function
            "disassembly": [                    # List[Dict]: Disassembled instructions
                {
                    "address": "00100400",      # String: Instruction address
                    "instruction": "SUB RSP, 0x20" # String: Disassembled instruction
                },
                ...
            ],
            "reference_count": 5                # Int: Number of references to this function
        },
        ...
    ]
    
    Args:
        filepath: Path to the exported JSON file
        
    Returns:
        List of function data dictionaries
    """
    try:
        with open(filepath, 'r') as f:
            data = json.load(f)
        
        print(f"Successfully loaded {len(data)} functions from {filepath}")
        return data
    except json.JSONDecodeError:
        print(f"Error: {filepath} contains invalid JSON")
        return []
    except FileNotFoundError:
        print(f"Error: File {filepath} not found")
        return []

def find_function_by_address(functions, address):
    """Find a function by its address, ignoring leading zeros and '0x' prefix"""
    # Normalize the input address: remove '0x' prefix and convert to integer
    if address.lower().startswith("0x"):
        address = address[2:]
    # Convert to integer to ignore leading zeros
    normalized_address = int(address, 16)
    
    for function in functions:
        func_addr = function["address"]
        if func_addr.lower().startswith("0x"):
            func_addr = func_addr[2:]
        # Convert to integer to ignore leading zeros
        func_addr_int = int(func_addr, 16)
        
        if func_addr_int == normalized_address:
            return function
    return None

def calculate_size_similarity(func1, func2):
    """Calculate similarity based on function size (number of bytes)"""
    size1 = len(func1["raw_bytes"])
    size2 = len(func2["raw_bytes"])
    
    # Calculate similarity score (1.0 is perfect match)
    if size1 == 0 or size2 == 0:
        return 0.0
    
    # Use the ratio of the smaller to the larger size
    return min(size1, size2) / max(size1, size2)

def calculate_reference_similarity(func1, func2):
    """Calculate similarity based on reference count"""
    ref1 = func1["reference_count"]
    ref2 = func2["reference_count"]
    
    # Calculate similarity score (1.0 is perfect match)
    if ref1 == 0 and ref2 == 0:
        return 1.0  # Both have 0 references, perfect match
    elif ref1 == 0 or ref2 == 0:
        return 0.0  # One has references, the other doesn't
    
    # Use the ratio of the smaller to the larger reference count
    return min(ref1, ref2) / max(ref1, ref2)

def calculate_combined_similarity(func1, func2, size_weight=0.6, ref_weight=0.4):
    """Calculate weighted combination of size and reference similarity"""
    size_sim = calculate_size_similarity(func1, func2)
    ref_sim = calculate_reference_similarity(func1, func2)
    
    # Apply weights
    return (size_sim * size_weight) + (ref_sim * ref_weight)

def find_most_similar_function(target_function, candidate_functions, size_weight=0.6, ref_weight=0.4):
    """Find the most similar function based on weighted combination of size and reference count"""
    best_matches = []
    best_score = -1
    
    # Calculate similarity threshold to consider a match almost identical
    SIMILARITY_THRESHOLD = 0.999
    
    for candidate in candidate_functions:
        score = calculate_combined_similarity(target_function, candidate, size_weight, ref_weight)
        
        # Round to 4 decimal places to avoid floating point comparison issues
        score = round(score, 4)
        
        if score > best_score:
            # New best score, clear previous matches
            best_score = score
            best_matches = [candidate]
        elif score == best_score:
            # Same score, add to list of matches
            best_matches.append(candidate)
    
    return best_matches, best_score

def is_real_name(name):
    """Check if a function has a real name or is an auto-generated name"""
    return name and not any([name.startswith(x) for x in ["FUN_", "thunk_", "Unwind@", "__", "FID_conflict:"]])

def find_named_functions(functions):
    """Return list of functions with real names (not auto-generated)"""
    return [f for f in functions if is_real_name(f["name"])]

def group_by_name(functions):
    """Group functions by name, filtering out duplicates"""
    name_groups = {}
    for func in functions:
        name = func["name"]
        if name not in name_groups:
            name_groups[name] = []
        name_groups[name].append(func)
    
    # Only keep names that have a single function
    return {name: funcs[0] for name, funcs in name_groups.items() if len(funcs) == 1}

def test_name_matching(steam_functions, gog_functions, size_weight, ref_weight):
    """Test how well our similarity metric matches already-named functions"""
    print(f"Testing name matching with weights: Size={size_weight:.2f}, References={ref_weight:.2f}")
    
    # Get named functions from steam
    named_steam_funcs = find_named_functions(steam_functions)
    print(f"Found {len(named_steam_funcs)} named functions in steam.json")
    
    # Group by name, excluding duplicates
    steam_name_map = group_by_name(named_steam_funcs)
    print(f"After removing duplicates: {len(steam_name_map)} unique named functions")
    
    # Counter for statistics
    total = 0
    correct_matches = 0
    incorrect_matches = 0
    ambiguous_matches = 0
    
    # Progress tracking
    progress_step = max(1, len(steam_name_map) // 20)  # Show progress in ~5% increments
    
    print(f"\nTesting matches for {len(steam_name_map)} functions...")
    
    for idx, (name, steam_func) in enumerate(steam_name_map.items()):
        # Show progress
        if idx % progress_step == 0:
            print(f"Progress: {idx}/{len(steam_name_map)} ({idx/len(steam_name_map)*100:.1f}%)")
        
        # Find most similar functions in GOG
        best_matches, _ = find_most_similar_function(
            steam_func, 
            gog_functions,
            size_weight=size_weight,
            ref_weight=ref_weight
        )
        
        total += 1
        
        # Check if best match has the same name
        if len(best_matches) > 1:
            # Ambiguous match (multiple with same score)
            ambiguous_matches += 1
        else:
            best_match = best_matches[0]
            if best_match["name"] == name:
                correct_matches += 1
            else:
                incorrect_matches += 1
    
    # Print final statistics
    print("\nMatching Results:")
    print(f"Total functions tested: {total}")
    print(f"Correct matches: {correct_matches} ({correct_matches/total*100:.2f}%)")
    print(f"Incorrect matches: {incorrect_matches} ({incorrect_matches/total*100:.2f}%)")
    print(f"Ambiguous matches: {ambiguous_matches} ({ambiguous_matches/total*100:.2f}%)")
    
    return correct_matches / total  # Return success rate

def main():
    global gog_functions, steam_functions

    # Set default weights
    size_weight = 0.6
    ref_weight = 0.4
    
    # Load functions from both files
    gog_functions = load_ghidra_export("gog.json")
    steam_functions = load_ghidra_export("steam.json")
    
    if not gog_functions or not steam_functions:
        print("Error loading function data. Exiting.")
        sys.exit(1)

    # Check for test mode
    if len(sys.argv) >= 2 and sys.argv[1] == "--test":
        # If weights are provided, use them
        if len(sys.argv) >= 4:
            try:
                size_weight = float(sys.argv[2])
                ref_weight = float(sys.argv[3])
                # Normalize weights if they don't sum to 1
                total = size_weight + ref_weight
                if total != 1.0:
                    size_weight /= total
                    ref_weight /= total
            except ValueError:
                print("Error: Weights must be numeric values")
                sys.exit(1)
        
        # Run test mode
        test_name_matching(steam_functions, gog_functions, size_weight, ref_weight)
        return
    
    # Normal mode - lookup a specific function
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python correlator.py <function_address> [size_weight] [ref_weight]")
        print("  python correlator.py --test [size_weight] [ref_weight]")
        print("\nExamples:")
        print("  python correlator.py 00401000")
        print("  python correlator.py 00401000 0.7 0.3")
        print("  python correlator.py --test")
        print("  python correlator.py --test 0.7 0.3")
        return
    
    target_address = sys.argv[1]
    
    # Allow overriding weights through command line
    if len(sys.argv) >= 4:
        try:
            size_weight = float(sys.argv[2])
            ref_weight = float(sys.argv[3])
            # Normalize weights if they don't sum to 1
            total = size_weight + ref_weight
            if total != 1.0:
                size_weight /= total
                ref_weight /= total
        except ValueError:
            print("Error: Weights must be numeric values")
            sys.exit(1)
    
    # Find the target function in gog.json
    target_function = find_function_by_address(gog_functions, target_address)
    if not target_function:
        print(f"No function found at address {target_address} in gog.json")
        sys.exit(1)
    
    target_size = len(target_function["raw_bytes"])
    print(f"Found target function: {target_function['name']} at {target_function['address']}")
    print(f"Function size: {target_size} bytes")
    print(f"References: {target_function['reference_count']}")
    print(f"Using weights: Size={size_weight:.2f}, References={ref_weight:.2f}")
    
    # Find the most similar function in steam.json
    best_matches, similarity_score = find_most_similar_function(
        target_function, 
        steam_functions,
        size_weight=size_weight,
        ref_weight=ref_weight
    )
    
    print("\nBest matches in steam.json:")
    print(f"Combined similarity score: {similarity_score:.4f}")
    
    if len(best_matches) > 1:
        print(f"\nWARNING: Found {len(best_matches)} functions with the same similarity score!")
        
    for i, match in enumerate(best_matches):
        match_size = len(match['raw_bytes'])
        size_sim = calculate_size_similarity(target_function, match)
        ref_sim = calculate_reference_similarity(target_function, match)
        
        print(f"\nMatch {i+1}:")
        print(f"Function: {match['name']} at {match['address']}")
        print(f"Size: {match_size} bytes (similarity: {size_sim:.4f})")
        print(f"References: {match['reference_count']} (similarity: {ref_sim:.4f})")
        
        # Show size difference as percentage
        size_diff_pct = abs(target_size - match_size) / target_size * 100
        print(f"Size difference: {abs(target_size - match_size)} bytes ({size_diff_pct:.1f}%)")

if __name__ == "__main__":
    main()
