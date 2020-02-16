// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_aatree.h
/// \brief AA trees code

#include "m_aatree.h"
#include "z_zone.h"

// A partial implementation of AA trees,
// according to the algorithms given on Wikipedia.
// http://en.wikipedia.org/wiki/AA_tree

typedef struct aatree_node_s
{
	INT32	level;
	INT32	key;
	void*	value;

	struct aatree_node_s *left, *right;
} aatree_node_t;

struct aatree_s
{
	aatree_node_t	*root;
	UINT32		flags;
	void** array;// test: replace aa-tree functionality with arrays
	INT32 size;//
};

aatree_t *M_AATreeAlloc(UINT32 flags, INT32 size)
{
	aatree_t *aatree = Z_Malloc(sizeof (aatree_t), PU_STATIC, NULL);
	aatree->root = NULL;
	aatree->flags = flags;
	aatree->size = size;//
	aatree->array = Z_Calloc(sizeof(void*) * size, PU_STATIC, NULL);//
	printf("ALLOCATED ARRAY OF SIZE %d\n", size);//
	return aatree;
}

void M_AATreeFree(aatree_t *aatree)
{
	Z_Free(aatree->array);//
	Z_Free(aatree);
}

void M_AATreeSet(aatree_t *aatree, INT32 key, void* value)
{
	if (value && (aatree->flags & AATREE_ZUSER)) Z_SetUser(value, &aatree->array[key]);//
	else aatree->array[key] = value;//
}

void *M_AATreeGet(aatree_t *aatree, INT32 key)
{
	return aatree->array[key];//
}


void M_AATreeIterate(aatree_t *aatree, aatree_iter_t callback)
{
	INT32 i;
	for (i = 0; i < aatree->size; i++)//
	{//
		if (aatree->array[i]) callback(i, aatree->array[i]);//
	}//
}
