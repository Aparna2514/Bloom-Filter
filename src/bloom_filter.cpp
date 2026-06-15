#include "bloom_filter.hpp"
#include <stdexcept> 
// Above header provides a set of predefined exception classes for reporting common error conditions.
// std::invalid_argument is logical error that could theoretically be caught at compile time 
#include <algorithm> // for count 
#include <cstdint>    // for uint64_t  

/*****************************************************************************************
 
 * Maths needed in constructor
 * Given
 * n -> numbers of elements
 * p -> desired false positive rates
 * 
 * optimal bit array size -> m 
 *  m  = -(n* ln(p))/(ln(2))^2
 * 
 * optimal number of hashes -> k
 *  k  =  (m/n)*ln(2)
  
******************************************************************************************/

//--------------------Static Helpers------------------------------------------------------

size_t BloomFilter::optimalBitArraySize(size_t n , double p){
    // m = -(n*ln(p))/(ln(2))^2
    double m = -(static_cast<double>(n)*std::log(p))/(std::log(2.0)*std::log(2.0));
    return static_cast<size_t>(std::ceil(m));

}

size_t BloomFilter::optimalHashCount(size_t m , size_t n){
    // k = (m/n)* ln(2)
    double k = (static_cast<double>(m)/static_cast<double>(n))*std::log(2.0);
    return static_cast<size_t>(std::round(k));
}


//-------------------------Constructor---------------------------------------------------
BloomFilter::BloomFilter(size_t expectedElements , double falsePositiveRate){

    // validiate inputs

    /************************************************************************************
     * Why expected elements must be > 0
     * 
     * In calaculation of m , if n = 0 -> m = 0 -> bit array has zero bits — a completely useless filter.
     * In calculation of k , if n = 0 , it leads to 0/0 -> undefined behaviour
     * bits_.assign(m_, false) creates empty vectorany call to bits_[hash(key, i)] would be accessing out of bounds memory
     
    ***************************************************************************************/

    if(expectedElements == 0)
        throw std::invalid_argument("expectedElements must be > 0");
    
    /***********************************************************************************
     
     * if p == 0 -> zero false positives — a perfect filter.
     *  m = -(n * ln(0)) / (ln(2))^2 
     * ln(0) is negative infinity, so: m = +infinity
     
     ---------------------------------------------------------------------------------

     *  if p == 1 -> 100% false positives — every query returns true, whether the element exists or not.
     *  m = -(n * ln(1)) / (ln(2))^2 = 0 bit array of size zero, empty vector, out of bounds crashes.
     
     ------------------------------------------------------------------------------------

     * if p < 0 -> A probability can never be negative — it has no mathematical meaning.
     * ln of a negative number is undefined in real numbers, it enters the complex number domain:
     * m_ would become NaN, and static_cast<size_t>(NaN) produces garbage or undefined behavior
     
     -----------------------------------------------------------------------------------------
     * If p > 1 -> a probability cannot exceed 1 — it has no real world meaning.
     * eg
     * m = -(n * ln(1.5)) / (ln(2))^2
     * ln(1.5) is a small positive number, so: m = negative number
     * m_ becomes negative, and casting a negative double to size_t (which is unsigned) 
     * wraps around to a huge garbage number 
     * program would try to allocate terabytes of memory and crash.
    
     ----------------------------------------------------------------------------------------

    *******************************************************************************************/

    if(falsePositiveRate <= 0.0 || falsePositiveRate >= 1.0)
        throw std::invalid_argument("falsePositiveRate must be between 0 and 1 exclusively");

    // calculate optimal m and k
    m_ = optimalBitArraySize(expectedElements, falsePositiveRate);
    k_ = optimalHashCount(m_, expectedElements);

    // Initialize bit array with all zeros
    bits_.assign(m_, false);

    // No elements inserted yet
    n_ = 0;
    
}

//--------------------Double Hashing-------------------------------------------------------

/****************************************************************************************** 
 * There is need of k bit positions for every key.
 * Instead of k separate hash functions , use double hashing
 * position_i = (h1(key) + i * h2(key)) % m
 * 
 * h1 uses FNV-1a 
 * a fast, non-cryptographic hash function designed for high dispersion and speed. 
 * It is widely used in hash tables, data integrity checks, and checksum calculations. 
 * Its unique design enables quick hashing of small data blocks with minimal collisions.
 * 
 * h2 uses a second FNV-1a pass with a different seed to get independence.
 * FNV-1a for each byte:
 * hash = (hash XOR byte) * FNV_prime
 * 
 * Why FNV-1a?
 * Simple to implement
 * Good distribution for strings
 *  No external libraries needed
 * 
 *******************************************************************************************/

