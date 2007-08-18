//  main.c
//  SkatTournament
//
//  Created by Simon Stiefel on 15.06.07.
//  Copyright 2006 stiefels.net. All rights reserved.
//
//  $Id$
//

#include <stdio.h>
#include <stdlib.h>

#define N_PLAYERS  8
#define N_TABLES   4
#define N_PPT      2

typedef char* comb;

typedef struct combset {
    comb comb;
    struct combset *next;
    struct combset *prev;
} combset;

typedef struct combset_tree {
    struct combset *round;
    struct combset_tree *parent;
    struct combset_tree *last;
    struct combset_tree *next_neighbour;
    struct combset_tree *prev_neighbour;
    struct combset_tree *childs;
} combset_tree;

// comb functions
void init_comb(comb*);
void destroy_comb(comb);
void copy_comb(comb, comb*);
int compare_comb(comb, comb);
int player_in_comb(comb, int);
void print_comb(comb);

// combset functions
void init_combset(combset **);
void build_all_comb(combset *);
void copy_combset(combset *, combset *);
void append_comb(combset *, comb);
combset* find_comb(combset *, comb);
comb comb_at_position(combset *, int);
void delete_comb(combset **, comb);
void delete_first_comb(combset **);
void delete_comb_with_players(combset **, comb, int); // removes all players in comb from combset
void delete_comb_with_players_of_combset(combset **, combset *, int);
int combset_count(combset *);
void destroy_combset(combset *);
void print_combset(combset *);

// combset_tree functions
void init_combset_tree(combset_tree **);
void append_combset_tree_child(combset_tree *, combset_tree *);
void destroy_combset_tree_childs(combset_tree *);
void destroy_combset_tree_element(combset_tree *);

// position functions
void init_positions(int**);
void destroy_positions(int*);

// actual 
int find_next_round_combset(combset *round, combset *worker, int *positions, int depth);
int find_all_round_combsets(combset *worker, combset_tree *parent);
int find_all_subcombsets(combset_tree *parent, combset *allcombs);

int autostop = 0;

int main (int argc, const char * argv[]) {    
    int selected_startpos;
    
    combset *allcombs = NULL; // all combinations
    combset_tree *resulttree_root; // result tree
    
    // start position
    if (argc < 2) {
        printf("Usage: %s <startpos> <autostop>\n", argv[0]);
        return 1;
    } else {
        selected_startpos = atoi(argv[1]);
    }
    
    if (argc == 3) {
        autostop = atoi(argv[2]);
    }
    
    // init
    init_combset(&allcombs);
    init_combset_tree(&resulttree_root);
    
    // Build all possible combinations
    build_all_comb(allcombs);
    
    // fill first level
    int num_first_level_combsets = find_all_round_combsets(allcombs, resulttree_root);
    
    // check if selected startposition is in range
    if (selected_startpos >= num_first_level_combsets) {
        printf("selected_startpos is out of range...\n");
        return 1;
    }
    
    // delete non-selected combs
    combset_tree *runner = resulttree_root->childs;
    int counter = 0;
    
    while (runner != NULL) {
        if (selected_startpos != counter) {
            destroy_combset_tree_element(runner);
        }
        counter++;
        runner = runner->next_neighbour;
    }
    
    // print selected startposition
    printf("selected start combination:\n");
    print_combset(resulttree_root->childs->round);
    
    find_all_subcombsets(resulttree_root, allcombs);
    
    return 0;
}

