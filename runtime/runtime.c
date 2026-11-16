#include <inttypes.h> // for int64_t typedefs
#include <stdbool.h>  // C-booleans are a typedef to int
#include <stdio.h>    // for debug printing
#include <stdlib.h>   // For malloc
#include <string.h>   // For memcpy

// –– I/O Operations –––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

void print_int64(int64_t x) {
    printf("%" PRId64 "\n", x);
}

int64_t input_int64() {
    int64_t x;
    int c;

    do {
        if (scanf("%" SCNd64, &x) == 1) 
            break; 

        while ((c = getchar()) != '\n' && c != EOF);
    } while (1);

    return x;
}

// –– Exception Handling –––––––––––––––––––––––––––––––––––––––––––––––––––––––

int64_t* exc_sp = 0;      // Stack pointer
int64_t* exc_fp = 0;      // Frame pointer
int64_t* exc_handler = 0; // Address of the exception handler's basic block

// –– Garbage Collector: API –––––––––––––––––––––––––––––––––––––––––––––––––––

// first free word on from-space
int64_t* gc_free_ptr;       

// first word after from-space
int64_t* gc_fromspace_end;  

void gc_init(
  // stack pointer at begin of main
  int64_t* stack_begin,   
  // initial size of from-/to-space in 64bit-words
  uint64_t heap_size      
);

void gc_collect(
  // current stack pointer
  int64_t* stack_end,       
  // to reallocate from-/to-space, if collection didn't free enough space
  uint64_t requested_words
);

// DEBUG PRINTING
//
// Compiling with the DEBUG preprocessor symbol (passing -D DEBUG to
// gcc) makes the garbage collector print debug information while it
// is running.

// –– Garbage Collector: Implementation ––––––––––––––––––––––––––––––––––––––––

#ifdef DEBUG
  #define printf_if_debug(...) printf(__VA_ARGS__)
#else
  #define printf_if_debug(...)
#endif

void* malloc_safe(uint64_t num_bytes) {
  void* p = malloc(num_bytes);
  if (p == NULL) {
    printf("PANIC: Garbage collector ran out of memory.\n");
    exit(1);
  }
  return p;
}

int64_t* gc_fromspace_begin;

int64_t* gc_tospace_begin;
int64_t* gc_tospace_end;

int64_t* gc_stack_begin;

void gc_init(
  int64_t* stack_begin,   
  uint64_t heap_size      
) {
  printf_if_debug("\n–––––––––– gc_init ––––––––––\n");
  printf_if_debug("Initial space size in words: %" PRIu64 "\n", heap_size);

  gc_stack_begin = stack_begin;

  gc_fromspace_begin = malloc_safe(heap_size * 8);
  gc_fromspace_end = gc_fromspace_begin + heap_size;

  gc_tospace_begin = malloc_safe(heap_size * 8);
  gc_tospace_end = gc_tospace_begin + heap_size;

  gc_free_ptr = gc_fromspace_begin;
  printf_if_debug("––––––––––––––––––––––––––––––\n");
} 

bool gc_contains_heap_ptr(int64_t* p) {
  return *p & 1;
}

bool gc_is_forwarded(int64_t* p) {
  return *p & 1;
}

int64_t* gc_remove_ptr_tag(int64_t* p) {
  return (int64_t*) (((int8_t*) p) - 1);
}

int64_t* gc_add_ptr_tag(int64_t* p) {
  return (int64_t*) (((int8_t*) p) + 1);
}

