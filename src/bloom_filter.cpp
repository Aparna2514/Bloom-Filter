#include "bloom_filter.hpp"
#include <stdexcept> 
// Above header provides a set of predefined exception classes for reporting common error conditions.
// std::invalid_argument is logical error that could theoretically be caught at compile time 
#include <algorithm> // for count 

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

