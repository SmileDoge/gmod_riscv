/*
fdtlib.h - Flattened Device Tree Library
Copyright (C) 2021  cerg2010cerg2010 <github.com/cerg2010cerg2010>
                    LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef RVVM_FDTLIB_H
#define RVVM_FDTLIB_H

#include "rvvmlib.h"

struct fdt_node;

/*
 * Node handling
 */

// Create a fdt node, root node should have name = NULL
PUBLIC struct fdt_node* fdt_node_create(const char* name);

// Create a fdt node with address, like device@10000
PUBLIC struct fdt_node* fdt_node_create_reg(const char* name, uint64_t addr);

// Attach child node
PUBLIC void fdt_node_add_child(struct fdt_node* node, struct fdt_node* child);

// Lookup for child node by name (returns NULL on failure)
PUBLIC struct fdt_node* fdt_node_find(struct fdt_node* node, const char* name);

// Lookup for child node by name + addr, like device@10000 (returns NULL on failure)
PUBLIC struct fdt_node* fdt_node_find_reg(struct fdt_node* node, const char* name, uint64_t addr);

// Lookup for any reg child node by name, like device@* (returns NULL on failure)
PUBLIC struct fdt_node* fdt_node_find_reg_any(struct fdt_node* node, const char *name);

// Get child node phandle. Allocates phandles transparently, all node hierarchy should be attached!
PUBLIC uint32_t fdt_node_get_phandle(struct fdt_node* node);

/*
 * Property handling
 */

// Add arbitary byte buffer property
PUBLIC void fdt_node_add_prop(struct fdt_node* node, const char* name, const void* data, uint32_t len);

// Add single-cell property
PUBLIC void fdt_node_add_prop_u32(struct fdt_node* node, const char* name, uint32_t val);

// Add double-cell property
PUBLIC void fdt_node_add_prop_u64(struct fdt_node* node, const char* name, uint64_t val);

// Add multi-cell property
PUBLIC void fdt_node_add_prop_cells(struct fdt_node* node, const char* name, uint32_t* cells, uint32_t count);

// Add string property
PUBLIC void fdt_node_add_prop_str(struct fdt_node* node, const char* name, const char* val);

// Add register range property (addr cells: 2, size cells: 2)
PUBLIC void fdt_node_add_prop_reg(struct fdt_node* node, const char* name, uint64_t begin, uint64_t size);

// Returns underlying property data
PUBLIC void* fdt_node_get_prop_data(struct fdt_node* node, const char* name);

// Returns underlying property size
PUBLIC size_t fdt_node_get_prop_size(struct fdt_node* node, const char* name);

// Delete property
PUBLIC bool fdt_node_del_prop(struct fdt_node* node, const char* name);

/*
 * Serialization, cleanup
 */

// Recursively free a node and it's child nodes
PUBLIC void fdt_node_free(struct fdt_node* node);

// Returns required buffer size for serializing
PUBLIC size_t fdt_size(struct fdt_node* node);

// Serialize DTB into buffer, returns 0 when there's insufficient space
// Returns required buffer size when buffer == NULL
PUBLIC size_t fdt_serialize(struct fdt_node* node, void* buffer, size_t size, uint32_t boot_cpuid);

#endif
