#pragma once

#include <vector>
#include <stack>

class exception_list : public std::exception
{
public:
    typedef std::exception_ptr value_type;
    typedef const value_type& reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef std::vector<std::exception_ptr>::iterator iterator;
    typedef std::vector<std::exception_ptr>::const_iterator const_iterator;

    exception_list(std::vector<std::exception_ptr> exceptions) : exceptions_(std::move(exceptions)) {}

    size_t size() const {
        return exceptions_.size();
    }
    const_iterator begin() const {
        return exceptions_.begin();
    }
    const_iterator end() const {
        return exceptions_.end();
    }
private:
    std::vector<std::exception_ptr> exceptions_;
};
