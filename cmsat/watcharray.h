#ifndef __WATCHARRAY_H__
#define __WATCHARRAY_H__

#include "watched.h"
#include <vector>

namespace CMSat {
using namespace CMSat;
using std::vector;

struct watch_subarray
{
    explicit watch_subarray(vector<Watched>& _array) :
        array(_array)
    {}

    vector<Watched>& array;
    Watched& operator[](const size_t at)
    {
        return array[at];
    }

    void clear()
    {
        array.clear();
    }

    size_t size() const
    {
        return array.size();
    }

    bool empty() const
    {
        return array.empty();
    }

    Watched* begin()
    {
        return array.data();
    }

    Watched* end()
    {
        return array.data()+array.size();
    }

    const Watched* begin() const
    {
        return array.data();
    }

    const Watched* end() const
    {
        return array.data()+array.size();
    }

    void shrink(const size_t num)
    {
        array.resize(array.size()-num);
    }

    void shrink_(const size_t num)
    {
        shrink(num);
    }

    void push(const Watched& watched)
    {
        array.push_back(watched);
    }

    /*watch_subarray* operator->()
    {
        return this;
    }*/

    typedef Watched* iterator;
    typedef const Watched* const_iterator;
};

struct watch_subarray_const
{
    explicit watch_subarray_const(const vector<Watched>& _array) :
        array(_array)
    {}

    const vector<Watched>& array;

    watch_subarray_const(const watch_subarray& other) :
        array(other.array)
    {}

    const Watched& operator[](const size_t at) const
    {
        return array[at];
    }

    size_t size() const
    {
        return array.size();
    }

    bool empty() const
    {
        return array.empty();
    }

    const Watched* begin() const
    {
        return array.data();
    }

    const Watched* end() const
    {
        return array.data() + array.size();
    }

    typedef const Watched* const_iterator;

    /*watch_subarray_const* operator->()
    {
        return this;
    }*/
};

struct watch_array
{
    vector<vector<Watched> > watches;

    watch_subarray operator[](size_t at)
    {
        return watch_subarray(watches[at]);
    }

    watch_subarray_const operator[](size_t at) const
    {
        return watch_subarray_const(watches[at]);
    }

    void resize(const size_t new_size)
    {
        watches.resize(new_size);
    }

    size_t memUsed() const
    {
        size_t mem = watches.capacity()*sizeof(vector<Watched>);
        for(size_t i = 0; i < watches.size(); i++) {
            mem += watches[i].capacity()*sizeof(Watched);
        }
        return mem;
    }

    size_t size() const
    {
        return watches.size();
    }

    void shrink_to_fit()
    {
        watches.shrink_to_fit();
    }

    void prefetch(const size_t at) const
    {
        __builtin_prefetch(watches[at].data());
    }

    struct iterator
    {
        vector<vector<Watched> >::iterator it;
        explicit iterator(vector<vector<Watched> >::iterator _it) :
            it(_it)
        {}

        iterator operator++()
        {
            ++it;
            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp = *this;
            ++it;
            return tmp;
        }

        watch_subarray operator*()
        {
            return watch_subarray(*it);
        }

        bool operator==(const iterator& it2) const
        {
            return it == it2.it;
        }

        bool operator!=(const iterator& it2) const
        {
            return it != it2.it;
        }

        /*watch_subarray operator->()
        {
            return watch_subarray(*it);
        }*/

        friend size_t operator-(const iterator& lhs, const iterator& rhs);
    };

    struct const_iterator
    {
        vector<vector<Watched> >::const_iterator it;
        explicit const_iterator(vector<vector<Watched> >::const_iterator _it) :
            it(_it)
        {}

        const_iterator(const iterator& other) :
            it(other.it)
        {}

        const_iterator operator++()
        {
            ++it;
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            ++it;
            return tmp;
        }

        const watch_subarray_const operator*() const
        {
            return watch_subarray_const(*it);
        }

        bool operator==(const const_iterator& it2) const
        {
            return it == it2.it;
        }

        bool operator!=(const const_iterator& it2) const
        {
            return it != it2.it;
        }

        /*const watch_subarray_const operator->() const
        {
            return watch_subarray_const(*it);
        }*/

        friend size_t operator-(const const_iterator& lhs, const const_iterator& rhs);
    };

    iterator begin()
    {
        return iterator(watches.begin());
    }

    iterator end()
    {
        return iterator(watches.end());
    }

    const_iterator begin() const
    {
        return const_iterator(watches.begin());
    }

    const_iterator end() const
    {
        return const_iterator(watches.end());
    }

    void fitToSize()
    {
        //TODO shirnk
    }
};

inline size_t operator-(const watch_array::iterator& lhs, const watch_array::iterator& rhs)
{
    return lhs.it-rhs.it;
}

inline size_t operator-(const watch_array::const_iterator& lhs, const watch_array::const_iterator& rhs)
{
    return lhs.it-rhs.it;
}

inline void swap(watch_subarray a, watch_subarray b)
{
    a.array.swap(b.array);
}


} //End of namespace

#endif //__WATCHARRAY_H__