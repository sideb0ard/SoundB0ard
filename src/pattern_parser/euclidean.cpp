#include <pattern_parser/euclidean.hpp>
#include <sstream>
#include <vector>

// code inspired from implementation at
// https://medium.com/code-music-noise/euclidean-rhythms-391d879494df
// I take full responsibility for the careless throwing around of vectors of
// vectors however!

namespace {

std::string euclidean_vector_to_string(std::vector<std::vector<int>> &vec) {
  std::stringstream ss;
  int seq_size = vec.size();
  for (int i = 0; i < seq_size; i++) {
    int inner_vec_size = vec[i].size();
    for (int j = 0; j < inner_vec_size; j++) ss << vec[i][j];
  }

  return ss.str();
}

std::vector<std::vector<int>> cat_vector_vector(
    std::vector<std::vector<int>> &front_vec,
    std::vector<std::vector<int>> &back_vec) {
  std::vector<std::vector<int>> ret;
  for (auto i : front_vec) ret.push_back(i);
  for (auto i : back_vec) ret.push_back(i);

  return ret;
}

std::vector<int> cat_flatten_vector(std::vector<int> &front_vec,
                                    std::vector<int> &back_vec) {
  std::vector<int> ret;
  for (auto i : front_vec) ret.push_back(i);
  for (auto i : back_vec) ret.push_back(i);

  return ret;
}

}  // namespace
std::string generate_euclidean_string(int num_hits, int len_sequence) {
  std::vector<std::vector<int>> front_vec;
  std::vector<std::vector<int>> back_vec;

  for (int i = 0; i < num_hits; i++) front_vec.push_back(std::vector{1});

  for (int i = num_hits; i < len_sequence; i++)
    back_vec.push_back(std::vector{0});

  std::vector<std::vector<int>> euclidean_sequence =
      euclidean_recursor(front_vec, back_vec);

  return euclidean_vector_to_string(euclidean_sequence);
}

std::vector<std::vector<int>> euclidean_recursor(
    std::vector<std::vector<int>> front_vec,
    std::vector<std::vector<int>> back_vec) {
  if (back_vec.size() < 2) {
    for (auto &v : back_vec) front_vec.push_back(v);
    return front_vec;
  }

  std::vector<std::vector<int>> new_front_vec;
  while (front_vec.size() > 0 && back_vec.size() > 0) {
    new_front_vec.push_back(
        cat_flatten_vector(front_vec.back(), back_vec.back()));
    front_vec.pop_back();
    back_vec.pop_back();
  }

  return euclidean_recursor(new_front_vec,
                            cat_vector_vector(front_vec, back_vec));
}
