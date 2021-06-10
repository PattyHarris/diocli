//
//
// Generic Dictionary Class
//
// Author: Geoff Wong
// Note: Generic Zikzak library code.
//
// NOTES:
//   should extend to hash generic keys (need function to
//   extract key from elements being stored
//
//   * should do something other than return '0' on a failed find
//     (need a special exception value)
//
//   Should fix copy constringuctor so Dict<Blah> X = Y; works nicely.
//   Should fix copy constringuctor so Dict<Blah>& X = Y; works nicely.
//
//   Use (example):
//   Dictionary<string *> dict;
//   dict["test"] = new string("fud");
//   x = dict["test"];
//
//   Uses a hash table internally for O(1) lookups.
//   Additions are O(1)/O(n) - the table initially starts
//   off with 317 elements but automagically resizes when it is 70% full
//
#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Stdafx.h"

#include <assert.h>
#include <stdio.h>
#include <string>

template <class T> class Dictionary;

template <class T>
class Dictionary
{
    unsigned int gap;

    int cacheIndex;
    // the index of the last bucket accessed.

protected:
    //        int last_full;		// last full bucket

public:

    class Bucket
    {
    public:
        std::string key;
        T element;
        bool used;
        bool deleted;
    };

    int deleted;
    Bucket * hashtable;
    unsigned int hsize;
    int elements;

    int find(const std::string& key)
        // returns the actual index in the hash table - internal use only!
        // returns -1 on failure to find
    {
        unsigned int h;

        if (key.size() == 0)
            return 0;
        h = hash(key);

        while (gap < hsize * 3)
        {
            if ((hashtable[h].used == true) &&
                (hashtable[h].deleted == false) &&
                (hashtable[h].key == key))
            {
                /* function found */
                assert(h < hsize);
                cacheIndex = h;
                return h;
            }
            if (hashtable[h].used==false && hashtable[h].deleted==false)
            {
                return -1;
            }
            h = (h + gap) % hsize;
        }
        return -1;
    }

    int add_element(const std::string& key, T ele)
    {
        int h;

        if (hashtable == NULL) return -1;
        if (key.size() == 0) return -1;

        if ((h = find(key)) != -1)
        {
            // no duplicate keys - don't add just return the index
            return h;
        }

        /* check to increase size here! */
        if (((elements + deleted) * 1000 / hsize) > 701)
        {
            // if this happens we're out of memory probably!
            if (upsize() == false) return -1;
        }

        h = hash(key);
        while ((hashtable[h]).used == true)
        {
            h = (h+gap) % hsize;
        }
        hashtable[h].key = key;
        hashtable[h].used = true;
        if (hashtable[h].deleted == true)
            deleted--;
        hashtable[h].deleted = false;
        hashtable[h].element = ele;
        elements++;

        assert((unsigned int)h < hsize);

        return h;
    }

    class iterator
    {
    public:
        Dictionary<T> * ihash;
        unsigned int where;

        iterator()
        {
            ihash = NULL;
            where = 0;
        }

        iterator(Dictionary<T> * t,  int w)
        {
            ihash = t;
            where = w;
        }

        iterator& operator++(int)
        {
            if (ihash == NULL) return *this;

            do {
                where++;
            } while (where < ihash->hsize &&
                     ((ihash->hashtable[where].used == false)
                      || (ihash->hashtable[where].deleted == true)));

            //                                if (where == ihash->hsize) where = 0;
            return *this;
        }

        T& element()
        {
            assert(ihash != NULL);
            //                        if (ihash == NULL) return NULL;
            return ihash->hashtable[where].element;
        }

        T& operator*()
        {
            return element();
        }

        std::string key()
        {
            if (ihash == NULL) return std::string(_T(""));
            return ihash->hashtable[where].key.c_str();
        }

        int operator==(iterator &it1)
        {
            if ((it1.where == where)
                && (it1.ihash == ihash)) return 1;
            return 0;
        }

        //                friend int operator==(iterator& it1, iterator& it2);
    };

    iterator nulliterator;

    // NAME: hash(std::string)
    // NOTES: computes a hash value from the first 8 chars of
    // 	      a null terminated C string.
    int hash(const std::string& szString)
    {
        int i = 0;
        int h = 0;
        while (szString[i] && i < 8) {
            h = (h << 2) + szString[i];
            i++;
        }
        // fprintf(stderr, "hash(%s) = %d\n", szString, h % hsize);
        return h % hsize;
    }

    iterator begin()
    {
        // find first full bucket and return
        unsigned int i = 0;
        if (elements == 0)
            return nulliterator;
        while (i < hsize)
        {
            if (hashtable[i].used == true &&
                hashtable[i].deleted == false)
            {
                return iterator(this, i);
            }
            i++;
        }
        return nulliterator;
    }

    iterator end()
    {
        // remember last full bucket and return
        if (elements == 0)
            return nulliterator;
        return iterator(this, hsize);
    }