int find_all_subcombsets(combset_tree *parent, combset *allcombs)
{
    static int depth = 0;
    static unsigned int num_results = 0;
    depth++;
    
    combset_tree *runner = parent->childs;
    int combcount = combset_count(allcombs);
    int count = 0;
    
    if (runner == NULL || combcount == 0) {
        depth--;
        return 0;
    }
    
    while (runner != NULL && combcount > 0) {
        // copy combs
        combset *reducedcombs = NULL;
        init_combset(&reducedcombs);
        copy_combset(allcombs, reducedcombs);
        
        // delete player occurence
        delete_comb_with_players_of_combset(&reducedcombs, runner->round, 1);
        
        // re-initialize if empty
        if (reducedcombs == NULL) { init_combset(&reducedcombs); }
        
        // find all combsets 
        find_all_round_combsets(reducedcombs, runner);
        
        // use found combsets to find subcombsets
        if (find_all_subcombsets(runner, reducedcombs) == 0 && count == 0) {
            num_results++;
            
            if (depth >= autostop) {
                printf("\nNew possible tournament (%d):\n", depth);
                combset_tree *uprunner = runner;
                while (uprunner != NULL) {
                    print_combset(uprunner->round);
                    uprunner = uprunner->parent;
                }
                
                if (autostop != -1) { 
                    printf("\nPress ENTER for next result...");
                    getchar();
                }
                
            } else {
                printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                printf("%d", num_results);
            }
        }
        
        destroy_combset(reducedcombs);
        
        // next child
        runner = runner->next_neighbour;
        
        // clear unused memory
        if (runner != NULL && runner->prev_neighbour != NULL) {
            destroy_combset_tree_childs(runner->prev_neighbour);
            destroy_combset_tree_element(runner->prev_neighbour);
        }
        count++;
    }
    
    depth--;
    return count;
}

int find_all_round_combsets(combset *worker, combset_tree *parent)
{
    int *positions;
    combset_tree *temp;
    int counter = 0;
    
    // init
    init_positions(&positions);
    init_combset_tree(&temp);
    
    // find next combset and append to tree
    while (find_next_round_combset(temp->round, worker, positions, 0) == 1) {
        append_combset_tree_child(parent, temp);
        init_combset_tree(&temp);
        counter++;
    }
    
    // clean memory
    destroy_combset_tree_element(temp);
    destroy_positions(positions);
    
    return counter;
}

int find_next_round_combset(combset *round, combset *worker, int *positions, int depth)
{
    int worker_elements = combset_count(worker);
    combset *subworker = NULL;
    int depth_counter;
    
    // deepest position
    if (depth == (N_TABLES - 1)) {
        if (combset_count(worker) > positions[depth]) {
            // comb found
            append_comb(round, comb_at_position(worker, positions[depth]));
            positions[depth]++;
            return 1;
        } else {
            // no comb found
            return 0;
        }
    }
    
    for (;positions[depth] < worker_elements;) {
        // reset subworker
        destroy_combset(subworker);
        init_combset(&subworker);
        copy_combset(worker, subworker);
        
        // delete used players from subworker
        delete_comb_with_players(&subworker, comb_at_position(worker,positions[depth]), 0);
        
        if (find_next_round_combset(round, subworker, positions, depth + 1) == 1) {
            // subcombset found
            append_comb(round, comb_at_position(worker, positions[depth]));
            destroy_combset(subworker);
            return 1;
        } else {
            // nothing found; increment position for this depth
            positions[depth]++;
            // reset sub positions
            for (depth_counter = depth+1; depth_counter < N_TABLES; depth_counter++) {
                positions[depth_counter] = 0;
            }
        }
    }
    
    destroy_combset(subworker);
    
    return 0;
}

void init_comb(comb* c) 
{
    *c = (comb)malloc(N_PPT * sizeof(char));
}

void destroy_comb(comb c)
{
    free(c);
}

void copy_comb(comb src, comb *dst)
{
    int i;
    for (i = 0; i < N_PPT; i++) {
        (*dst)[i] = src[i];
    }
}

int compare_comb(comb c1, comb c2)
{
    int i;
    int count = 0;
    
    for (i = 0; i < N_PPT; i++) {
        count += player_in_comb(c1, c2[i]);
    }
    
    return count;
}

int player_in_comb(comb c, int p)
{
    int i;
    for (i = 0; i < N_PPT; i++) {
        if (c[i] == p) {
            return 1;
        }
    }
    return 0;
}


void init_combset(combset **cs) {
    *cs = (combset *)malloc(sizeof(combset));
    (*cs)->next = NULL;
    (*cs)->prev = NULL;
    (*cs)->comb = NULL;
}

