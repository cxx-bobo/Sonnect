#ifndef _SC_UTILS_DISTRIBUTION_GENERATOR_H_
#define _SC_UTILS_DISTRIBUTION_GENERATOR_H_

#include <atomic>
#include <random>
#include <array>
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
 * \brief   expose the class with certain template parameters to C runtime
 */
#ifdef __cplusplus
    extern "C" {
#endif
        using sc_utils_distribution_uint64_generator = sc_utils_distribution_generator<uint64_t>;
#ifdef __cplusplus
    }
#endif

/*!
 * \brief   exponential distribution generator
 * \param   exp_mean    the mean value of the desired exponential distribution
 */
template<typename T>
class sc_util_exponential_generator : public sc_utils_distribution_generator<T> {
 public:
    sc_util_exponential_generator(int exp_mean) {
        double mean = (double)exp_mean;
        double dist_lambda = 1 / mean;
        G_ = std::exponential_distribution<>{dist_lambda};
        generator_ = std::default_random_engine{};
    }

    T next();
    T last();

 private:  
    std::exponential_distribution<> G_;
    std::default_random_engine generator_;
    T last_generated_;
};

/*!
 * \brief   generate a new value based on uniform distribution
 * \return  the generated new value
 */
template<typename T>
T sc_util_exponential_generator<T>::next() {
    return last_generated_ = (T)(ceil(G_(generator_)));
}

/*!
 * \brief   obtain the last generated value
 * \return  the last generated value
 */
template<typename T>
T sc_util_exponential_generator<T>::last() {
    return last_generated_;
}

/*!
 * \brief   expose the class with certain template parameters to C runtime
 */
#ifdef __cplusplus
    extern "C" {
#endif
        using sc_util_exponential_uint64_generator = sc_util_exponential_generator<uint64_t>;
#ifdef __cplusplus
    }
#endif

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
  T last_generated_;
};

/*!
 * \brief   generate a new value based on uniform distribution
 * \return  the generated new value
 */
template<typename T>
inline T sc_utils_uniform_distribution_generator<T>::next() {
  return last_generated_ = dist_(generator_);
}

/*!
 * \brief   obtain the last generated value
 * \return  the last generated value
 */
template<typename T>
inline T sc_utils_uniform_distribution_generator<T>::last() {
  return last_generated_;
}

/*!
 * \brief   expose the class with certain template parameters to C runtime
 */
#ifdef __cplusplus
    extern "C" {
#endif
        using sc_utils_uniform_distribution_uint64_generator = sc_utils_uniform_distribution_generator<uint64_t>;
#ifdef __cplusplus
    }
#endif

/*!
 * \brief   bimodal distribution generator
 * \param   first_modal     ?
 * \param   first_prob      ?
 * \param   second_modal    ?
 * \param   second_prob     ?
 */
template<typename T>
class sc_utils_bimodal_distribution_generator : public sc_utils_distribution_generator<T> {
 public:
  using normal_dist   = std::normal_distribution<>;
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
  T last_generated_;
};

/*!
 * \brief   generate a new value based on bimodal distribution
 * \return  
 */
template<typename T>
inline T sc_utils_bimodal_distribution_generator<T>::next() {
  auto index = w_(generator_);
  auto sample = G_[index](generator_);
  return last_generated_ = (T)ceil(sample);
}

/*!
 * \brief   obtain the last generated value
 */
template<typename T>
inline T sc_utils_bimodal_distribution_generator<T>::last() {
  return last_generated_;
}

/*!
 * \brief   expose the class with certain template parameters to C runtime
 */
#ifdef __cplusplus
    extern "C" {
#endif
        using sc_utils_bimodal_distribution_uint64_generator = sc_utils_bimodal_distribution_generator<uint64_t>;
#ifdef __cplusplus
    }
#endif

#endif