// Copies an object to wherever free_ptr points to and increases free_ptr.
// Updates the pointer to the object to the new location.
void gc_copy_if_heap_ptr(int64_t* p) {
  /* printf_if_debug("  PTR: %" PRIu64 "\n", (uint64_t) p); */
  // Check if p points to a heap-pointer
  if (gc_contains_heap_ptr(p)) {
    // p is actually a pointer to a tagged heap pointer, e.g. a
    // pointer to a stack location that contains a tagged heap pointer.
    int64_t** object_ptr_ptr = (int64_t**) p;

    // To retrieve the actual heap pointer, we need to get the tagged
    // heap pointer and remove the tag.
    int64_t* object_ptr = gc_remove_ptr_tag(*object_ptr_ptr);
    /* printf_if_debug("  OBJ PTR: %" PRIu64 "\n", (uint64_t) *object_ptr_ptr); */
    /* printf_if_debug("  OBJ PTR: %" PRIu64 "\n", (uint64_t) object_ptr); */

    // Check if the meta data has the forwarding bit set
    if (gc_is_forwarded(object_ptr)) {
      printf_if_debug("  heap ptr: object already copied\n");
      // In this case, the meta data is the forwarding pointer with the
      // tag bit set to 1. We do not need to set the tag bit to 0,
      // because we need to store a tagged pointer anyways (the tag now
      // just means "heap ptr" instead of "forwarded").
      *object_ptr_ptr = (int64_t*) *object_ptr;
    } else {
      // In this case, the meta data contains the number of words after
      // the meta data.  We rightshift to remove the 0 tag bit, and add
      // 1 to account for the meta data itself.
      uint64_t word_len = 1 + (((uint64_t) *object_ptr) >> 1);

      printf_if_debug("  heap ptr: copying object of %" PRIu64 " words...\n", word_len);
      /* printf_if_debug("    %d\n", (object_ptr < gc_fromspace_end && object_ptr >= gc_fromspace_begin)); */
      /* printf_if_debug("    %d\n", object_ptr); */

      // Copy all the bytes of the object
      int64_t* new_object_ptr = gc_free_ptr;
      uint64_t num_bytes = word_len * 8;
      memcpy(new_object_ptr, object_ptr, num_bytes);

      // Add forwarding pointer to old object.
      *object_ptr = (uint64_t) gc_add_ptr_tag(new_object_ptr);

      // Move the free_ptr up by how many bytes we used.
      gc_free_ptr += word_len;

      // Adjust the pointer, which previously pointed to the
      // from-space object, to point to the new to-space object.
      // Note that we need to add a tag to new_object_ptr,
      // because it is also a heap pointer.
      *object_ptr_ptr = gc_add_ptr_tag(new_object_ptr);
    }
  } else {
    printf_if_debug("  no heap ptr of value: %" PRIu64 "\n", *p >> 1);
      /* printf_if_debug("    %" PRIu64 "\n", (uint64_t) (gc_stack_begin - p)); */
  }
}

void gc_copy_and_swap_spaces(
  int64_t* stack_end
) {
  printf_if_debug("from begin: %d\n", gc_fromspace_begin);
  printf_if_debug("from end:   %d\n", gc_fromspace_end);
  printf_if_debug("free_ptr:   %d\n", gc_free_ptr);

  gc_free_ptr = gc_tospace_begin;

  printf_if_debug("Scanning stack...\n");
  // Stack grows upwards!
  for (int64_t* p = gc_stack_begin - 1; p >= stack_end; --p) {
    gc_copy_if_heap_ptr(p);
  }

  printf_if_debug("Scanning to-space...\n");
  // Note that gc_free_ptr in the for-loop condition might
  // increase while the for-loop is running due to the calls to
  // gc_copy_object. This is by design!
  for (int64_t* p = gc_tospace_begin; p < gc_free_ptr; ++p) {
    gc_copy_if_heap_ptr(p);
  }

  // Swap from-space and to-space
  int64_t* tmp_begin = gc_fromspace_begin;
  int64_t* tmp_end = gc_fromspace_end;
  gc_fromspace_begin = gc_tospace_begin;
  gc_fromspace_end = gc_tospace_end;
  gc_tospace_begin = tmp_begin;
  gc_tospace_end = tmp_end;
}

