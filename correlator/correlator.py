#!/usr/bin/env python3

import json
import sys
import os
from typing import List, Dict, Any

def precompute_function_features(functions):
    """Precompute expensive features for all functions"""
    print(f"Precomputing opcode features for {len(functions)} functions...")
    for i, func in enumerate(functions):
        if i % 1000 == 0 and i > 0:
            print(f"  Processed {i}/{len(functions)} functions...")
        
        opcodes = extract_opcodes(func.get("disassembly", []))
        func['_bigrams'] = set(get_opcode_ngrams(opcodes, 2))
        func['_trigrams'] = set(get_opcode_ngrams(opcodes, 3))
    print(f"  Completed preprocessing {len(functions)} functions")

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
        
        # Precompute opcode features
        precompute_function_features(data)
        
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
    """Calculate similarity based on instruction opcode patterns using precomputed n-grams"""
    # Use precomputed n-grams if available, otherwise compute on the fly
    bigrams1 = func1.get('_bigrams')
    bigrams2 = func2.get('_bigrams')
    trigrams1 = func1.get('_trigrams')
    trigrams2 = func2.get('_trigrams')
    
    if bigrams1 is None or bigrams2 is None or trigrams1 is None or trigrams2 is None:
        # Fallback to computing on the fly (for backward compatibility)
        opcodes1 = extract_opcodes(func1.get("disassembly", []))
        opcodes2 = extract_opcodes(func2.get("disassembly", []))
        
        if not opcodes1 and not opcodes2:
            return 1.0
        if not opcodes1 or not opcodes2:
            return 0.0
        
        bigrams1 = set(get_opcode_ngrams(opcodes1, 2))
        bigrams2 = set(get_opcode_ngrams(opcodes2, 2))
        trigrams1 = set(get_opcode_ngrams(opcodes1, 3))
        trigrams2 = set(get_opcode_ngrams(opcodes2, 3))
    
    # Calculate similarities using precomputed or computed n-grams
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

def calculate_combined_similarity(func1, func2, size_weight=0.35, ref_weight=0.27, addr_weight=0.03, opcode_weight=0.35, max_addr_diff=86852):
    """Calculate weighted combination of size, reference, address, and opcode similarity"""
    size_sim = calculate_size_similarity(func1, func2)
    ref_sim = calculate_reference_similarity(func1, func2)
    addr_sim = calculate_address_similarity(func1, func2, max_addr_diff)
    opcode_sim = calculate_opcode_similarity(func1, func2)
    
    # Apply weights
    return (size_sim * size_weight) + (ref_sim * ref_weight) + (addr_sim * addr_weight) + (opcode_sim * opcode_weight)

def find_most_similar_function(target_function, candidate_functions, size_weight=0.35, ref_weight=0.27, addr_weight=0.03, opcode_weight=0.35, max_addr_diff=86852):
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

def test_name_matching(steam_functions, gog_functions, size_weight, ref_weight, addr_weight=0.03, opcode_weight=0.35, max_addr_diff=86852, comparison_label="Steam"):
    """Test how well our similarity metric matches already-named functions"""
    global correct_matches, incorrect_matches, ambiguous_matches
    
    # Clear previous examples
    correct_matches.clear()
    incorrect_matches.clear()
    ambiguous_matches.clear()
    
    print(f"Testing name matching between GOG and {comparison_label} with weights: Size={size_weight:.2f}, References={ref_weight:.2f}, Address={addr_weight:.2f}, Opcode={opcode_weight:.2f}")
    
    # Get named functions from comparison file
    named_steam_funcs = find_named_functions(steam_functions)
    print(f"Found {len(named_steam_funcs)} named functions in comparison file ({comparison_label})")
    
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

