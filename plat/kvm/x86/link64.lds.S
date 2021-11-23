/* SPDX-License-Identifier: ISC */
/*
 * Author(s): Dan Williams <djwillia@us.ibm.com>
 *            Martin Lucina <martin.lucina@docker.com>
 *            Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2016, IBM
 *           (c) 2016-2017 Docker, Inc.
 *           (c) 2017-2018, NEC Europe Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <uk/arch/limits.h> /* for __PAGE_SIZE */
#include <uk/plat/common/common.lds.h>

ENTRY(_libkvmplat_entry)
SECTIONS
{
	. = 0x100000;

	.multiboot_header :
	{
		KEEP (*(.data.boot))
	}

	. = ALIGN(__PAGE_SIZE);
	_sshared = .;
	.data_shared :
	{
		KEEP (*(.data_shared .data_shared.*))
		. = ALIGN(__PAGE_SIZE);
		/* this padding might not be actually needed */
		. = . + 0x1000;
	}
	_eshared = .;

	/* Code */
	_text = .;
	.text :
	{
		/* prevent linker gc from removing multiboot header */
		*(.text.boot)

		*(.text_shared)
		*(.text)
		*(.text.*)
	}

	/* __FLEXOS MARKER__: insert compartment text sections here. */

	_etext = .;

	EXCEPTION_SECTIONS

	CTORTAB_SECTION

	INITTAB_SECTION

	/* Read-only data */
	. = ALIGN(__PAGE_SIZE);
	_rodata = .;
	.rodata :
	{
		*(.rodata)
		*(.rodata.*)
	}
	_erodata = .;

	/* Constructor tables (read-only) */
	. = ALIGN(0x8);
	_ctors = .;
	.preinit_array : {
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);
	}

	. = ALIGN(0x8);
	.init_array : {
		PROVIDE_HIDDEN (__init_array_start = .);
		/* __FLEXOS MARKER__: insert compartment init array sections here. */
		PROVIDE_HIDDEN (__init_array_end = .);
	}
	_ectors = .;

	TLS_SECTIONS

	/* -- compartment data sections begin -- */
	/* __FLEXOS MARKER__: insert compartment data sections here. */
	/* -- compartment data sections end -- */

	/* Read-write data (initialized) */
	. = ALIGN(__PAGE_SIZE);
	_data = .;
	.data :
	{
		*(.data_comp0)
		*(.data_comp0.*)
		*(.data)
		*(.data.*)
	}
	_edata = .;

	/* Read-write data (uninitialized) */
	. = ALIGN(__PAGE_SIZE);
	__bss_start = .;
	.bss :
	{
		*(.bss_comp0)
		*(.bss_comp0.*)
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(__PAGE_SIZE);
	}

	/* -- compartment bss sections begin -- */
	/* __FLEXOS MARKER__: insert compartment bss sections here. */
	/* -- compartment bss sections end -- */

	/* We keep the interrupt stack on a different section
	 * given that it may have a big alignment and it would
	 * change the entire binary layout
	 */
	_intrstack_start = .;
	.intrstack :
	{
		*(.intrstack)
		. = ALIGN(__PAGE_SIZE);
	}

	_end = .;

	.comment       0 : { *(.comment) }
	DEBUG_SYMBOLS

	/DISCARD/ : { *(.note.gnu.build-id) }
}