void gc_collect(
  int64_t* stack_end,
  uint64_t requested_words
) {
  printf_if_debug("\n–––––––––– gc_collect ––––––––––\n");

  // Copy from-space to to-space and swap
  gc_copy_and_swap_spaces(stack_end);

  // If garbage collection did not free enough memory, reallocate
  // from- and to-space.
  if (gc_free_ptr + requested_words >= gc_fromspace_end) {
    // Compute new space size
    uint64_t cur_space_words = gc_fromspace_end - gc_fromspace_begin;
    uint64_t new_space_words = cur_space_words;
    while (new_space_words < cur_space_words + requested_words) {
      new_space_words *= 2;
    }
    printf_if_debug("Reallocating spaces:\n");
    printf_if_debug("  requested words:      %" PRId64 "\n", requested_words);
    printf_if_debug("  current space words:  %" PRId64 "\n", cur_space_words);
    printf_if_debug("  new space words:      %" PRId64 "\n", new_space_words);

    // Reallocate to-space
    free(gc_tospace_begin);
    gc_tospace_begin = malloc(new_space_words * 8);
    gc_tospace_end = gc_tospace_begin + new_space_words;

    // Copy from-space to to-space and swap
    gc_copy_and_swap_spaces(stack_end);

    // Reallocate to-space
    free(gc_tospace_begin);
    gc_tospace_begin = malloc(new_space_words * 8);
    gc_tospace_end = gc_tospace_begin + new_space_words;
  }
  printf_if_debug("––––––––––––––––––––––––––––––––\n");
} 

// –– Dict –––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
//
// Dict wrapper layout (GC tuple, 3 words):
//   [metadata: 3<<1] [count<<1] [capacity<<1] [backing_ptr|1]
//
// Backing array layout (GC tuple, 3*capacity words):
//   [metadata: (3*capacity)<<1] [(flag, key, val) * capacity]
//   flag: 0=empty, 2=occupied, 4=deleted  (LSB=0 so GC ignores the flag word)
//
// Deleted slots keep probe chains intact: lookups skip them, inserts reuse
// them, and a resize drops them.
//
// Wrapper and backing live on the GC heap. Functions that allocate take the
// current stack pointer and collect first if necessary, so they never run out
// of space halfway through. They must not use their heap pointer arguments
// after a collection, as the objects may have moved; the compiled code reloads
// its (spilled) variables, which the GC has updated.

// Collect if fewer than `words` words are free on the from-space.
static void gc_ensure(int64_t* stack_end, uint64_t words) {
    if (gc_free_ptr + words >= gc_fromspace_end) gc_collect(stack_end, words);
}

// Allocate `words` words that were reserved with gc_ensure beforehand.
static int64_t* gc_bump(uint64_t words) {
    if (gc_free_ptr + words > gc_fromspace_end) {
        printf("PANIC: dict allocation without reserved heap space\n");
        exit(1);
    }
    int64_t* p = gc_free_ptr;
    gc_free_ptr += words;
    return p;
}

// Words that dict_set may allocate when inserting a new key into the dict.
static uint64_t dict_set_words(int64_t* wrapper) {
    uint64_t count    = (uint64_t)wrapper[1] >> 1;
    uint64_t capacity = (uint64_t)wrapper[2] >> 1;
    if (count * 10 >= capacity * 7) return 1 + 3 * 2 * capacity;
    return 0;
}

static uint64_t struct_hash(int64_t tagged);
static int struct_eq(int64_t a, int64_t b);

static uint64_t struct_hash(int64_t tagged) {
    if (!(tagged & 1)) {
        // Integer or bool: hash the untagged value
        return (uint64_t)(tagged >> 1) * 2654435761ULL;
    }
    // Heap pointer: hash by contents recursively
    int64_t* obj = gc_remove_ptr_tag((int64_t*) tagged);
    uint64_t len = (uint64_t)((uint64_t)obj[0] >> 1);
    uint64_t h = len * 2654435761ULL;
    for (uint64_t i = 0; i < len; i++) {
        h = h * 31 + struct_hash(obj[i + 1]);
    }
    return h;
}