def generate_training_data():
    """Generate training data from functions with known matches and mismatches"""
    global gog_functions, steam_functions
    
    if not gog_functions or not steam_functions:
        print("Function data not loaded. Run main() first.")
        return [], []
    
    print("Generating training data for logistic regression...")
    
    # Get functions with real names from both executables
    steam_named = {f['name']: f for f in steam_functions if is_real_name(f['name'])}
    gog_named = {f['name']: f for f in gog_functions if is_real_name(f['name'])}
    
    # Find functions that exist in both with identical names (positive examples)
    common_names = set(steam_named.keys()) & set(gog_named.keys())
    print(f"Found {len(common_names)} positive examples (identical names)")
    
    features = []
    labels = []
    
    # Positive examples: functions with identical names
    for name in common_names:
        steam_func = steam_named[name]
        gog_func = gog_named[name]
        
        # Calculate all similarity features
        size_sim = calculate_size_similarity(steam_func, gog_func)
        ref_sim = calculate_reference_similarity(steam_func, gog_func)
        addr_sim = calculate_address_similarity(steam_func, gog_func)
        opcode_sim = calculate_opcode_similarity(steam_func, gog_func)
        
        features.append([size_sim, ref_sim, addr_sim, opcode_sim])
        labels.append(1)  # Correct match
    
    # Negative examples: generate mismatches
    # For each positive example, find some functions that are NOT matches
    print("Generating negative examples...")
    import random
    random.seed(42)  # For reproducibility
    
    negative_count = 0
    target_negatives = len(common_names) * 2  # 2:1 ratio of negatives to positives
    
    steam_funcs_list = list(steam_named.values())
    gog_funcs_list = list(gog_named.values())
    
    for _ in range(target_negatives):
        if negative_count >= target_negatives:
            break
            
        # Pick random functions that don't have the same name
        steam_func = random.choice(steam_funcs_list)
        gog_func = random.choice(gog_funcs_list)
        
        # Skip if they actually match
        if steam_func['name'] == gog_func['name']:
            continue
            
        # Calculate similarity features
        size_sim = calculate_size_similarity(steam_func, gog_func)
        ref_sim = calculate_reference_similarity(steam_func, gog_func)
        addr_sim = calculate_address_similarity(steam_func, gog_func)
        opcode_sim = calculate_opcode_similarity(steam_func, gog_func)
        
        features.append([size_sim, ref_sim, addr_sim, opcode_sim])
        labels.append(0)  # Incorrect match
        negative_count += 1
    
    print(f"Generated {len([l for l in labels if l == 1])} positive and {len([l for l in labels if l == 0])} negative examples")
    return features, labels

def train_logistic_regression():
    """Train logistic regression model to learn optimal feature weights"""
    try:
        from sklearn.linear_model import LogisticRegression
        from sklearn.model_selection import cross_val_score
        from sklearn.preprocessing import StandardScaler
        import numpy as np
    except ImportError:
        print("Error: scikit-learn not available. Install with: pip install scikit-learn")
        return None
    
    # Generate training data
    features, labels = generate_training_data()
    if not features:
        print("No training data generated")
        return None
    
    features = np.array(features)
    labels = np.array(labels)
    
    print(f"Training on {len(features)} examples...")
    
    # Standardize features (important for logistic regression)
    scaler = StandardScaler()
    features_scaled = scaler.fit_transform(features)
    
    # Train logistic regression with L2 regularization
    model = LogisticRegression(random_state=42, max_iter=1000)
    model.fit(features_scaled, labels)
    
    # Evaluate with cross-validation
    cv_scores = cross_val_score(model, features_scaled, labels, cv=5)
    print(f"Cross-validation accuracy: {cv_scores.mean():.3f} (+/- {cv_scores.std() * 2:.3f})")
    
    # Get feature importance (coefficients)
    coefficients = model.coef_[0]
    feature_names = ['Size', 'References', 'Address', 'Opcode']
    
    print("\nLearned feature coefficients:")
    for name, coef in zip(feature_names, coefficients):
        print(f"  {name}: {coef:.4f}")
    
    # Convert to normalized weights (positive, sum to 1)
    # Take absolute values and normalize
    abs_coefficients = np.abs(coefficients)
    normalized_weights = abs_coefficients / np.sum(abs_coefficients)
    
    print("\nNormalized weights for correlator:")
    for name, weight in zip(feature_names, normalized_weights):
        print(f"  {name}: {weight:.3f}")
    
    print(f"\nSuggested command line:")
    print(f"python correlator.py --test {normalized_weights[0]:.3f} {normalized_weights[1]:.3f} {normalized_weights[2]:.3f} {normalized_weights[3]:.3f}")
    
    return {
        'model': model,
        'scaler': scaler,
        'weights': normalized_weights,
        'coefficients': coefficients,
        'cv_accuracy': cv_scores.mean()
    }

