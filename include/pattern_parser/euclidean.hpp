#pragma once

#include <iostream>
#include <string>
#include <vector>

std::string generate_euclidean_string(int num_hits, int len_sequence);
std::vector<std::vector<int>>
euclidean_recursor(std::vector<std::vector<int>> front,
                   std::vector<std::vector<int>> back);