static int struct_eq(int64_t a, int64_t b) {
    if ((a & 1) != (b & 1)) return 0;
    if (!(a & 1)) return a == b;
    // Both heap pointers: compare contents recursively
    int64_t* obj_a = gc_remove_ptr_tag((int64_t*) a);
    int64_t* obj_b = gc_remove_ptr_tag((int64_t*) b);
    uint64_t len_a = (uint64_t)((uint64_t)obj_a[0] >> 1);
    uint64_t len_b = (uint64_t)((uint64_t)obj_b[0] >> 1);
    if (len_a != len_b) return 0;
    for (uint64_t i = 0; i < len_a; i++) {
        if (!struct_eq(obj_a[i + 1], obj_b[i + 1])) return 0;
    }
    return 1;
}

// Insert key→val into backing, assuming key is not already present and there is space.
// Empty and deleted slots can both be used.
static void dict_insert_new(int64_t* backing, int64_t capacity, int64_t key, int64_t val) {
    uint64_t h = struct_hash(key);
    for (int64_t i = 0; i < capacity; i++) {
        int64_t slot = (int64_t)((h + (uint64_t)i) % (uint64_t)capacity);
        int64_t* entry = backing + 1 + 3 * slot;
        if (entry[0] != 2) {
            entry[0] = 2;
            entry[1] = key;
            entry[2] = val;
            return;
        }
    }
    printf("PANIC: dict_insert_new found no empty slot\n");
    exit(1);
}

int64_t dict_create(int64_t* stack_end, uint64_t capacity) {
    uint64_t backing_words = 1 + 3 * capacity;
    gc_ensure(stack_end, (1 + 3) + backing_words);

    int64_t* wrapper = gc_bump(1 + 3);
    wrapper[0] = 3 << 1;
    wrapper[1] = 0;                           // count = 0 (tagged)
    wrapper[2] = (int64_t)(capacity << 1);    // capacity (tagged)

    int64_t* backing = gc_bump(backing_words);
    memset(backing, 0, backing_words * 8);
    backing[0] = (int64_t)((3 * capacity) << 1);

    wrapper[3] = (int64_t) gc_add_ptr_tag(backing);
    return (int64_t) gc_add_ptr_tag(wrapper);
}

// Must be called before dict_set, so that a resize inside dict_set finds enough
// free heap space.
void dict_reserve(int64_t* stack_end, int64_t tagged_dict) {
    int64_t* wrapper = gc_remove_ptr_tag((int64_t*) tagged_dict);
    gc_ensure(stack_end, dict_set_words(wrapper));
}

// Returns the slot holding key, or NULL if the key is absent.
static int64_t* dict_find(int64_t tagged_dict, int64_t tagged_key) {
    int64_t* wrapper  = gc_remove_ptr_tag((int64_t*) tagged_dict);
    int64_t  capacity = (int64_t)((uint64_t)wrapper[2] >> 1);
    int64_t* backing  = gc_remove_ptr_tag((int64_t*) wrapper[3]);

    uint64_t h = struct_hash(tagged_key);
    for (int64_t i = 0; i < capacity; i++) {
        int64_t slot  = (int64_t)((h + (uint64_t)i) % (uint64_t)capacity);
        int64_t* entry = backing + 1 + 3 * slot;
        if (entry[0] == 0) break;  // empty slot: key absent
        if (entry[0] == 2 && struct_eq(entry[1], tagged_key)) return entry;
    }
    return NULL;
}

// Returns a tagged bool.
int64_t dict_has(int64_t tagged_dict, int64_t tagged_key) {
    return dict_find(tagged_dict, tagged_key) ? 1 << 1 : 0;
}

