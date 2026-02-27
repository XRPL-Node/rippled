# SHAMap Module Best Practices

## Description
Use when working with the Merkle tree implementation in `src/libxrpl/shamap/` or `include/xrpl/shamap/`. Covers the SHA-256 based radix tree used for ledger state representation.

## Responsibility
Implements a SHA-256 based Merkle radix tree with 16-way branching (hexadecimal). Provides efficient ledger state representation with O(log N) proof-of-inclusion, copy-on-write snapshots, synchronization, and delta calculation.

## Key Patterns

### State Machine Lifecycle
```cpp
enum class SHAMapState { Modifying, Immutable, Synching, Invalid };
// Modifying: Can add/remove/modify entries
// Immutable: Snapshot - no modifications allowed
// Synching: Being synchronized from peers
// Invalid: Corrupted or destroyed
// Transitions are one-way: Modifying -> Immutable (via snapshot)
```

### Copy-on-Write via COWID
```cpp
std::uint32_t cowid_ = 1;
// Each snapshot gets a new COWID
// Children are only copied when modified (lazy copy)
// Immutable snapshots share nodes with the active map
auto snapshot = map.snapShot(true);  // Creates immutable copy
```

### 16-Way Radix Branching
```cpp
static inline constexpr unsigned int branchFactor = 16;
static inline constexpr unsigned int leafDepth = 64;
// 256-bit key / 4 bits per nibble = 64 levels max
// Each inner node has up to 16 children
```

### Node Type Hierarchy
```cpp
// SHAMapTreeNode (base) -> SHAMapInnerNode (16 children)
//                       -> SHAMapLeafNode (data + key)
// Use dynamic_pointer_cast for safe downcasting
auto inner = intr_ptr::dynamic_pointer_cast<SHAMapInnerNode>(node);
if (inner) { /* process inner node */ }
```

### Walk-Up for Modifications (dirtyUp)
```cpp
// After modifying a leaf, propagate hash changes to root
void dirtyUp(SharedPtrNodeStack& stack, uint256 const& target, ...);
// Updates hashes from leaf -> root through the stack
```

### Lazy Traversal
```cpp
// Navigate to a leaf by key
SHAMapLeafNode* walkTowardsKey(uint256 const& id,
    SharedPtrNodeStack* stack = nullptr);
// Returns leaf or nullptr; optionally records path for dirtyUp
```

### Intrusive Pointers for Nodes
```cpp
// Nodes use intrusive reference counting (not std::shared_ptr)
intr_ptr::SharedPtr<SHAMapTreeNode> root_;
// Minimal overhead - reference count embedded in node
```

## Common Pitfalls
- Never modify an Immutable map - check state before mutations
- Always call dirtyUp after modifying a leaf to maintain hash integrity
- Never assume node type without dynamic_pointer_cast check
- Remember that snapshots share nodes - COW means nodes are only copied on write
- Always use the provided iterators for safe traversal, not manual tree walking

## Key Files
- `include/xrpl/shamap/SHAMap.h` - Main map implementation
- `include/xrpl/shamap/SHAMapTreeNode.h` - Base node class
- `include/xrpl/shamap/SHAMapInnerNode.h` - Interior nodes (16-way)
- `include/xrpl/shamap/SHAMapLeafNode.h` - Leaf nodes with data
- `include/xrpl/shamap/SHAMapNodeID.h` - Tree position identifier
- `src/libxrpl/shamap/SHAMapDelta.cpp` - Difference between maps
- `src/libxrpl/shamap/SHAMapSync.cpp` - Synchronization algorithm
