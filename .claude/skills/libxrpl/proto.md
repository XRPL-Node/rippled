# Proto Module Best Practices

## Description
Use when working with Protocol Buffer definitions in `include/xrpl/proto/`. Covers gRPC API definitions and generated protobuf headers.

## Responsibility
Contains Protocol Buffer generated headers defining the gRPC API for rippled. These define the wire format for RPC communication between clients and the server.

## Key Patterns

### Proto Organization
```
include/xrpl/proto/
└── org/xrpl/rpc/v1/
    └── *.proto generated headers
```

### Usage
```cpp
#include <xrpl/proto/org/xrpl/rpc/v1/xrp_ledger.grpc.pb.h>
// Use generated types for gRPC request/response
```

### Versioning
- API is versioned under `org/xrpl/rpc/v1/`
- New versions get a new directory (v2/, etc.)
- Never modify generated files directly - modify the .proto source

## Common Pitfalls
- Never hand-edit generated .pb.h files - regenerate from .proto sources
- Always use the versioned path (v1/) when including proto headers
- Proto types are for serialization only - convert to native types for business logic

## Key Files
- `include/xrpl/proto/org/xrpl/rpc/v1/` - Generated gRPC API headers
