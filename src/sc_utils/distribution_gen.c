#include "sc_utils/distribution_gen.h"

/*!
 * \brief   generate a new value based on uniform distribution
 * \return  
 */
template<typename T>
inline T sc_utils_uniform_distribution_generator<T>::next() {
  return last_int_ = dist_(generator_);
}

/*!
 * \brief   obtain the last generated value
 */
template<typename T>
inline T sc_utils_uniform_distribution_generator<T>::last() {
  return last_int_;
}

inline uint64_t BimodalGenerator::Next() {
  auto index = w_(generator_);
  auto sample = G_[index](generator_);
  return (uint64_t)ceil(sample);
}

inline uint64_t BimodalGenerator::Last() {
  return last_int_;
}