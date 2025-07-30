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

def extract_opcodes(disassembly):
    """Extract just the instruction mnemonics from disassembly"""
    if not disassembly:
        return []
    return [instr["instruction"].split()[0] for instr in disassembly]

def get_opcode_ngrams(opcodes, n=2):
    """Generate n-grams from opcode sequence"""
    if len(opcodes) < n:
        return []
    return [tuple(opcodes[i:i+n]) for i in range(len(opcodes)-n+1)]

def jaccard_similarity(set1, set2):
    """Calculate Jaccard similarity between two sets"""
    if not set1 and not set2:
        return 1.0
    if not set1 or not set2:
        return 0.0
    intersection = len(set1 & set2)
    union = len(set1 | set2)
    return intersection / union

def calculate_opcode_similarity(func1, func2):
    """Calculate similarity based on instruction opcode patterns"""
    opcodes1 = extract_opcodes(func1.get("disassembly", []))
    opcodes2 = extract_opcodes(func2.get("disassembly", []))
    
    if not opcodes1 and not opcodes2:
        return 1.0
    if not opcodes1 or not opcodes2:
        return 0.0
    
    # Get 2-grams and 3-grams
    bigrams1 = set(get_opcode_ngrams(opcodes1, 2))
    bigrams2 = set(get_opcode_ngrams(opcodes2, 2))
    trigrams1 = set(get_opcode_ngrams(opcodes1, 3))
    trigrams2 = set(get_opcode_ngrams(opcodes2, 3))
    
    # Calculate similarities
    bigram_sim = jaccard_similarity(bigrams1, bigrams2)
    trigram_sim = jaccard_similarity(trigrams1, trigrams2)
    
    # Weight trigrams more heavily as they capture more specific patterns
    return (bigram_sim * 0.4) + (trigram_sim * 0.6)

def calculate_address_similarity(func1, func2, max_diff=86852):
    """Calculate similarity based on address proximity"""
    addr1 = int(func1["address"], 16)
    addr2 = int(func2["address"], 16)
    diff = abs(addr1 - addr2)
    
    # Convert to similarity score (1.0 is perfect match, 0.0 is max_diff or greater)
    if diff >= max_diff:
        return 0.0
    return 1.0 - (diff / max_diff)

def calculate_combined_similarity(func1, func2, size_weight=0.5, ref_weight=0.45, addr_weight=0.05, opcode_weight=0.0, max_addr_diff=86852):
    """Calculate weighted combination of size, reference, address, and opcode similarity"""
    size_sim = calculate_size_similarity(func1, func2)
    ref_sim = calculate_reference_similarity(func1, func2)
    addr_sim = calculate_address_similarity(func1, func2, max_addr_diff)
    opcode_sim = calculate_opcode_similarity(func1, func2)
    
    # Apply weights
    return (size_sim * size_weight) + (ref_sim * ref_weight) + (addr_sim * addr_weight) + (opcode_sim * opcode_weight)

def find_most_similar_function(target_function, candidate_functions, size_weight=0.5, ref_weight=0.45, addr_weight=0.05, opcode_weight=0.0, max_addr_diff=86852):
    """Find the most similar function based on weighted combination of size, reference count, address, and opcodes"""
    best_matches = []
    best_score = -1
    
    # Calculate similarity threshold to consider a match almost identical
    SIMILARITY_THRESHOLD = 0.999
    
    for candidate in candidate_functions:
        score = calculate_combined_similarity(target_function, candidate, size_weight, ref_weight, addr_weight, opcode_weight, max_addr_diff)
        
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

# Global arrays to store match examples for inspection
correct_matches = []
incorrect_matches = []
ambiguous_matches = []

