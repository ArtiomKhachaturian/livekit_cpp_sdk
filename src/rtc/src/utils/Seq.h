// Copyright 2025 Artiom Khachaturian
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once // Seq.h
#include <algorithm>
#include <iterator>
#include <set>

namespace LiveKitCpp
{

/**
 * @brief A utility class that provides methods for computing differences and intersections
 *        of non-sorted ranges. Unlike std::set_difference and std::set_intersection, these
 *        methods work with unsorted input ranges.
 *
 * @tparam T The type of elements in the input ranges.
 */
template <typename T>
class Seq
{
public:
    /**
     * @brief Computes the elements from the first range that are not found in the second range.
     *
     * @tparam TIn The template type of the input range (e.g., std::vector or other container).
     * @tparam TOut The template type of the output range (defaults to the same type as TIn).
     * @tparam TCompare The comparator used to compare elements (defaults to std::equal_to<T>).
     * @param range1 The first input range.
     * @param range2 The second input range.
     * @param compare The comparator to use for element comparison.
     * @return A range of elements that are in range1 but not in range2.
     */
    template <template <typename, typename...> class TIn,
              template <typename, typename...> class TOut = TIn,
              class TCompare = std::equal_to<T>>
    static TOut<T> difference(const TIn<T>& range1, const TIn<T>& range2,
                              TCompare compare = {}) {
        if (&range1 != &range2 && !range1.empty()) {
            if (range2.empty()) {
                return TOut<T>(std::begin(range1), std::end(range1));
            }
            const auto r2b = std::begin(range2);
            const auto r2e = std::end(range2);
            TOut<T> result;
            for (auto it = std::begin(range1), end = std::end(range1); it != end; ++it) {
                const auto res = std::find_if(r2b, r2e, [&it, &compare](const auto& r) {
                    return compare(*it, r);
                });
                if (r2e == res) {
                    result.push_back(*it);
                }
            }
            return result;
        }
        return {};
    }
    /**
     * @brief Computes the intersection of two input ranges, i.e., elements found in both ranges.
     *
     * @tparam TIn The template type of the input range (e.g., std::vector or other container).
     * @tparam TOut The template type of the output range (defaults to the same type as TIn).
     * @tparam TCompare The comparator used to compare elements (defaults to std::equal_to<T>).
     * @param range1 The first input range.
     * @param range2 The second input range.
     * @param compare The comparator to use for element comparison.
     * @return A range consisting of elements found in both range1 and range2.
     */
    template <template <typename, typename...> class TIn,
              template <typename, typename...> class TOut = TIn,
              class TCompare = std::equal_to<T>>
    static TOut<T> intersection(const TIn<T>& range1, const TIn<T>& range2,
                                TCompare compare = {}) {
        if (!range1.empty() && !range2.empty()) {
            if (&range1 == &range2) {
                return TOut<T>(std::begin(range1), std::end(range1));
            }
            TOut<T> result;
            auto b = std::begin(range2);
            auto e = std::end(range2);
            std::set<decltype(b)> skipped;
            // traverse over 1st input seq.
            for (auto it = std::begin(range1), end = std::end(range1); it != end; ++it) {
                const auto res = std::find_if(b, e, [&it, &compare](const auto& r) {
                    return compare(*it, r);
                });
                if (res != e) {
                    result.push_back(*res);
                    skipped.insert(res);
                }
            }
            // traverse over 2nd input seq.
            b = std::begin(range1);
            e = std::end(range1);
            for (auto it = std::begin(range2), end = std::end(range2); it != end; ++it) {
                if (0 == skipped.count(it)) {
                    const auto res = std::find_if(b, e, [&it, &compare](const auto& r) {
                        return compare(*it, r);
                    });
                    if (res != e) {
                        result.push_back(*res);
                    }
                }
            }
            return result;
        }
        return {};
    }
};

} // namespace LiveKitCpp
