#include "bloom_filter.hpp"
#include <iostream>
#include <vector>
#include <string>

/**************************************************************************************
 
 * main.cpp — Demo driver for BloomFilter
 * 
 * What this file does:
 * 1. Creates a BloomFilter with known n and p
 * 2. Inserts a set of known words
 * 3. Queries known words  -> should all return true  (no false negatives)
 * 4. Queries unknown words -> should mostly return false (rare false positives)
 * 5. Prints diagnostics — actual vs theoretical false positive rate
 
***************************************************************************************/

int main() {

    //------------------Step 1 : Create the filter---------------------------------------
    
    size_t expectedElements  = 100;   // we plan to insert 100 words
    double falsePositiveRate = 0.01;  // we want at most 1% false positives

    BloomFilter bf(expectedElements, falsePositiveRate);

    std::cout << "==============================\n";
    std::cout << "   Bloom Filter Demo\n";
    std::cout << "==============================\n\n";

    std::cout << "Configuration:\n";
    std::cout << "  Expected elements  (n) : " << expectedElements  << "\n";
    std::cout << "  False positive rate(p) : " << falsePositiveRate << "\n";
    std::cout << "  Bit array size     (m) : " << bf.bitArraySize()      << "\n";
    std::cout << "  Hash functions     (k) : " << bf.numHashFunctions()  << "\n\n";

    //------------------Step 2 : Insert known words--------------------------------------

    std::vector<std::string> inserted = {
        "apple", "banana", "cherry", "date", "elderberry",
        "fig", "grape", "honeydew", "kiwi", "lemon",
        "mango", "nectarine", "orange", "papaya", "quince",
        "raspberry", "strawberry", "tangerine", "ugli", "vanilla"
    };

    std::cout << "Inserting " << inserted.size() << " words...\n\n";
    for (const auto& word : inserted) {
        bf.insert(word);
    }

    //------------------Step 3 : Query inserted words ------------------------------------
    // A correct bloom filter must NEVER return false for an inserted element.

    std::cout << "--- Querying inserted words (expect: all PRESENT) ---\n";
    int falseNegatives = 0;
    for (const auto& word : inserted) {
        bool result = bf.contains(word);
        std::cout << "  contains(\"" << word << "\") -> " 
                  << (result ? "PRESENT" : "ABSENT") << "\n";
        if (!result) falseNegatives++;
    }

    if (falseNegatives == 0)
        std::cout << "\n  No false negatives. Correct!\n\n";
    else
        std::cout << "\n  ERROR: " << falseNegatives << " false negatives found!\n\n";

    //------------------Step 4 : Query unknown words -------------------------------------
    // These were never inserted. Bloom filter should return false for most.
    // A few may return true -> those are false positives.

    std::vector<std::string> notInserted = {
        "apricot", "blueberry", "coconut", "dragonfruit", "eucalyptus",
        "feijoa", "guava", "huckleberry", "imbe", "jackfruit",
        "kumquat", "lime", "mulberry", "noni", "olive",
        "peach", "plum", "rambutan", "soursop", "tamarind"
    };

    std::cout << "--- Querying non-inserted words (expect: mostly ABSENT) ---\n";
    int falsePositives = 0;
    for (const auto& word : notInserted) {
        bool result = bf.contains(word);
        std::cout << "  contains(\"" << word << "\") -> "
                  << (result ? "PRESENT (false positive!)" : "ABSENT") << "\n";
        if (result) falsePositives++;
    }

    //------------------Step 5 : Diagnostics---------------------------------------------

    double actualFPRate      = static_cast<double>(falsePositives) / notInserted.size();
    double theoreticalFPRate = bf.theoreticalFalsePositiveRate();

    std::cout << "\n--- Diagnostics ---\n";
    std::cout << "  Elements inserted          : " << bf.elementCount()   << "\n";
    std::cout << "  Bits set to 1              : " << bf.setBitCount()    << " / " << bf.bitArraySize() << "\n";
    std::cout << "  False positives found      : " << falsePositives      << " / " << notInserted.size() << "\n";
    std::cout << "  Actual false positive rate : " << actualFPRate        << "\n";
    std::cout << "  Theoretical FP rate        : " << theoreticalFPRate   << "\n";

    if (actualFPRate <= falsePositiveRate * 2)
        std::cout << "\n  FP rate within expected range. Filter working correctly!\n";
    else
        std::cout << "\n  WARNING: FP rate higher than expected.\n";

    std::cout << "\n==============================\n";

    return 0;
}

