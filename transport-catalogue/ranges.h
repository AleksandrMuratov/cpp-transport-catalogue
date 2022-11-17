#pragma once

namespace ranges {

    template <typename It>
    class Range {
    public:
        using ValueType = typename std::iterator_traits<It>::value_type;

        Range(It begin, It end)
            : begin_(begin)
            , end_(end)
            , size_(std::distance(begin_, end_)) {
        }
        It begin() const {
            return begin_;
        }
        It end() const {
            return end_;
        }

        size_t size() const {
            return size_;
        }

    private:
        It begin_;
        It end_;
        size_t size_;
    };

    template <typename C>
    auto AsRange(const C& container) {
        return Range{ container.begin(), container.end() };
    }

}  // namespace ranges