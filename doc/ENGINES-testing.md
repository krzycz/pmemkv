# Testing Storage Engines for pmemkv

- [dram_vcmap](#dram_vcmap)
- [dram_vcsmap](dram_vcsmap)

# dram_vcmap

A volatile concurrent engine backed by std::allocator. Data written using this engine is stored entirely in DRAM and lost after the database is closed.

This engine is variant of vcmap, which uses std::allocator to allocate memory. That means it is built on top of tbb::concurrent\_hash\_map data structure; std::basic\_string is used as a type of a key and a value.

## Requirements:
	* TBB package.
	* This engine does not require any config parameters.

# dram_vcsmap

This engine is a thin wrapper over tbb::concurrent_map data structure, which uses default dram allocator. It implements only bare minimum subset of pmemkv API.

## Requirements:
	* TBB package.
	* This engine does not require any config parameters.

