# Net Images Module Best Practices

## Description
Use when working with static image assets in `src/libxrpl/net/images/`. Contains embedded image data used by the HTTP server.

## Responsibility
Stores static image assets (favicons, logos) as embedded byte arrays for serving via the built-in HTTP server without external file dependencies.

## Key Patterns

### Embedded Assets
```cpp
// Images stored as static const byte arrays
// Compiled directly into the binary - no file I/O needed at runtime
static unsigned char const favicon[] = { /* ... */ };
```

## Common Pitfalls
- Keep embedded images small - they increase binary size
- Use appropriate image formats (ICO for favicons, PNG for logos)
- Update both the source image and the byte array when changing assets

## Key Files
- `src/libxrpl/net/images/` - Static image byte arrays embedded in the binary