void copy_combset(combset *src, combset *dst)
{
    combset *runner = src;
    do {
        append_comb(dst, runner->comb);
    } while (runner->next != NULL && (runner = runner->next));
}

void append_comb(combset *cs, comb c) {
    if (cs->comb == NULL) {
        // first comb
        init_comb(&(cs->comb));
        copy_comb(c, &(cs->comb));
    } else {
        // append
        combset *runner = cs;
        while (runner->next != NULL) { runner = runner->next; }
        combset *newcombset = NULL;
        init_combset(&newcombset);
        init_comb(&(newcombset->comb));
        copy_comb(c, &(newcombset->comb));
        runner->next = newcombset;
        newcombset->prev = runner;
    }
}

combset* find_comb(combset *cs, comb c)
{
    combset *runner = cs;
    
    do {
        if (compare_comb(runner->comb, c) == N_PPT) {
            return runner;
        }
    } while (runner->next != NULL && (runner = runner->next));
    
    return NULL;
}

comb comb_at_position(combset *cs, int p)
{
    combset *runner = cs;
    int i = 0;
    
    while (i != p && runner->next != NULL) { runner = runner->next; i++; }
    
    if (i == p) return runner->comb;
    return NULL;
}

void delete_first_comb(combset **cs)
{
    combset *pos = *cs;
    
    // connect prev with next
    if (pos->prev != NULL) { pos->prev->next = pos->next; }
    if (pos->next != NULL) { pos->next->prev = pos->prev; }
    
    // move pointer
    if ((*cs)->next != NULL) {
        *cs = (*cs)->next;
    } else if ((*cs)->prev != NULL) {
        *cs = (*cs)->prev;
    } else {
        // last element
        *cs = NULL;
    }
    
    free(pos->comb);
    free(pos);
}

void delete_comb(combset **cs, comb c)
{
    combset *pos = find_comb(*cs, c);
    combset *newpos = pos;
    
    if (pos != NULL) {
        delete_first_comb(&newpos);
        if (*cs == pos) {
            *cs = newpos;
        }
    }
}

void delete_comb_with_players(combset **cs, comb c, int n)
{
    // n = number of max matches per comb
    // 0 = remove all players from combset found in comb
    // 1 = allow one player to be in combset found in comb (no double games)
    
    combset *runner = *cs;
    combset *newpos;
    
    do {
        if (compare_comb(runner->comb, c) > n) {
            
            newpos = runner;
            delete_comb(&newpos, runner->comb);
            
            if (*cs == runner) {
                *cs = newpos;
            }
            
            if (newpos != runner) {
                runner = newpos;
            } else {
                runner = runner->next;
            }
        } else {
            runner = runner->next;
        }
    } while (runner != NULL);
}

void delete_comb_with_players_of_combset(combset **cs, combset *c, int n)
{
    combset *runner = c;
    
    while (runner != NULL) {
        delete_comb_with_players(cs, runner->comb, n);
        runner = runner->next;
    }
}

void build_all_comb(combset *cs)
{
    // builds all player combinations (N_PLAYERS over N_PPT)
    comb nextcomb;
    init_comb(&nextcomb);
    
    int i;
    // nextcomb start condition
    for (i = 0; i < N_PPT; i++) {
        nextcomb[i] = i+1;
    }
    
    i = 0;
    
    for (;;) {
        int j = N_PPT - 1;
        int of_pos = -1;
        
        // save new combination
        append_comb(cs, nextcomb);
        
        // next comb
        nextcomb[N_PPT - 1]++;
        
        // break condition
        if (nextcomb[0] == N_PLAYERS - (N_PPT - 1)) { break; }
        
        for (; j >= 0; j--) {
            // check overflows
            if (nextcomb[j] > (N_PLAYERS - ((N_PPT - 1) - j))) {
                // increase j -1
                if (j > 0) { nextcomb[j-1]++; nextcomb[j] = nextcomb[j-1]+1; }
                // reset following values
                of_pos = j;
                for (; of_pos < (N_PPT - 1); of_pos++) {
                    nextcomb[of_pos + 1] = nextcomb[of_pos] + 1;
                }
            }
        }
        
        i++;
    }
    
    destroy_comb(nextcomb);
}

