#pragma once
#include <string>
#include <iterator>

template <typename T>
class List;

template <typename T>
class Node {
    friend class List<T>;
    T data;
    [[nodiscard]] std::string _toString(std::string str) const {
        str += data;
        if (next != nullptr) return next->_toString(str);
        return str;
    }
    [[nodiscard]] std::string _toString(std::string str, const std::string& sep) const {
        str += data;
        if (next->next != nullptr) return next->_toString(str + sep, sep);
        return str;
    }
public:
    Node* next = nullptr;
    Node() = default;
    explicit Node(T& data) : data(data) {}
    T& operator*() { return data; }
    [[nodiscard]] std::string toString() const { return this->_toString(""); };
    [[nodiscard]] std::string toString(const std::string& sep) const { return this->_toString("", sep); };
};

template <typename T>
class Iter {
    Node<T>* pos;

public:
    explicit Iter(Node<T>* pos) : pos(pos) {}
    Iter() : pos(nullptr) {}
    explicit Iter(const Node<T>* pos) : pos(pos) {}
    using difference_type = std::ptrdiff_t;
    using value_type = T;

    Iter& operator++() { pos = pos->next; return *this; }

    Iter operator++(int) {
        Iter tmp(pos);
        ++pos;
        return tmp;
    }

    [[nodiscard]] Node<T> *peek() const { return pos->next; }

    bool operator==(const Iter & end) const { return end.pos == this->pos; }
    T& operator*() const { return **pos; }

    [[nodiscard]] std::string toString() const { return pos->toString(); }
    [[nodiscard]] std::string toString(const std::string& sep) const { return pos->toString(sep); };
};

static_assert(std::forward_iterator<Iter<std::string>>);

template <typename T>
class List {
    Node<T>* head = nullptr;
    Node<T>* tail = nullptr;

    T* (List::*_append)(T&) = &List::append_first;

    T* append_first(T &value) {
        head = new Node<T>(value);
        tail = head->next = new Node<T>();
        _append = &List::append_subsequent;
        return &head->data;
    }

    T* append_subsequent(T &value) {
        **tail = value;
        T *ptr = &tail->data;
        tail = tail->next = new Node<T>();
        return ptr;
    }

public:
    List() = default;
    Iter<T> begin() const { return Iter(head); }
    Iter<T> end() const { return Iter<T>(tail); }
    T* Append(T value) {
        return (this->*_append)(value);
    }

    [[nodiscard]] std::string toString() const { return head->toString(); }
    [[nodiscard]] std::string toString(const std::string& sep) const { return head->toString(sep); };
};