def test_name_matching(steam_functions, gog_functions, size_weight, ref_weight, addr_weight=0.05, opcode_weight=0.0, max_addr_diff=86852):
    """Test how well our similarity metric matches already-named functions"""
    global correct_matches, incorrect_matches, ambiguous_matches
    
    # Clear previous examples
    correct_matches.clear()
    incorrect_matches.clear()
    ambiguous_matches.clear()
    
    print(f"Testing name matching with weights: Size={size_weight:.2f}, References={ref_weight:.2f}, Address={addr_weight:.2f}, Opcode={opcode_weight:.2f}")
    
    # Get named functions from steam
    named_steam_funcs = find_named_functions(steam_functions)
    print(f"Found {len(named_steam_funcs)} named functions in steam.json")
    
    # Group by name, excluding duplicates
    steam_name_map = group_by_name(named_steam_funcs)
    print(f"After removing duplicates: {len(steam_name_map)} unique named functions")
    
    # Keep track of total for progress
    total = 0
    
    # Progress tracking
    progress_step = max(1, len(steam_name_map) // 20)  # Show progress in ~5% increments
    
    print(f"\nTesting matches for {len(steam_name_map)} functions...")
    
    for idx, (name, steam_func) in enumerate(steam_name_map.items()):
        # Show progress
        if idx % progress_step == 0:
            print(f"Progress: {idx}/{len(steam_name_map)} ({idx/len(steam_name_map)*100:.1f}%)")
        
        # Find most similar functions in GOG
        best_matches, similarity_score = find_most_similar_function(
            steam_func, 
            gog_functions,
            size_weight=size_weight,
            ref_weight=ref_weight,
            addr_weight=addr_weight,
            opcode_weight=opcode_weight,
            max_addr_diff=max_addr_diff
        )
        
        total += 1
        
        # Check if best match has the same name
        if len(best_matches) > 1:
            # Ambiguous match (multiple with same score)
            ambiguous_matches.append({
                'steam_func': steam_func,
                'gog_matches': best_matches,
                'similarity_score': similarity_score
            })
        else:
            best_match = best_matches[0]
            if best_match["name"] == name:
                correct_matches.append({
                    'steam_func': steam_func,
                    'gog_match': best_match,
                    'similarity_score': similarity_score
                })
            else:
                incorrect_matches.append({
                    'steam_func': steam_func,
                    'gog_match': best_match,
                    'similarity_score': similarity_score
                })
    
    # Print final statistics
    print("\nMatching Results:")
    print(f"Total functions tested: {total}")
    print(f"Correct matches: {len(correct_matches)} ({len(correct_matches)/total*100:.2f}%)")
    print(f"Incorrect matches: {len(incorrect_matches)} ({len(incorrect_matches)/total*100:.2f}%)")
    print(f"Ambiguous matches: {len(ambiguous_matches)} ({len(ambiguous_matches)/total*100:.2f}%)")
    
    return len(correct_matches) / total  # Return success rate

def analyze_address_differences():
    """Analyze address differences between functions with identical real names"""
    global gog_functions, steam_functions
    
    if not gog_functions or not steam_functions:
        print("Function data not loaded. Run main() first.")
        return
    
    print("Finding functions with identical real names in both executables...")
    
    # Get functions with real names from both executables
    steam_named = {f['name']: f for f in steam_functions if is_real_name(f['name'])}
    gog_named = {f['name']: f for f in gog_functions if is_real_name(f['name'])}
    
    # Find functions that exist in both with identical names
    common_names = set(steam_named.keys()) & set(gog_named.keys())
    print(f"Found {len(common_names)} functions with identical real names in both executables")
    
    if not common_names:
        print("No matching named functions found.")
        return
    
    address_diffs = []
    for name in common_names:
        steam_func = steam_named[name]
        gog_func = gog_named[name]
        
        steam_addr = int(steam_func['address'], 16)
        gog_addr = int(gog_func['address'], 16)
        diff = gog_addr - steam_addr
        
        address_diffs.append({
            'function_name': name,
            'steam_addr': steam_addr,
            'gog_addr': gog_addr,
            'difference': diff,
            'abs_difference': abs(diff)
        })
    
    # Sort by absolute difference to see patterns
    address_diffs.sort(key=lambda x: x['abs_difference'])
    
    # Calculate statistics
    diffs = [item['abs_difference'] for item in address_diffs]
    min_diff = min(diffs)
    max_diff = max(diffs)
    avg_diff = sum(diffs) / len(diffs)
    
    # Find most common difference
    from collections import Counter
    diff_counts = Counter(diffs)
    most_common_diff = diff_counts.most_common(1)[0]
    
    print(f"\nAddress difference statistics:")
    print(f"  Min difference: 0x{min_diff:08x} ({min_diff})")
    print(f"  Max difference: 0x{max_diff:08x} ({max_diff})")
    print(f"  Average difference: 0x{int(avg_diff):08x} ({int(avg_diff)})")
    print(f"  Most common difference: 0x{most_common_diff[0]:08x} ({most_common_diff[0]}) - appears {most_common_diff[1]} times")
    
    # Show distribution of differences
    print(f"\nTop 10 most common differences:")
    for diff, count in diff_counts.most_common(10):
        print(f"  0x{diff:08x} ({diff:8d}): {count:3d} functions")
    
    # Show some examples
    print(f"\nFirst 10 examples (sorted by absolute difference):")
    for i, item in enumerate(address_diffs[:10]):
        print(f"  {item['function_name']:30s} Steam: 0x{item['steam_addr']:08x} GOG: 0x{item['gog_addr']:08x} Diff: 0x{item['difference']:08x} (abs: 0x{item['abs_difference']:08x})")
    
    return address_diffs

def main():
    global gog_functions, steam_functions

    # Set default weights
    size_weight = 0.5
    ref_weight = 0.45
    addr_weight = 0.05
    opcode_weight = 0.0
    max_addr_diff = 86852  # Average from analysis
    
    # Load functions from both files
    gog_functions = load_ghidra_export("gog.json")
    steam_functions = load_ghidra_export("steam.json")
    
    if not gog_functions or not steam_functions:
        print("Error loading function data. Exiting.")
        sys.exit(1)

    # Check for analysis mode
    if len(sys.argv) >= 2 and sys.argv[1] == "--analyze":
        analyze_address_differences()
        return
    
    # Check for test mode
    if len(sys.argv) >= 2 and sys.argv[1] == "--test":
        # If weights are provided, use them
        if len(sys.argv) >= 4:
            try:
                size_weight = float(sys.argv[2])
                ref_weight = float(sys.argv[3])
                if len(sys.argv) >= 5:
                    addr_weight = float(sys.argv[4])
                if len(sys.argv) >= 6:
                    opcode_weight = float(sys.argv[5])
                # Normalize weights if they don't sum to 1
                total = size_weight + ref_weight + addr_weight + opcode_weight
                if total != 1.0:
                    size_weight /= total
                    ref_weight /= total
                    addr_weight /= total
                    opcode_weight /= total
            except ValueError:
                print("Error: Weights must be numeric values")
                sys.exit(1)
        
        # Run test mode
        test_name_matching(steam_functions, gog_functions, size_weight, ref_weight, addr_weight, opcode_weight, max_addr_diff)
        return
    
    # Normal mode - lookup a specific function
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python correlator.py <function_address> [size_weight] [ref_weight] [addr_weight] [opcode_weight]")
        print("  python correlator.py --test [size_weight] [ref_weight] [addr_weight] [opcode_weight]")
        print("  python correlator.py --analyze")
        print("\nExamples:")
        print("  python correlator.py 00401000")
        print("  python correlator.py 00401000 0.4 0.3 0.1 0.2")
        print("  python correlator.py --test")
        print("  python correlator.py --test 0.4 0.3 0.1 0.2")
        return
    
    target_address = sys.argv[1]
    
    # Allow overriding weights through command line
    if len(sys.argv) >= 4:
        try:
            size_weight = float(sys.argv[2])
            ref_weight = float(sys.argv[3])
            if len(sys.argv) >= 5:
                addr_weight = float(sys.argv[4])
            if len(sys.argv) >= 6:
                opcode_weight = float(sys.argv[5])
            # Normalize weights if they don't sum to 1
            total = size_weight + ref_weight + addr_weight + opcode_weight
            if total != 1.0:
                size_weight /= total
                ref_weight /= total
                addr_weight /= total
                opcode_weight /= total
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
    print(f"Using weights: Size={size_weight:.2f}, References={ref_weight:.2f}, Address={addr_weight:.2f}, Opcode={opcode_weight:.2f}")
    
    # Find the most similar function in steam.json
    best_matches, similarity_score = find_most_similar_function(
        target_function, 
        steam_functions,
        size_weight=size_weight,
        ref_weight=ref_weight,
        addr_weight=addr_weight,
        opcode_weight=opcode_weight,
        max_addr_diff=max_addr_diff
    )
    
    print("\nBest matches in steam.json:")
    print(f"Combined similarity score: {similarity_score:.4f}")
    
    if len(best_matches) > 1:
        print(f"\nWARNING: Found {len(best_matches)} functions with the same similarity score!")
        
    for i, match in enumerate(best_matches):
        match_size = len(match['raw_bytes'])
        size_sim = calculate_size_similarity(target_function, match)
        ref_sim = calculate_reference_similarity(target_function, match)
        addr_sim = calculate_address_similarity(target_function, match, max_addr_diff)
        opcode_sim = calculate_opcode_similarity(target_function, match)
        
        print(f"\nMatch {i+1}:")
        print(f"Function: {match['name']} at {match['address']}")
        print(f"Size: {match_size} bytes (similarity: {size_sim:.4f})")
        print(f"References: {match['reference_count']} (similarity: {ref_sim:.4f})")
        print(f"Address similarity: {addr_sim:.4f}")
        print(f"Opcode similarity: {opcode_sim:.4f}")
        
        # Show size difference as percentage
        size_diff_pct = abs(target_size - match_size) / target_size * 100
        print(f"Size difference: {abs(target_size - match_size)} bytes ({size_diff_pct:.1f}%)")

if __name__ == "__main__":
    main()
