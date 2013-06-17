#ifndef _starcode_trie_loaded
#define _starcode_trie_loaded

#define MAXBRCDLEN 1024

struct _node
{
   // Trie nodes contain a single DNA letter, A, C, G, T or anything
   // else. 'counter' is a positive 'int' for leaf nodes and 0 otherwise.
   // Trie nodes have a pointer to their parent and 5 pointers to
   // their children. They have a maximum of 5 children because all
   // non DNA letters are considered identical.
   char c;
   int counter;
   struct _node *parent;
   struct _node *child[5];
};
typedef struct _node trienode;

typedef struct
{
   // Structure to keep track of hits in linked lists.
   int size;
   int n_hits;
   trienode **node;
} hitlist;
#endif

hitlist   * new_hitlist (void);
void        clear_hitlist (hitlist*);
void        destroy_hitlist (hitlist*);
trienode  * new_trie (void);
trienode  * find_path (trienode *, const char **);
trienode  * insert (trienode *, const char *, int);
void        search (trienode *, char *, int, hitlist *);
void        destroy_trie (trienode*);
char      * seq (const trienode *, char *, int);
int         add_to_count(trienode *, int);
