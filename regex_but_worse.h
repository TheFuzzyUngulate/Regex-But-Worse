#ifndef REGEX_BUT_WORSE_H
#define REGEX_BUT_WORSE_H
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct my_regex_item_levels
{
    unsigned int count;
    unsigned int capacity;
    struct my_regex_item** levels;
} my_regex_item_levels;

typedef struct my_regex_item
{
    int type;                                       // item type
    const char* name;                               // item name (?)
    struct my_regex_item* par;                      // parent of item
    struct my_regex_item* next;                     // item next to this
    struct my_regex_item* prev;                     // item before this
    my_regex_item_levels levels;                    // levels within item
} my_regex_item;

my_regex_item* my_regex_item_init(int type, const char* name);
void my_regex_item_print(my_regex_item* root, int INDENT);
/** int my_regex_item_add_level(my_regex_item* self, my_regex_item* item);
my_regex_item* my_regex_item_get_child(my_regex_item* self);
my_regex_item* my_regex_item_index_child(my_regex_item* self, unsigned int index);
my_regex_item* my_regex_item_add_child(my_regex_item* self, my_regex_item* item);
my_regex_item* my_regex_item_remove_child(my_regex_item* self);
int my_regex_item_graft(my_regex_item* self);**/

typedef struct my_regex_checker
{
    int index;                      // current index in string
    my_regex_item* reg;             // resultant regex item 
    const char* str_in;             // input string
    const char* str_reg;            // regex string        
} my_regex_checker;

my_regex_checker* my_regex_checker_init(const char* str1);
int my_regex_checker_verify(my_regex_checker* self, const char* str);
/**void my_regex_checker_load(my_regex_checker* self);
int my_regex_checker_verify_outer(my_regex_checker* self, my_regex_item* item);
int my_regex_checker_verify_inner(my_regex_checker* self, my_regex_item* item);**/

#endif