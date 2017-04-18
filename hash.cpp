#include "hash.h"

size_t hash_fn(const std::string& str)
{
	std::hash<std::string> hash_str;

	return hash_str(str) & HASHLIMIT;
}