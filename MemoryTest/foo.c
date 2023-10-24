#include "foo.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <mach/vm_types.h>
#include <mach/vm_map.h>
#include <mach/mach_init.h>

typedef uintptr_t uword;
uword RoundDown(uword x, uword alignment) {
    return x & -alignment;
}
uword Min(uword a, uword b) {
    return a < b ? a : b;
}
uword Max(uword a, uword b) {
    return a > b ? a : b;
}

long global_counter = 0;
long increment(void) {
  return global_counter++;
}
typedef long (*Increment)(void);


static int testFunction(int x) {
    return 2 * x;
}

void foo(void) {
    size_t page_size = getpagesize();
    printf("page_size: %lx\n", page_size);
    
    uword function = (uword)&increment;
    uword data = (uword)&global_counter;

    uword function_page = RoundDown(function, page_size);
    uword data_page = RoundDown(data, page_size);

    uword lower = Min(function_page, data_page);
    uword higher = Max(function_page, data_page);
    uword size = higher - lower + page_size;

    void* reservation =
        mmap(NULL, size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (reservation == MAP_FAILED) {
      perror("mmap for reservation");
      abort();
    }

    uword function_page2 = function_page - lower + (uword)reservation;
    uword data_page2 = data_page - lower + (uword)reservation;
    printf("size: %lx\n", size);
    printf("data: %lx function: %lx\n", data_page, function_page);
    printf("data: %lx function: %lx\n", data_page2, function_page2);

    {
        const mach_port_t task = mach_task_self();
        const vm_address_t source_address = (vm_address_t)data_page;
        const vm_size_t mem_size = page_size;
        vm_prot_t current_protection = VM_PROT_READ | VM_PROT_WRITE;
        vm_prot_t max_protection = VM_PROT_READ | VM_PROT_WRITE;
        vm_address_t target_address = data_page2;
        kern_return_t status =
            vm_remap(task, &target_address, mem_size,
                     /*mask=*/0,
                     /*flags=*/VM_FLAGS_OVERWRITE, task, source_address,
                     /*copy=*/1, &current_protection, &max_protection,
                     /*inheritance=*/VM_INHERIT_NONE);
        if (status != KERN_SUCCESS) {
            printf("vm_remap %d", status);
            abort();
        }
    }

    {
        const mach_port_t task = mach_task_self();
        const vm_address_t source_address = (vm_address_t)function_page;
        const vm_size_t mem_size = page_size;
        vm_prot_t current_protection = VM_PROT_READ | VM_PROT_EXECUTE;
        vm_prot_t max_protection = VM_PROT_READ | VM_PROT_EXECUTE;
        vm_address_t target_address = function_page2;
        kern_return_t status =
            vm_remap(task, &target_address, mem_size,
                     /*mask=*/0,
                     /*flags=*/VM_FLAGS_OVERWRITE, task, source_address,
                     /*copy=*/1, &current_protection, &max_protection,
                     /*inheritance=*/VM_INHERIT_NONE);
        if (status != KERN_SUCCESS) {
          printf("vm_remap %d", status);
          abort();
        }
    }

    Increment increment2 = (Increment)(function_page2 + function - function_page);

    printf("-- %ld\n", increment());
    printf("-- %ld\n", increment());
    printf("-- %ld\n", increment());
    printf("-- %ld\n", increment());

    printf("-- %ld\n", increment2());
    printf("-- %ld\n", increment2());
    printf("-- %ld\n", increment2());
    printf("-- %ld\n", increment2());
}
