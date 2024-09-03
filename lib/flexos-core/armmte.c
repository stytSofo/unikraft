#include "flexos/impl/armmte.h"

uintptr_t set_specific_tag(uintptr_t ptr, uint8_t tag)
{
	// Clear the existing tag and apply the new one
	return (ptr & ~(0xFUL << 56)) | ((uintptr_t)tag << 56);
}

/// @brief Generates a tagged pointer with the specified tag
/// @param ptr The pointer we want to tag
/// @param tag The tag to tag the pointer with
/// @param size How many bytes to tag
void *mte_set_tag(void *ptr, uint8_t tag, size_t size)
{
	uintptr_t base_addr = set_specific_tag((uintptr_t)ptr, tag);
	size_t mte_granule_size = 16;

	// Tag each block in the memory region
	for (size_t offset = 0; offset < size; offset += mte_granule_size) {
		uintptr_t current_addr = base_addr + offset;
		asm volatile("stg %0, [%0]" : : "r"(current_addr) : "memory");
	}

	return (void *)base_addr;
}

void *mte_alloc(size_t size)
{
	void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_MTE,
			 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == MAP_FAILED) {
		perror("mmap failed");
		return NULL;
	}
	return ptr;
}

/// @brief Allocate memory and tag with the supplied tag
/// @param size
/// @param tag The color we want to tag the allocated memory with. See
/// @return
flexos_mte_pointer mte_malloc(size_t size, uint8_t tag)
{
	flexos_mte_pointer mte_ptr;
	mte_ptr.ptr = mte_alloc(size);
	mte_ptr.size = size;

	printf("Tagging %d bytes with %x\n", size, tag);
	if (mte_ptr.ptr) {
		mte_ptr.ptr = mte_set_tag(mte_ptr.ptr, tag, size);
		printf("Tagged ptr %p\n", mte_ptr.ptr);

		return mte_ptr;
	}

	return;
}

void cross_compartment(void *ptr, size_t size, uint8_t current_tag,
		       uint8_t target_tag, function_ptr_t func)
{
	if (current_tag == target_tag) {
		func(ptr);
		return;
	}
	CROSS_COMPARTMENT_CALLS++;
	uk_pr_debug("Calls: %ld", CROSS_COMPARTMENT_CALLS);

	// Retag the pointer with the target compartment's tag
	ptr = mte_set_tag(ptr, target_tag, size);

	// Call the function with the retagged pointer
	func(ptr);

	// Optionally, restore the original tag
	ptr = mte_set_tag(ptr, current_tag, size);
}

void cross_compartment1(void *ptr, size_t size, uint8_t current_tag,
			uint8_t target_tag, function_ptr_t2 func, uint64_t arg1)
{
	if (current_tag == target_tag) {
		func(ptr, arg1);
		return;
	}
	CROSS_COMPARTMENT_CALLS++;
	uk_pr_debug("Calls: %ld", CROSS_COMPARTMENT_CALLS);

	// Retag the pointer with the target compartment's tag
	ptr = mte_set_tag(ptr, target_tag, size);
	arg1 = mte_set_tag(arg1, target_tag, size);

	// Call the function with the retagged pointer
	func(ptr, arg1);

	// Optionally, restore the original tag
	ptr = mte_set_tag(ptr, current_tag, size);
	arg1 = mte_set_tag(arg1, current_tag, size);
}

void cross_compartment2(void *ptr, size_t size, uint8_t current_tag,
			uint8_t target_tag, function_ptr_t3 func, uint64_t arg1,
			uint64_t arg2)
{
	if (current_tag == target_tag) {
		func(ptr, arg1, arg2);
		return;
	}
	// Retag the pointer with the target compartment's tag
	CROSS_COMPARTMENT_CALLS++;
	uk_pr_debug("Calls: %ld", CROSS_COMPARTMENT_CALLS);

	ptr = mte_set_tag(ptr, target_tag, size);
	arg1 = mte_set_tag(arg1, target_tag, size);
	arg2 = mte_set_tag(arg2, target_tag, size);

	// Call the function with the retagged pointer
	func(ptr, arg1, arg2);

	// Optionally, restore the original tag
	ptr = mte_set_tag(ptr, current_tag, size);
	arg1 = mte_set_tag(arg1, current_tag, size);
	arg2 = mte_set_tag(arg2, current_tag, size);
}

void cross_compartment3(void *ptr, size_t size, uint8_t current_tag,
			uint8_t target_tag, function_ptr_t4 func, uint64_t arg1,
			uint64_t arg2, uint64_t arg3)
{
	if (current_tag == target_tag) {
		func(ptr, arg1, arg2, arg3);
		return;
	}
	CROSS_COMPARTMENT_CALLS++;
	uk_pr_debug("Calls: %ld", CROSS_COMPARTMENT_CALLS);
	// Retag the pointer with the target compartment's tag
	ptr = mte_set_tag(ptr, target_tag, size);
	arg1 = mte_set_tag(arg1, target_tag, size);
	arg2 = mte_set_tag(arg2, target_tag, size);
	arg3 = mte_set_tag(arg3, target_tag, size);

	// Call the function with the retagged pointer
	func(ptr, arg1, arg2, arg3);

	// Optionally, restore the original tag
	ptr = mte_set_tag(ptr, current_tag, size);
	arg1 = mte_set_tag(arg1, current_tag, size);
	arg2 = mte_set_tag(arg2, current_tag, size);
	arg3 = mte_set_tag(arg3, current_tag, size);
}
