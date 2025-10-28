#include "uros.h"

// Free-list memory allocator with coalescing
// Replaces simple bump allocator for better memory reuse

typedef struct mem_header {
    size_t size;              // Size of this block (excluding header)
    int free;                 // 1 if free, 0 if allocated
    struct mem_header *next;  // Next block in free list
} mem_header_t;

#define HEADER_SIZE sizeof(mem_header_t)
#define MIN_ALLOC_SIZE 16  // Minimum allocation size for splitting

static u8 heap[HEAP_SIZE];
static mem_header_t *free_list = (mem_header_t *)0;
static size_t total_allocated = 0;
static size_t total_free = 0;
static int initialized = 0;

// Initialize memory system
static void kmem_init(void) {
    if (initialized) {
        return;
    }
    
    // Initialize first block (entire heap)
    free_list = (mem_header_t *)heap;
    free_list->size = HEAP_SIZE - HEADER_SIZE;
    free_list->free = 1;
    free_list->next = (mem_header_t *)0;
    
    total_free = HEAP_SIZE - HEADER_SIZE;
    total_allocated = 0;
    initialized = 1;
}

// First-fit allocation with splitting
void *kmalloc(size_t size) {
    if (!initialized) {
        kmem_init();
    }
    
    if (size == 0) {
        return (void *)0;
    }
    
    // Align to 16 bytes
    size = (size + 15) & ~15;
    
    disable_irq();
    
    // Search for first-fit block
    mem_header_t *current = free_list;
    
    while (current) {
        if (current->free && current->size >= size) {
            // Found a suitable block
            
            // Should we split this block?
            size_t remaining = current->size - size;
            if (remaining >= HEADER_SIZE + MIN_ALLOC_SIZE) {
                // Split the block
                mem_header_t *new_block = (mem_header_t *)((u8 *)current + HEADER_SIZE + size);
                new_block->size = remaining - HEADER_SIZE;
                new_block->free = 1;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            // Allocate this block
            current->free = 0;
            total_allocated += current->size;
            total_free -= current->size;
            
            enable_irq();
            return (void *)((u8 *)current + HEADER_SIZE);
        }
        
        current = current->next;
    }
    
    enable_irq();
    return (void *)0;  // Out of memory
}

// Free memory and coalesce with neighbors
void kfree(void *ptr) {
    if (!ptr) {
        return;
    }
    
    disable_irq();
    
    // Get header
    mem_header_t *block = (mem_header_t *)((u8 *)ptr - HEADER_SIZE);
    
    // Mark as free
    if (!block->free) {
        block->free = 1;
        total_allocated -= block->size;
        total_free += block->size;
    }
    
    // Coalesce with next block if it's free
    if (block->next && block->next->free) {
        mem_header_t *next = block->next;
        block->size += HEADER_SIZE + next->size;
        block->next = next->next;
    }
    
    // Coalesce with previous block (need to find it)
    mem_header_t *current = free_list;
    mem_header_t *prev = (mem_header_t *)0;
    
    while (current && current != block) {
        prev = current;
        current = current->next;
    }
    
    if (prev && prev->free) {
        // Coalesce with previous
        prev->size += HEADER_SIZE + block->size;
        prev->next = block->next;
    }
    
    enable_irq();
}

// Get used memory
size_t kmalloc_used(void) {
    return total_allocated;
}

// Get free memory
size_t kmalloc_free(void) {
    return total_free;
}

// Get number of free blocks
int kmalloc_free_blocks(void) {
    if (!initialized) {
        return 0;
    }
    
    int count = 0;
    mem_header_t *current = free_list;
    
    while (current) {
        if (current->free) {
            count++;
        }
        current = current->next;
    }
    
    return count;
}

