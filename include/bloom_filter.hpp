#pragma once // to ensure that the file is included only once per compilation unit.

#include <vector>
#include <string>
#include <cmath> // for log , pow
#include <cstddef> // size_t -> An unsigned integer type used to represent the size of objects and array indices.


/****************************************************************************************

 * BloomFilter — a space-efficient probabilistic data structure
 * If Bloom filer says element is
 * Absent -> Definitely not present
 * Present -> Maybe present
 * Bloom Filters allow false positives but never false negatives.

****************************************************************************************/

class BloomFilter{
public:  // Members are private by default.
    
    /***********************************************************************************
     
     * constructor -> A constructor is a special method that runs when  Bloom Filter object is created
     * A Bloom Filter needs two internal settings:
     * m = size of bit array
     * k = number of hash functions
     * Instead of asking the user to calculate these values manually, the constructor computes them automatically.
     * so user has to tell
     * How many items to store (n)
     * How much error can be tolerated (p)
     * @param expectedElements   n  — how many items  to insert
     * @param falsePositiveRate  p  — desired false positive probability (0.0 to 1.0)
     
     *************************************************************************************/

    BloomFilter(size_t expectedElements , double falsePositiveRate );

    /*************************************************************************************
     
     * Insert Method
     * After this method is called for inserting value called val 
     * Bloom filter will always return true for contains(value). 
     * In a standard Bloom filter, elements cannot be deleted once they are added.
     * Attempt to delete an element by unsetting its corresponding bits 
     * risk creating false negatives, because those same bits may be shared by other, still-valid elements
     
     *************************************************************************************/

    void insert(const std::string& key) ;

    /**************************************************************************************
     
     * check that element is probably in the set. Beacause even if the bloom filer says yes it might not be present so answer will be probablistic
     * @return false → element is DEFINITELY NOT in the set (100% certain)
     * @return true  → element is PROBABLY in the set (may be false positive)
      
    ***************************************************************************************/

    bool contains(const std:: string &key) const ;
    // The second const means the function won't modify the object itself
    // No member variables of the BloomFilter will be changed.

    //--------------- Diagnostics-----------------------------------------------------------

    // Theoretical false positive rate based on current fill level
    // trailing const member function qualifier, meaning this function won't modify the BloomFilter object.
    double theoreticalFalsePositiveRate() const;

    // Number of bits set to 1 in the bit array
    size_t setBitCount() const;

    // Total size of the bit array (m)
    size_t bitArraySize() const;

    // Number of hash functions being used (k)
    size_t numHashFunctions() const;

    // Number of elements inserted so far
    size_t elementCount() const;

    //---------------------------------------------------------------------------------------

private:

    //-------------- core state ------------------------------------------------------------
    // The trailing underscore is a C++ convention for private member variables.
    std::vector<bool> bits_ ; // bit array of size m 
    size_t m_ ; // bit array size ; 
    size_t k_ ; // number of hash functions ;
    size_t n_ ; // number of elements inserted
    //--------------------------------------------------------------------------------------

    //------------------Internal helpers ---------------------------------------------------

    /**************************************************************************************
     
     * Bloom filter needs k different hash positions for every key. 
     * Computing k completely independent hash functions is expensive
     * Double Hashing Trick : compute 2 base hashes, then combine them to fake k hashes:
     * h(key, i) = (h1(key) + i * h2(key)) % m
     * h1(key) -> first base hash of the key
     * h2(Key) -> second base hash of key 
     * i -> hash number to be calculated (0, 1 , 2 .... k-1)
     * m -> size of bit array
     * 
     * Example -> h1("cat") = 10, h2("cat") = 7, m = 20, k = 3
     * i=0 → (10 + 0*7) % 20 = 10   ← set bit 10
     * i=1 → (10 + 1*7) % 20 = 17   ← set bit 17
     * i=2 → (10 + 2*7) % 20 = 4   ← set bit 4
     * Three different positions, from just two hash computations.
      
    ***************************************************************************************/

    size_t hash(const std:: string &key , size_t i) const ;

    // Calculate optimal m from n and desired false positive rate p 
    static size_t optimalBitArraySize(size_t n, double p);
    
    // Calculate optimal k from m and n
    static size_t optimalHashCount(size_t m, size_t n);


} ;