// The compiler checks with dict_has that the key exists. Returns None.
int64_t dict_delete(int64_t tagged_dict, int64_t tagged_key) {
    int64_t* entry = dict_find(tagged_dict, tagged_key);
    if (entry) {
        int64_t* wrapper = gc_remove_ptr_tag((int64_t*) tagged_dict);
        entry[0] = 4;
        // clear key and value, so the GC doesn't keep them alive
        entry[1] = 0;
        entry[2] = 0;
        wrapper[1] = (int64_t)(((uint64_t)wrapper[1] >> 1) - 1) << 1;
    }
    return 0;
}

// The compiler checks with dict_has that the key exists.
int64_t dict_get(int64_t tagged_dict, int64_t tagged_key) {
    int64_t* entry = dict_find(tagged_dict, tagged_key);
    if (!entry) {
        printf("PANIC: dict_get on missing key\n");
        exit(1);
    }
    return entry[2];
}

int64_t dict_set(int64_t tagged_dict, int64_t tagged_key, int64_t tagged_val) {
    int64_t* wrapper  = gc_remove_ptr_tag((int64_t*) tagged_dict);
    int64_t  count    = (int64_t)((uint64_t)wrapper[1] >> 1);
    int64_t  capacity = (int64_t)((uint64_t)wrapper[2] >> 1);
    int64_t* backing  = gc_remove_ptr_tag((int64_t*) wrapper[3]);

    // Update existing key if present
    int64_t* entry = dict_find(tagged_dict, tagged_key);
    if (entry) {
        entry[2] = tagged_val;
        return 0;
    }

    // New key: resize if load > 0.7
    if ((uint64_t)count * 10 >= (uint64_t)capacity * 7) {
        int64_t new_cap = capacity * 2;
        uint64_t new_words = (uint64_t)(1 + 3 * new_cap);
        int64_t* new_backing = gc_bump(new_words);
        memset(new_backing, 0, new_words * 8);
        new_backing[0] = (3 * new_cap) << 1;
        for (int64_t i = 0; i < capacity; i++) {
            int64_t* entry = backing + 1 + 3 * i;
            if (entry[0] == 2) dict_insert_new(new_backing, new_cap, entry[1], entry[2]);
        }
        wrapper[2] = new_cap << 1;
        wrapper[3] = (int64_t) gc_add_ptr_tag(new_backing);
        backing    = new_backing;
        capacity   = new_cap;
    }

    dict_insert_new(backing, capacity, tagged_key, tagged_val);
    wrapper[1] = (count + 1) << 1;
    return 0;
}

// Tests ///////////////////////////////////////////////////////////////////////

// int64_t* fake_stack_begin;
// int64_t* fake_stack_cur;
// int64_t* fake_stack_end;
// 
// uint64_t next_element = 10000;
// 
// int64_t* alloc_tuple(uint64_t num_elements) {
//   printf_if_debug("\nAllocating tuple with %" PRIu64 " elements...\n", num_elements);
//   uint64_t num_words = num_elements + 1;
//   if (gc_free_ptr + num_words >= gc_fromspace_end) {
//     gc_collect(fake_stack_cur, num_words);
//   }
//   int64_t* t = gc_free_ptr;
//   gc_free_ptr += num_words;
//   t[0] = num_elements << 1;
//   for (uint64_t i = 0; i < num_elements; ++i) {
//     t[i+1] = (next_element + i) << 1;
//   }
//   next_element += 100;
//   return gc_add_ptr_tag(t);
// }
// 
// int main() {
//   uint64_t stack_size = 1024;
//   uint64_t heap_size = 8;
// 
//   int64_t* begin = malloc(stack_size * 8);
//   int64_t* end = begin + stack_size;
//   fake_stack_end = begin;
//   fake_stack_begin = end;
//   fake_stack_cur = fake_stack_begin;
// 
//   gc_init(fake_stack_begin, heap_size);
// 
//   for (uint64_t i = 0; i < 5; ++i) {
//     *(fake_stack_cur--) = i << 1; 
//   }
// 
//   for (uint64_t i = 0; i < 5; ++i) {
//     *(fake_stack_cur--) = (int64_t) alloc_tuple(2);
//   }
// }