def process_multiple_addresses(addresses, gog_functions, steam_functions, size_weight, ref_weight, addr_weight, opcode_weight, max_addr_diff, comparison_label="Steam"):
    """Process multiple addresses and find matches for each"""
    results = []
    
    print(f"Processing {len(addresses)} addresses with default weights...")
    print(f"Using weights: Size={size_weight:.2f}, References={ref_weight:.2f}, Address={addr_weight:.2f}, Opcode={opcode_weight:.2f}")
    print("=" * 80)
    
    for i, address in enumerate(addresses):
        print(f"\n[{i+1}/{len(addresses)}] Processing address: {address}")
        print("-" * 40)
        
        # Find the target function in gog.json
        target_function = find_function_by_address(gog_functions, address)
        if not target_function:
            print(f"No function found at address {address} in gog.json")
            results.append({
                'address': address,
                'found': False,
                'error': f"No function found at address {address}"
            })
            continue
        
        target_size = len(target_function["raw_bytes"])
        print(f"Found target function: {target_function['name']} at {target_function['address']}")
        print(f"Function size: {target_size} bytes, References: {target_function['reference_count']}")
        
        # Find the most similar function in comparison file
        best_matches, similarity_score = find_most_similar_function(
            target_function, 
            steam_functions,
            size_weight=size_weight,
            ref_weight=ref_weight,
            addr_weight=addr_weight,
            opcode_weight=opcode_weight,
            max_addr_diff=max_addr_diff
        )
        
        print(f"Combined similarity score: {similarity_score:.4f}")
        
        # Store results
        match_data = []
        for match in best_matches:
            match_size = len(match['raw_bytes'])
            size_sim = calculate_size_similarity(target_function, match)
            ref_sim = calculate_reference_similarity(target_function, match)
            addr_sim = calculate_address_similarity(target_function, match, max_addr_diff)
            opcode_sim = calculate_opcode_similarity(target_function, match)
            
            match_info = {
                'name': match['name'],
                'address': match['address'],
                'size': match_size,
                'references': match['reference_count'],
                'size_similarity': size_sim,
                'ref_similarity': ref_sim,
                'addr_similarity': addr_sim,
                'opcode_similarity': opcode_sim
            }
            match_data.append(match_info)
            
            print(f"Best match in {comparison_label}: {match['name']} at {match['address']}")
            print(f"  Size: {match_size} bytes (similarity: {size_sim:.4f})")
            print(f"  References: {match['reference_count']} (similarity: {ref_sim:.4f})")
            print(f"  Address similarity: {addr_sim:.4f}")
            print(f"  Opcode similarity: {opcode_sim:.4f}")
        
        if len(best_matches) > 1:
            print(f"WARNING: Found {len(best_matches)} functions with the same similarity score!")
        
        results.append({
            'address': address,
            'found': True,
            'target_function': {
                'name': target_function['name'],
                'address': target_function['address'],
                'size': target_size,
                'references': target_function['reference_count']
            },
            'matches': match_data,
            'similarity_score': similarity_score,
            'match_count': len(best_matches)
        })
    
    return results

