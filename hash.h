#ifndef __HASH_H__
#define __HASH_H__

#include <functional>
#include <string>


const size_t HASHLIMIT = 0xFFFF;

size_t hash_fn(const std::string& str);

#endif