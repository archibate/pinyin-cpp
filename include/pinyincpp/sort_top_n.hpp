#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <algorithm>

namespace pinyincpp {

template <class T, class KeyFunc>
void sortTopN(std::vector<T> &arr, std::size_t num, KeyFunc &&keyFunc) {
    num = std::min(num, arr.size());
    std::partial_sort(arr.begin(), arr.begin() + num, arr.end(), [keyFunc](const T &a, const T &b) {
        return std::invoke(keyFunc, a) > std::invoke(keyFunc, b);
    });
    arr.resize(num);
}

}
