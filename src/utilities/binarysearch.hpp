#pragma once

#include <vector>

namespace Utilities {

template <typename T>
int binarySearch(const std::vector<T> &vector, const T &value) {
    int low = 0;
    int high = static_cast<int>(vector.size()) - 1;

    while (low <= high) {
        auto mid = (low + high) / 2;
        if (value == vector[mid]) {
            return mid;
        } else if (value < vector[mid]) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    return -(low + 1);
}

template <typename T>
int binarySearchLowest(const std::vector<T> &vector, const T &value) {
    int low = 0;
    int high = static_cast<int>(vector.size()) - 1;

    while (low <= high) {
        auto mid = (low + high) / 2;
        if (vector[mid] == value) {
            return mid;
        } else if (vector[mid] < value) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return std::max(0, low - 1);
}
}