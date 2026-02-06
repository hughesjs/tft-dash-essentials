# TFT Dash Refactoring Progress

## Phase 1: Extract and Test Parsing Logic ✅ COMPLETE

### What We've Done

1. **Created `parser.h`** - Clean API for message parsing with three structs:
   - `DashboardState` - Live sensor data from firmware
   - `MenuState` - Configuration/menu data from firmware
   - `NavState` - Navigation data from phone

2. **Created `parser.c`** - Data-driven parsing implementation:
   - **Eliminated if-ladders**: Used lookup tables with field descriptors
   - **Parse once, not 37 times**: Each field parsed exactly once with correct function
   - **O(1) field lookup**: Direct array indexing instead of cascading if statements
   - **Type-safe**: Field type encoded in descriptor (INT, FLOAT, BOOL, STRING)
   - **Maintainable**: Adding/removing fields is just editing the table

3. **Created `test_parser.c`** - Comprehensive test suite:
   - 10 test cases covering all message types
   - Tests for integers, floats, booleans, strings
   - Real-world message validation
   - Null pointer safety checks
   - **All tests passing ✅**

### Performance Improvements

**Before (if-ladder):**
```c
if (messagenumber == 1) { currentSpeed = atoi(szCurrent); }
if (messagenumber == 2) { rpm = atoi(szCurrent); }
// ... 35 more comparisons for EVERY field
```
- 37 comparisons per field
- For 37 fields: ~684 total comparisons

**After (lookup table):**
```c
parse_field(szCurrent, state, &fields[field_num]);
```
- 1 array lookup per field
- For 37 fields: 37 total lookups
- **~18x fewer operations**

### How to Build & Test

```bash
# Compile tests
gcc -o test_parser test_parser.c parser.c -std=c99 -lm

# Run tests
./test_parser
```

## Next Steps

### Phase 2: Modularise Codebase (In Progress)

Break `testsdl.cpp` into logical modules:
- `parser.c/h` ✅ Done
- `serial.c/h` - Serial port handling and framing
- `render.c/h` - SDL2 rendering functions
- `menu.c/h` - Menu system
- `main.c` - Main loop and initialisation

### Phase 3: Replace Global State

Create `DashboardContext` struct to replace ~80 global variables:
```c
typedef struct {
    DashboardState live;
    MenuState menu;
    NavState nav;
    SDL_Renderer* renderer;
    // ... SDL textures, surfaces, etc.
} DashboardContext;
```

Pass context pointer through function calls instead of using globals.

### Phase 4: Documentation

Update CLAUDE.md with new architecture, module descriptions, and testing instructions.

## Benefits Achieved So Far

- ✅ **Testable code**: Parsing logic can be tested without SDL/hardware
- ✅ **No regressions**: Tests validate behaviour before/after refactoring
- ✅ **Better performance**: O(1) lookups vs O(n) if-ladders
- ✅ **More maintainable**: Data-driven approach, clear field mappings
- ✅ **Learning opportunity**: Understanding the protocol deeply
- ✅ **Rust rewrite prep**: Clear data structures and parsing logic to port

## Parsing Implementation Details

### Field Descriptor Pattern

```c
typedef struct {
    FieldType type;    // INT, FLOAT, BOOL, STRING
    size_t offset;     // Byte offset into target struct
    size_t max_len;    // For strings only
} FieldDescriptor;
```

### Example Field Table

```c
static const FieldDescriptor live_fields[] = {
    FIELD_SKIP(),                            /* 0: skip */
    FIELD_INT(DashboardState, currentSpeed), /* 1 */
    FIELD_INT(DashboardState, rpm),          /* 2 */
    // ...
};
```

### Generic Parser

One parser handles all message types:
```c
parse_delimited_message(msg, delimiter, state, fields, num_fields);
```

This means:
- Live messages use `,` delimiter and `live_fields[]`
- Menu messages use `,` delimiter and `menu_fields[]`
- Nav messages use `%` delimiter and `nav_fields[]`

### Type-Safe Field Parsing

```c
void parse_field(const char* value, void* base_ptr, const FieldDescriptor* desc) {
    char* target = (char*)base_ptr + desc->offset;

    switch (desc->type) {
        case TYPE_INT:   *(int*)target = atoi(value); break;
        case TYPE_FLOAT: *(float*)target = atof(value); break;
        case TYPE_BOOL:  *(bool*)target = (atoi(value) == 1); break;
        case TYPE_STRING: strncpy(target, value, desc->max_len); break;
    }
}
```

Each field is parsed exactly once with the correct conversion function.
