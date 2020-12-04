#pragma once
#include <vector>

template<typename T>
struct flat_heap {

    std::vector<std::vector<T> > contents;
    int64_t elements;
    size_t x;

    flat_heap() : elements(0), x(0) { }

    void push(size_t depth, T &it) {

        if (contents.size() <= depth) {
            contents.resize(depth+1);
        }

        contents[depth].push_back(it);

        if (x > depth) { x = depth; }

        elements += 1;
    }

    T pop() {

        while (contents[x].empty()) {
            x += 1;
        }

        T element = contents[x].back();
        contents[x].pop_back();

        if (contents[x].empty()) {
            std::vector<T> y;
            contents[x].swap(y);
        }

        elements -= 1;
        return element;
    }
};

template<typename T>
struct double_heap {

    std::vector<flat_heap<T> > contents;
    int64_t elements;
    size_t x;

    double_heap() : elements(0), x(0) { }

    void push(size_t breadth, size_t depth, T &it) {

        if (contents.size() <= breadth) {
            contents.resize(breadth + 1);
        }

        contents[breadth].push(depth, it);

        if (x > breadth) { x = breadth; }

        elements += 1;
    }

    T pop() {

        while (contents[x].elements == 0) { x += 1; }
        elements -= 1;
        return contents[x].pop();
    }
};