    bool upsize()
    {
        // fprintf(stderr, "Upsizing: %d -> ", hsize);
        unsigned int hash_primes[] = {
            37, 93, 193, 317, 701,
            1279, 2557, 5119, 10223, 16001, 23001, 40939,
            81919, 163819, 371001, 707001, 29000039
        };
        // more primes: 11, 19, 37, 79, 157, 317, 431, 631, 20479,

        unsigned int old_size;
        unsigned int i;
        Bucket * old_table;

        for (i = 0; i < sizeof(hash_primes); i++)
        {
            if (hash_primes[i] > hsize) break;
        }

        if (i > sizeof(hash_primes))
        {
            /* no more primes :-( */
            /* btw: that'd be a HUGE error I expect */
            // fprintf(stderr, "No more hash table space\n");
            return false;
        }
        old_table = hashtable;
        old_size = hsize;
        gap = old_size;

        hsize = hash_primes[i];
        // fprintf(stderr, "%d\n", hsize);

        hashtable = new Bucket[hsize];
#if 1
        // initialisation
        for (i = 0; i < hsize; i++)
        {
            hashtable[i].used = false;
            hashtable[i].deleted = false;
            hashtable[i].key[0] = _T('\0');
            hashtable[i].element = T();
        }
#endif
        elements = 0;
        deleted = 0;

        /* ugly time - go through each element an add to the new table */
        /* how can we do this when we don't know what the "keys" are? */
        for (i = 0; i < old_size; i++)
        {
            if (old_table[i].used == true && old_table[i].deleted == false)
            {
                add_element(old_table[i].key.c_str(), old_table[i].element);
            }

            old_table[i].key[0] = _T('\0');
        }
        delete [] old_table;
        return true;
    }


    bool remove(const std::string& key)
    {
        int h;
        if (hashtable == NULL) return false;

        h = hash(key);

        while (gap < hsize * 3)
        {
            if ((hashtable[h].used == true) &&
                (hashtable[h].deleted == false) &&
                (hashtable[h].key == key))
            {
                hashtable[h].key[0] = _T('\0');
                hashtable[h].deleted = true;
                hashtable[h].used = false;
                hashtable[h].element = T();
                elements--;
                deleted++;
                return true;
            }
            if (hashtable[h].used==false && hashtable[h].deleted==false)
            {
                return false;
            }
            h = (h + gap) % hsize;
        }
        return false;
    }

#if 0
    bool remove(std::string &key)
    {
        return remove(key.c_str());
    }
#endif

    void clear_all_elements()
    {
        for (unsigned int i = 0; i < hsize; i++)
        {
#if 0
            if (hashtable[i].used == true && hashtable[i].deleted == false)
            {
                hashtable[i].element = T();
                // destroy(&(hashtable[i].element));
                // delete hashtable[i].element;
            }
#endif
            hashtable[i].element = T();
            hashtable[i].used = false;
            hashtable[i].deleted = false;
            hashtable[i].key[0] = _T('\0');
        }
        elements = 0;
        deleted = 0;
    }

    bool exists(const std::string& key)
    {
        if (find(key) == -1)
            return false;
        return true;
    }

    T * element_array()
    {
        T * ret;
        unsigned int i, count = 0;

        if (elements == 0) return NULL;

        ret = new T[elements];
        for (i = 0; i < hsize; i++)
        {
            if (hashtable[i].used == true &&
                hashtable[i].deleted == false)
            {
                ret[count++] = hashtable[i].element;
            }
        }
        return ret;
    }

    T& operator[](const std::string& szString)
    {
        if (cacheIndex != -1 && hashtable[cacheIndex].key == szString)
        {
            return hashtable[cacheIndex].element;
        }

        int h;

        // add it (with NULL value - risk with integers)
        h = add_element(szString, T());
        assert(h != -1);
        assert((unsigned int)h < hsize);
        cacheIndex = h;

        return hashtable[h].element;
    }

    int size() { return hsize; }
    int num_elements() { return elements; }

    Dictionary()
    {
        cacheIndex = -1;
        gap = 157;
        hsize = 317;
        hashtable = new Bucket[hsize];
#if 1
        for (unsigned int i = 0; i < hsize; i++)
        {
            hashtable[i].used = false;
            hashtable[i].deleted = false;
            hashtable[i].key[0] = _T('\0');
            hashtable[i].element = T();
        }
#endif
        elements = 0;
        deleted = 0;
    }

    Dictionary(int nsize)
    {
        hsize = nsize;                // should make sure it's the next largest prime
        hashtable = new Bucket[hsize];
#if 1
        for (unsigned int i = 0; i < hsize; i++)
        {
            hashtable[i].used = false;
            hashtable[i].deleted = false;
            hashtable[i].key[0] = _T('\0');
            hashtable[i].element = T();
        }
#endif
        elements = 0;
        deleted = 0;
    }

    ~Dictionary()
    {
        // clear_all_elements();
        delete [] hashtable;
    }
};



#if 0
template <class T>
int operator==(const Dictionary<T>::iterator& it1,
               const Dictionary<T>::iterator& it2)
{
    if ((it1.where == it2.where)
        && (it1.ihash == it2.ihash)) return 1;
    return 0;
}
#endif

#endif //__DICTIONARY_H__