def main():
    global gog_functions, steam_functions

    # Check for --pcg flag first (before loading files)
    use_pcg = "--pcg" in sys.argv
    if use_pcg:
        # Remove --pcg from argv so it doesn't interfere with other parsing
        sys.argv = [arg for arg in sys.argv if arg != "--pcg"]
    
    # Set default weights
    size_weight = 0.35
    ref_weight = 0.27
    addr_weight = 0.03
    opcode_weight = 0.35
    max_addr_diff = 86852  # Average from analysis
    
    # Load functions from both files
    comparison_file = "pcg.json" if use_pcg else "steam.json"
    comparison_label = "PCGames.de" if use_pcg else "Steam"
    
    gog_functions = load_ghidra_export("gog.json")
    steam_functions = load_ghidra_export(comparison_file)
    
    if not gog_functions or not steam_functions:
        print("Error loading function data. Exiting.")
        sys.exit(1)

    # Check for analysis mode
    if len(sys.argv) >= 2 and sys.argv[1] == "--analyze":
        analyze_address_differences()
        return
    
    # Check for ML training mode
    if len(sys.argv) >= 2 and sys.argv[1] == "--train":
        train_logistic_regression()
        return
    
    # Check for list mode
    if len(sys.argv) >= 2 and sys.argv[1] == "--list":
        if len(sys.argv) < 3:
            print("Error: --list requires at least one address")
            sys.exit(1)
        addresses = sys.argv[2:]
        
        results = process_multiple_addresses(addresses, gog_functions, steam_functions, size_weight, ref_weight, addr_weight, opcode_weight, max_addr_diff, comparison_label)
        
        # Print summary
        print("\n" + "=" * 80)
        print("SUMMARY")
        print("=" * 80)
        
        found_count = sum(1 for r in results if r['found'])
        print(f"Total addresses processed: {len(results)}")
        print(f"Functions found: {found_count}")
        print(f"Functions not found: {len(results) - found_count}")
        
        if found_count > 0:
            avg_score = sum(r['similarity_score'] for r in results if r['found']) / found_count
            print(f"Average similarity score: {avg_score:.4f}")
            
            ambiguous_count = sum(1 for r in results if r['found'] and r['match_count'] > 1)
            if ambiguous_count > 0:
                print(f"Ambiguous matches (multiple with same score): {ambiguous_count}")
        
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
        test_name_matching(steam_functions, gog_functions, size_weight, ref_weight, addr_weight, opcode_weight, max_addr_diff, comparison_label)
        return
    
    # Normal mode - lookup a specific function
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python correlator.py <function_address> [size_weight] [ref_weight] [addr_weight] [opcode_weight] [--pcg]")
        print("  python correlator.py --list <address1> <address2> ... <addressN> [--pcg]")
        print("  python correlator.py --test [size_weight] [ref_weight] [addr_weight] [opcode_weight] [--pcg]")
        print("  python correlator.py --analyze [--pcg]")
        print("  python correlator.py --train [--pcg]")
        print("\nOptions:")
        print("  --pcg    Use pcg.json (PCGames.de version) instead of steam.json for comparison")
        print("\nExamples:")
        print("  python correlator.py 00401000")
        print("  python correlator.py 00401000 --pcg")
        print("  python correlator.py 00401000 0.4 0.3 0.1 0.2")
        print("  python correlator.py --list 00401000 00402000 00403000")
        print("  python correlator.py --list 00401000 00402000 --pcg")
        print("  python correlator.py --test")
        print("  python correlator.py --test --pcg")
        print("  python correlator.py --test 0.4 0.3 0.1 0.2 --pcg")
        print("  python correlator.py --train  # Learn optimal weights with ML")
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
    
    # Find the most similar function in comparison file
    best_matches, similarity_score = find_most_similar_function(
        target_function, 
        steam_functions,
        size_weight=size_weight,
        ref_weight=ref_weight,
        addr_weight=addr_weight,
        opcode_weight=opcode_weight,
        max_addr_diff=max_addr_diff
    )
    
    print(f"\nBest matches in {comparison_label}:")
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
