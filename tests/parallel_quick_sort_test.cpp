// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include "parallel_quick_sort.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <random>
#include <vector>

/** @brief Sorting an empty range is a no-op. */
void empty_range_test() {
    std::vector<int> v;
    parallel_quick_sort(v.begin(), v.end());
    assert(v.empty());
}

/** @brief Sorting a single element is a no-op. */
void single_element_test() {
    std::vector<int> v = {42};
    parallel_quick_sort(v.begin(), v.end());
    assert(v.size() == 1 && v[0] == 42);
}

/** @brief Sorting an already-sorted range preserves the order. */
void already_sorted_test() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};
    parallel_quick_sort(v.begin(), v.end());
    assert(std::is_sorted(v.begin(), v.end()));
}

/** @brief Sorting a reverse-sorted range produces the correct order. */
void reverse_sorted_test() {
    std::vector<int> v = {8, 7, 6, 5, 4, 3, 2, 1};
    parallel_quick_sort(v.begin(), v.end());
    assert(std::is_sorted(v.begin(), v.end()));
}

/** @brief Sorting a range with many duplicates produces a sorted result. */
void duplicates_test() {
    std::vector<int> v = {5, 3, 5, 1, 3, 5, 1, 3, 5, 1};
    parallel_quick_sort(v.begin(), v.end());
    assert(std::is_sorted(v.begin(), v.end()));
}

/** @brief Sorting all-equal elements is a no-op (already sorted). */
void all_equal_test() {
    std::vector<int> v(100, 7);
    parallel_quick_sort(v.begin(), v.end());
    assert(std::is_sorted(v.begin(), v.end()));
    assert(v.front() == 7 && v.back() == 7);
}

/** @brief Sorting a large random vector produces a sorted result. */
void large_random_test() {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 100000);
    std::vector<int> v(5000);
    for (auto& x : v) x = dist(rng);

    parallel_quick_sort(v.begin(), v.end());
    assert(std::is_sorted(v.begin(), v.end()));
}

/** @brief Sorting preserves all original elements (permutation check). */
void preserves_elements_test() {
    std::vector<int> original = {9, 1, 8, 2, 7, 3, 6, 4, 5, 0};
    std::vector<int> sorted = original;
    parallel_quick_sort(sorted.begin(), sorted.end());

    std::vector<int> expected = original;
    std::sort(expected.begin(), expected.end());
    assert(sorted == expected);
}

/** @brief Sorting with floating-point values works correctly. */
void float_sort_test() {
    std::vector<double> v = {3.14, 1.41, 2.72, 0.58, 1.73};
    parallel_quick_sort(v.begin(), v.end());
    assert(std::is_sorted(v.begin(), v.end()));
}

int main() {
    empty_range_test();
    single_element_test();
    already_sorted_test();
    reverse_sorted_test();
    duplicates_test();
    all_equal_test();
    large_random_test();
    preserves_elements_test();
    float_sort_test();
    return 0;
}
