#ifndef _SC_UTILS_GENERATOR_H_
#define _SC_UTILS_GENERATOR_H_

#include <atomic>
#include <random>
#include <cstdint>
#include <string>

/*!
 * \brief: distribution generator base class
 */
template <typename T>
class sc_utils_distribution_generator {
    public:
        virtual T next() = 0;
        virtual T last() = 0;
        virtual ~sc_utils_distribution_generator() { }
};

/*!
 * \brief   uniform distribution generator
 * \param   min the minimum generated value (inclusive)
 * \param   max the maximum generated value (inclusive)
 */
template <typename T>
class sc_utils_uniform_distribution_generator : public sc_utils_distribution_generator<T> {
 public:
  sc_utils_uniform_distribution_generator(uint64_t min, uint64_t max) : dist_(min, max) { next(); }
  
  T next();
  T last();
  
 private:
  std::mt19937_64 generator_;
  std::uniform_int_distribution<T> dist_;
  uint64_t last_int_;
};


template<typename T>
class sc_utils_bimodal_distribution_generator : public sc_utils_distribution_generator<T> {
 public:
  using normal_dist   = std::normal_distribution<T>;
  using discrete_dist = std::discrete_distribution<std::size_t>;

  std::array<normal_dist, 2> G_;
  std::discrete_distribution<std::size_t> w_;

  sc_utils_bimodal_distribution_generator(uint64_t first_modal, double first_prob, uint64_t second_modal, double second_prob) {
    G_ = std::array<normal_dist, 2>{
        normal_dist{(double)first_modal, 0.1},  // mean, stddev of G[0]
        normal_dist{(double)second_modal, 0.1}, // mean, stddev of G[1]
    };

    w_ = discrete_dist{
        first_prob,     // weight of G[0]
        second_prob,    // weight of G[1]
    };
  }
  
  T next();
  T last();

private:
  std::mt19937_64 generator_;
  T last_int_;
};

#endif
