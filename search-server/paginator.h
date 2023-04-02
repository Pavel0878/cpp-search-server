#pragma once

#include <iterator>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:

    IteratorRange(const Iterator& begin, const Iterator& end, size_t page_size)
    {
        begin_ = begin;
        end_ = end;
        size_ = page_size;
    }

    Iterator begin() {
        advance(begin_, size_);
        return begin_;
    }

    Iterator end() {
        return end_;
    }

    int size() {
        return size_;
    }

private:
    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(const Iterator& begin, const Iterator& end, size_t page_size)
        :size_page(static_cast<int>(page_size)) {
        begin_ = begin;
        int size_documents = static_cast<int>(distance(begin, end));
        for (int i = 0; i < size_documents; i = i + size_page) {
            int remains_documents = (size_documents - i);
            if (remains_documents < size_page) {
                IteratorRange<Iterator>page(begin, end, i);
                pages_.push_back(page);
            }
            else {
                advance(begin_, size_page);
                IteratorRange<Iterator>page(begin, begin_, i);
                pages_.push_back(page);
            }
        }
    }

    auto begin() const { return pages_.begin(); }

    auto end() const { return pages_.end(); }

private:
    Iterator begin_;
    int size_page;
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, IteratorRange<Iterator> page) {
    for (auto it = page.begin(); it != page.end(); ++it) {
        out << *it;
    }
    return out;
}