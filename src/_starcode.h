#ifndef _STARCODE_PRIVATE_HEADER
#define _STARCODE_PRIVATE_HEADER
int        size_order (const void *a, const void *b);
void       addmatch (useq_t*, useq_t*, int, int);
int        bisection (int, int, char *, useq_t **, int, int);
int        canonical_order (const void*, const void*);
long int   count_trie_nodes (useq_t **, int, int);
int        count_order (const void *, const void *);
void       destroy_useq (useq_t*);
void     * do_query (void*);
void       lut_insert (lookup_t *, useq_t *); 
int        lut_search (lookup_t *, useq_t *); 
void       message_passing_clustering (gstack_t*, int);
lookup_t * new_lookup (int, int, int, int);
useq_t   * new_useq (int, char *);
int        pad_useq (gstack_t*, int*);
mtplan_t * plan_mt (int, int, int, int, gstack_t *, const int);
void       run_plan (mtplan_t *, int, int);
gstack_t * read_file (FILE *);
int        seq2id (char *, int);
gstack_t * seq2useq (gstack_t*, int);
int        seqsort (void **, int, int (*)(const void*, const void*), int);
void       sphere_clustering (gstack_t *, int);
void       transfer_counts_and_update_canonicals (useq_t*);
void       unpad_useq (gstack_t*);
void     * _mergesort (void *); 
#endif