size_t BloomFilter::hash(const std::string &key , size_t i) const {

    /***************************************************************************************
     * FNV-1a constants for 64-bit
     * They are mathematically derived and officially standardized as part of the FNV algorithm specification.
     ------------------------------------------------------------------------------------------
       FNV_prime 
     * FNV_prime  :- 2^40 + 2^8 + 0xb3 = 1099511628211
     * It must satisfy these conditions:
     * It must be a prime number — primes scatter bits more chaotically, reducing collisions
     * It must have a specific form 2^s + 2^8 + b 
     * this makes multiplication spread bits across all 64 positions efficiently
     * Different bit sizes (32, 64, 128) have their own specific primes — this one is specifically for 64-bit hashing
     * FNV_prime —> not a seed
     * It is a fixed mathematical constant used in every multiplication step
     * It doesn't change — it's part of the algorithm itself, not the starting state
     * 
     -------------------------------------------------------------------------------------------------
       FNV_offset
     * FNV_offset = 14695981039346656037 -> it is literally the FNV hash of a specific historical string:
     * Its purpose is to ensure that:
     * Hashing an empty string doesn't return zero  
     * The starting state is already "mixed up" before processing any bytes
     * FNV_offset —> acts like a seed
     * It is the starting value of the hash before any bytes are processed
     * It determines the initial state
     * In the code, h2 uses a modified seed FNV_offset ^ 0xdeadbeefcafeULL to make it independent from h1
     * 
     ---------------------------------------------------------------------------------------------------
     **************************************************************************************************/
    
    const uint64_t FNV_prime  = 1099511628211ULL;
    const uint64_t FNV_offset = 14695981039346656037ULL;

    // h1 — standard FNV-1a
    uint64_t h1 = FNV_offset ;
    for(unsigned char c : key){
        h1 ^= c  ;
        h1 *= FNV_prime ;
    }

    // h2 — FNV-1a with a different starting seed for independence
    uint64_t h2 = FNV_offset ^ 0xdeadbeefcafeULL;
    for(unsigned char c : key){
        h2 ^= c ;
        h2 *= FNV_prime ;
    }

    // Double hashing: combine h1 and h2 to produce the i-th position
    return static_cast<size_t>((h1 + i * h2) % m_);

}

//-------------------- INSERT --------------------------------------------------------------
void BloomFilter::insert(const std::string &key){
    // Compute k bit positions and set each one to 1
    for (size_t i = 0; i < k_; i++) {
        bits_[hash(key, i)] = true;
    } 
    n_++;  // track number of inserted elements
}

//-------------------Contains---------------------------------------------------------------
bool BloomFilter::contains(const std::string &key) const{
    // Check all k positions
    // If ANY bit is 0 → definitely not in set
    // If ALL bits are 1 → probably in set
    for (size_t i = 0; i < k_; i++) {
        if (!bits_[hash(key, i)]) return false ;
    }
    return true ;
}

//------------------Diagnostics---------------------------------------------------------------
/********************************************************************************************
 * Theoretical false positive rate formula: P = (1 - e^(-k * n / m)) ^ k
 * This tells : given how full the filter currently is (n_ elements),
 * what fraction of non-inserted keys would incorrectly return true?
 * The formula makes one assumption — that all k bit positions are independent of each other.
 * In reality they are slightly correlated because the same bit array is shared.
 * the real false positive rate may differ very slightly — hence theoretical, not exact.
 * The false positive rate grows as more elements are inserted
 * more n -> more bits flipped to 1 ->  
 * non-inserted keys more likely to find all their positions already 1 -> higher false positive rate
 * 
 ************************************************************************************************/

double BloomFilter::theoreticalFalsePositiveRate() const{
    // P = (1 - e^(-k * n / m))^k
    double exponent = - static_cast<double>(k_)* static_cast<double>(n_)/static_cast<double>(m_);
    return std::pow(1.0 - std::exp(exponent), static_cast<double>(k_));
}

size_t BloomFilter::setBitCount() const{
    // Count how many bits are currently set to 1
    return static_cast<size_t>(std::count(bits_.begin(), bits_.end(), true));
}

size_t BloomFilter::bitArraySize() const {
    return m_ ;
}

size_t BloomFilter::numHashFunctions() const {
    return k_ ;
}

size_t BloomFilter::elementCount() const {
    return n_;
}

