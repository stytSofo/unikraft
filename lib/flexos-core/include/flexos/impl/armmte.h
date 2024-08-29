#ifndef FLEXOS_ARMMTE_H
#define FLEXOS_ARMMTE_H

#include "mman.h"
#include "prctl.h"
#include <unistd.h>
#include <stdlib.h>

typedef struct {
    size_t size;     // Size of the memory block
    uint64_t *ptr;   // Pointer to the memory block, should be tagged if using MTE
} flexos_mte_pointer;


typedef void (*function_ptr_t)(void *);
typedef void (*function_ptr_t2)(void *, uint64_t);
typedef void (*function_ptr_t3)(void *, uint64_t, uint64_t);
typedef void (*function_ptr_t4)(void *, uint64_t, uint64_t, uint64_t);
typedef void (*function_ptr_t5)(void *, uint64_t, uint64_t, uint64_t, uint64_t);
// Taken from the implementation of vmept.h
#define CHOOSE_GATE(dummy, g6, g5, g4, g3, g2, g1, g0, ...) g0

// Do this since functions can accept more than one parameters
#define flexos_mte_gate(ptr, size, tag_from, tag_to, fname, ...)               \
	flexos_mte_gate6(ptr, size, tag_from, tag_to, &(fname), __VA_ARGS__),  \
	    flexos_mte_gate5(ptr, size, tag_from, tag_to, &(fname),            \
			     __VA_ARGS__),                                     \
	    flexos_mte_gate4(ptr, size, tag_from, tag_to, &(fname),            \
			     __VA_ARGS__),                                     \
	    flexos_mte_gate3(ptr, size, tag_from, tag_to, &(fname),            \
			     __VA_ARGS__),                                     \
	    flexos_mte_gate2(ptr, size, tag_from, tag_to, &(fname),            \
			     __VA_ARGS__),                                     \
	    flexos_mte_gate1(ptr, size, tag_from, tag_to, &(fname),            \
			     __VA_ARGS__),                                     \
	    flexos_mte_gate0(ptr, size, tag_from, tag_to, &(fname))

#define flexos_mte_gate0(ptr, size, tag_from, tag_to, fname)                   \
	do {                                                                   \
		cross_compartment((void *)ptr, size, tag_from, tag_to, fname); \
	}

#define flexos_mte_gate1(ptr, size, tag_from, tag_to, fname, arg1)             \
	do {                                                                   \
		cross_compartment1((void *)ptr, (size_t)size,                  \
				   (uint8_t)tag_from, (uint8_t)tag_to,         \
				   (function_ptr_t2)fname, (uint64_t)arg1);    \
	}

#define flexos_mte_gate2(ptr, size, tag_from, tag_to, fname, arg1, arg2)       \
	do {                                                                   \
		cross_compartment2((void *)ptr, (size_t)size,                  \
				   (uint8_t)tag_from, (uint8_t)tag_to,         \
				   (function_ptr_t3)fname, (uint64_t)arg1,     \
				   (uint64_t)arg2);                            \
	}

#define flexos_mte_gate3(ptr, size, tag_from, tag_to, fname, arg1, arg2, arg3) \
	do {                                                                   \
		cross_compartment3((void *)ptr, (size_t)size,                  \
				   (uint8_t)tag_from, (uint8_t)tag_to,         \
				   (function_ptr_t4)fname, (uint64_t)arg1,     \
				   (uint64_t)arg2, (uint64_t), arg3);          \
	}

#define flexos_mte_gate4(ptr, size, tag_from, tag_to, fname, arg1, arg2, arg3, \
			 arg4)                                                 \
	do {                                                                   \
		cross_compartment4(                                            \
		    (void *)ptr, (size_t)size, (uint8_t)tag_from,              \
		    (uint8_t)tag_to, (function_ptr_t5)fname, (uint64_t)arg1,   \
		    (uint64_t)arg2, (uint64_t)arg3, (uint64_t)arg4);           \
	}

enum MTE_TAGS {
	NO = 0x0,
	BLACK,
	WHITE,
	RED,
	GREEN,
	BLUE,
	YELLOW,
	CYAN,
	MAGENTA,
	PINK,
	BROWN,
	ORANGE,
	PURPLE,
	GOLD,
	GREY
};

uintptr_t set_specific_tag(uintptr_t ptr, uint8_t tag);
/// @brief Generates a tagged pointer with the specified tag
/// @param ptr The pointer we want to tag
/// @param tag The tag to tag the pointer with
/// @param size How many bytes to tag
void *mte_set_tag(void *ptr, uint8_t tag, size_t size);

flexos_mte_pointer mte_malloc(size_t size, uint8_t tag);

void *mte_alloc(size_t size);

/// @brief A helper function to re-tag addresses when crossing compartments
/// @param ptr The tagged data pointer
/// @param size The allocated memory size
/// @param current_tag The current tag of ptr
/// @param target_tag The tag of the callee
/// @param func The function pointer
void cross_compartment(void *ptr, size_t size, uint8_t current_tag,
		       uint8_t target_tag, function_ptr_t func);

/// @brief A helper function to re-tag addresses when crossing compartments and
/// the target function accepts 1 argument
/// @param ptr The tagged data pointer
/// @param size The allocated memory size
/// @param current_tag The current tag of ptr
/// @param target_tag The tag of the callee
/// @param func The function pointer
/// @param arg1 The argument of the func function
void cross_compartment1(void *ptr, size_t size, uint8_t current_tag,
			uint8_t target_tag, function_ptr_t2 func,
			uint64_t arg1);

/// @brief A helper function to re-tag addresses when crossing compartments and
/// the target function accepts 2 argument
/// @param ptr The tagged data pointer
/// @param size The allocated memory size
/// @param current_tag The current tag of ptr
/// @param target_tag The tag of the callee
/// @param func The function pointer
/// @param arg1 The argument of the func function
/// @param arg2 The argument of the func function
void cross_compartment2(void *ptr, size_t size, uint8_t current_tag,
			uint8_t target_tag, function_ptr_t3 func, uint64_t arg1,
			uint64_t arg2);

/// @brief A helper function to re-tag addresses when crossing compartments and
/// the target function accepts 3 argument
/// @param ptr The tagged data pointer
/// @param size The allocated memory size
/// @param current_tag The current tag of ptr
/// @param target_tag The tag of the callee
/// @param func The function pointer
/// @param arg1 The argument of the func function
/// @param arg2 The argument of the func function
/// @param arg3 The argument of the func function
void cross_compartment3(void *ptr, size_t size, uint8_t current_tag,
			uint8_t target_tag, function_ptr_t4 func, uint64_t arg1,
			uint64_t arg2, uint64_t arg3);

/// @brief A helper function to re-tag addresses when crossing compartments and
/// the target function accepts 4 argument
/// @param ptr The tagged data pointer
/// @param size The allocated memory size
/// @param current_tag The current tag of ptr
/// @param target_tag The tag of the callee
/// @param func The function pointer
/// @param arg1 The argument of the func function
/// @param arg2 The argument of the func function
/// @param arg3 The argument of the func function
/// @param arg4 The argument of the func function
void cross_compartment4(void *ptr, size_t size, uint8_t current_tag,
			uint8_t target_tag, function_ptr_t5 func, uint64_t arg1,
			uint64_t arg2, uint64_t arg3, uint64_t arg4);

#endif