int combset_count(combset *cs)
{
    if (cs == NULL) return 0;
    if (cs->comb == NULL) return 0;
    
    combset *runner = cs;
    int c = 0;
    do {
        c++;
    } 
    while (runner->next != NULL && (runner = runner->next));
    
    return c;
}

void destroy_combset(combset *cs)
{
    if (cs == NULL) {
        return;
    }
    
    combset *runner = cs;
    combset *next;
    
    int i = 0;
    int c = combset_count(cs);
    
    if (c == 0) {
        // Just delete container
        free(runner);
    }
    
    for (i = 0; i < c; i++) {
        next = runner->next;
        free(runner->comb);
        free(runner);
        runner = next;
    }
    
    cs = NULL;
}

void init_positions(int** pos)
{
    int i;
    *pos = (int *)malloc(N_TABLES * sizeof(int));
    
    for (i = 0; i < N_TABLES; i++) {
        (*pos)[i] = 0;
    }
}

void destroy_positions(int *pos)
{
    free(pos);
}

void init_combset_tree(combset_tree **tree)
{
    *tree = (combset_tree *)malloc(sizeof(combset_tree));
    
    (*tree)->parent         = NULL;
    (*tree)->childs         = NULL;
    (*tree)->last           = NULL;
    (*tree)->next_neighbour = NULL;
    (*tree)->prev_neighbour = NULL;
    
    init_combset(&((*tree)->round));
}

void destroy_combset_tree_element(combset_tree *tree)
{
    // modify predecessor
    if (tree->prev_neighbour != NULL) {
        if (tree->next_neighbour != NULL) {
            tree->prev_neighbour->next_neighbour = tree->next_neighbour;
        } else {
            tree->prev_neighbour->next_neighbour = NULL;
        }
    } else {
        // parent points to tree (childs)
        if (tree->parent != NULL) {
            if (tree->next_neighbour != NULL) {
                tree->parent->childs = tree->next_neighbour;
            } else {
                tree->parent->childs = NULL;
            }
        }
    }
    
    // modify successor
    if (tree->next_neighbour != NULL) {
        if (tree->prev_neighbour != NULL) {
            tree->next_neighbour->prev_neighbour = tree->prev_neighbour;
        } else {
            tree->next_neighbour->prev_neighbour = NULL;
        }
    } else {
        if (tree->parent != NULL) {
            // parent points to tree (last)
            if (tree->next_neighbour != NULL) {
                tree->parent->last = tree->next_neighbour;
            } else {
                tree->parent->last = NULL;
            }
        }
    }
    
    destroy_combset(tree->round);
    free(tree);
}

void destroy_combset_tree_childs(combset_tree *tree)
{
    combset_tree *runner = tree->childs;
    
    while (runner != NULL) {
        combset_tree *next = runner->next_neighbour;
        destroy_combset_tree_childs(runner);
        destroy_combset_tree_element(runner);
        runner = next;
    }
}

void append_combset_tree_child(combset_tree *parent, combset_tree *child)
{
    child->parent = parent;
    
    if (parent->childs == NULL) {
        // first child
        parent->childs = child;
    } else {
        // append
        parent->last->next_neighbour = child;
        child->prev_neighbour = parent->last;
    }
    parent->last = child;
}

void print_comb(comb c)
{
    int i;
    printf("Combination: [");
    
    for (i = 0; i < N_PPT; i++) {
        printf("%d", c[i]);
        if (N_PPT - i > 1) {
            printf(", ");
        }
    }
    
    printf("]\n");
}

void print_combset(combset *cs)
{
    if (cs->comb == NULL && cs->next == NULL) { return; }
    combset *runner = cs;
    int i = 0;
    do {
        print_comb(runner->comb);
        i++;
    } while (runner->next != NULL && (runner = runner->next));
    
    printf("Combset total: %d comb\n", i);
}