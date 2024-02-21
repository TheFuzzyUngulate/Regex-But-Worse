#include <stdio.h>
#include <stdlib.h>
#include "regex_but_worse.h"

#define MY_REGEX_PR 0
#define MY_REGEX_OPT 1
#define MY_REGEX_REP 2
#define MY_REGEX_ALT 3
#define MY_REGEX_CLOSE 4
#define MY_REGEX_SELECT 5

static inline void my_regex_error(const char* msg)
{
    fprintf(stderr, "regex error: %s\n", msg);
    exit(EXIT_FAILURE);
}

static int my_regex_item_add_level(my_regex_item* self, my_regex_item* item)
{
    if (self->type == MY_REGEX_ALT) {
        if (self->levels.count >= self->levels.capacity) {
            self->levels.capacity *= 2;
            self->levels.levels = (my_regex_item**)realloc(
                self->levels.levels,
                self->levels.capacity * sizeof(my_regex_item*)
            );
        } self->levels.levels[self->levels.count++] = item;
        return 1;
    } else return 0;
}

static my_regex_item* my_regex_item_get_child(my_regex_item* self)
{
    switch (self->type)
    {
        case MY_REGEX_ALT:
            return self->levels.levels[self->levels.count-1];

        case MY_REGEX_REP:
        case MY_REGEX_OPT:
        case MY_REGEX_CLOSE:
        case MY_REGEX_SELECT:
            return self->levels.levels[0];

        default:
            my_regex_error("cannot get child from primitive");
    }
}

static my_regex_item* my_regex_item_index_child(my_regex_item* self, unsigned int index)
{
    if (self->type == MY_REGEX_ALT) {
        if (index < self->levels.count)
            return self->levels.levels[index];
        else my_regex_error("index out of bounds");
    } 
    else my_regex_error("search by index only available for alt trees");
}

static my_regex_item* my_regex_item_add_child(my_regex_item* self, my_regex_item* item)
{
    switch (self->type)
    {
        case MY_REGEX_ALT:
        {
            item->par = self;
            if (self->levels.count > 0) {
                my_regex_item* old = my_regex_item_get_child(self);
                if (old != NULL) {
                    while (old->next != NULL)
                        old = old->next;
                    old->next = item;
                } else self->levels.levels[self->levels.count-1] = item;
            } else my_regex_item_add_level(self, item);
            break;
        }

        case MY_REGEX_REP:
        case MY_REGEX_OPT:
        case MY_REGEX_CLOSE:
        case MY_REGEX_SELECT:
        {
            item->par = self;
            my_regex_item* child = my_regex_item_get_child(self);

            if (child != NULL) {
                my_regex_item* old = child;
                while (old->next != NULL)
                    old = old->next;
                old->next = item;
            } else self->levels.levels[0] = item;
            break;
        }

        default:
            my_regex_error("cannot add child to primitive");
    }
}

static my_regex_item* my_regex_item_remove_child(my_regex_item* self)
{
    switch (self->type)
    {
        case MY_REGEX_ALT:
        {
            if (self->levels.count > 0)
            {
                my_regex_item* old = my_regex_item_get_child(self);
                if (old->next == NULL)
                {
                    self->levels.levels[self->levels.count-1] = NULL;
                    old->par = NULL;
                    return old;
                }
                else
                {
                    while (old->next->next != NULL)
                        old = old->next;
                    my_regex_item* result = old->next;
                    old->next = NULL;
                    result->par = NULL;
                    return result;
                }
            } else my_regex_error("no child available");
            break;
        }

        case MY_REGEX_REP:
        case MY_REGEX_OPT:
        case MY_REGEX_CLOSE:
        case MY_REGEX_SELECT:
        {
            my_regex_item* child = self->levels.levels[0];
            if (child != NULL)
            {
                my_regex_item* old = child;
                if (old->next == NULL)
                {
                    self->levels.levels[0] = NULL;
                    old->par = NULL;
                    return old;

                }
                else
                {
                    while (old->next->next != NULL)
                        old = old->next;
                    my_regex_item* result = old->next;
                    old->next = NULL;
                    result->par = NULL;
                    return result;
                }
            } else my_regex_error("no child available");
            break;
        }
        
        default:
            my_regex_error("primitive has no children");
    }
}

static int my_regex_item_graft(my_regex_item* self)
{
    if (self->type != MY_REGEX_PR && self->type != MY_REGEX_ALT) {
        my_regex_item* child = my_regex_item_get_child(self);
        my_regex_item* leaf = my_regex_item_init(MY_REGEX_ALT, NULL);
        
        my_regex_item_add_child(leaf, child);
        leaf->par = self;
        my_regex_item_add_level(leaf, NULL);
        self->levels.levels[0] = leaf;
        return 1;
    } else return 0;
}

void my_regex_item_print(my_regex_item* root, int INDENT)
{
    int c = root->type;
    for (int i = 0; i < INDENT*4; ++i)
        putchar(' ');

    if (c == MY_REGEX_PR)
    {
        printf("prim: %s\n", root->name);
    }

    else
    if (c == MY_REGEX_ALT)
    {
        printf("%s:\n", root->name);
        for (int i = 0; i < root->levels.count; ++i)
        {
            for (int i = 0; i < (INDENT+1)*4; ++i)
                putchar(' ');
            printf("level %i:\n", i);
            my_regex_item* ptr = root->levels.levels[i];
            while (ptr != NULL) {
                my_regex_item_print(ptr, INDENT+2);
                ptr = ptr->next;
            }
        }
    }

    else
    {
        printf("%s:\n", root->name);
        my_regex_item* ptr = my_regex_item_get_child(root);
        while (ptr != NULL) {
            my_regex_item_print(ptr, INDENT+1);
            ptr = ptr->next;
        }
    }
}

static void my_regex_checker_load(my_regex_checker* self)
{
    my_regex_item* root = my_regex_item_init(MY_REGEX_CLOSE, "root");
    my_regex_item* head = root;

    int i = 0;
    while (i < strlen(self->str_reg))
    {
        char ch = self->str_reg[i++];
        switch (ch)
        {
            case '{': 
            {
                my_regex_item* k = my_regex_item_init(MY_REGEX_REP, NULL);
                my_regex_item_add_child(head, k);
                head = k;
                break;
            }

            case '(':
            {
                my_regex_item* k = my_regex_item_init(MY_REGEX_CLOSE, NULL);
                my_regex_item_add_child(head, k);
                head = k;
                break;
            }

            case '[':
            {
                my_regex_item* k = my_regex_item_init(MY_REGEX_SELECT, NULL);
                my_regex_item_add_child(head, k);
                head = k;
                break;
            }

            case '?':
            {
                my_regex_item* r = my_regex_item_remove_child(head);
                my_regex_item* k = my_regex_item_init(MY_REGEX_OPT, NULL);
                my_regex_item_add_child(k, r);
                my_regex_item_add_child(head, k);
                break;
            }

            case '|':
            {
                if (head->type != MY_REGEX_ALT)
                {
                    my_regex_item_graft(head);
                    head = my_regex_item_get_child(head);
                } else my_regex_item_add_level(head, NULL);
                break;
            }

            case '\\':
            {
                ch = self->str_reg[i++];
                char* str = (char*)malloc(3 * sizeof(char));

                str[2] = 0;
                str[1] = ch;
                str[0] = '\\';

                my_regex_item_add_child(
                    head,
                    my_regex_item_init(MY_REGEX_PR, str)
                );

                break;
            }

            case '}':
            case ']':
            case ')':
            {
                if (head->type == MY_REGEX_ALT)
                    head = head->par->par;
                else head = head->par;
                break;
            }

            default:
            {
                char* str = (char*)calloc(4, sizeof(char));
                str[0] = ch;

                if (i < strlen(self->str_reg))
                {
                    char y = self->str_reg[i++];
                    if (y == '-' && i < strlen(self->str_reg))
                    {
                        y = self->str_reg[i++];
                        str[1] = '-';
                        str[2] = y;

                    } else --i;
                }

                my_regex_item_add_child(
                    head, 
                    my_regex_item_init(MY_REGEX_PR, str)
                );
                
                break;
            }
        }
    }
    
    self->reg = root;
}

static int my_regex_checker_verify_inner(my_regex_checker* self, my_regex_item* expr);
static int my_regex_checker_verify_outer(my_regex_checker* self, my_regex_item* expr);

static int my_regex_checker_verify_inner(my_regex_checker* self, my_regex_item* expr)
{
    if (expr == NULL) return 1;
    switch (expr->type)
    {
        case MY_REGEX_ALT:
        {
            for (int i = 0; i < expr->levels.count; ++i) {
                my_regex_item* el_i = my_regex_item_index_child(expr, i);
                if (my_regex_checker_verify_outer(self, el_i)) return 1;
            } return 0;
            break;
        }

        case MY_REGEX_REP:
        {
            int old = self->index;
            my_regex_item* snd = expr->next;
            my_regex_item* ofst = my_regex_item_get_child(expr);

            if (my_regex_checker_verify_outer(self, ofst)) {
                int bt_size = 1, bt_capacity = 4;
                int* bt_indices = (int*)malloc(bt_capacity * sizeof(int));
                
                bt_indices[0] = self->index;
                
                while (my_regex_checker_verify_outer(self, ofst)) {
                    if (bt_size >= bt_capacity) {
                        bt_capacity = bt_capacity * 3 / 2;
                        bt_indices = (int*)realloc(bt_indices, bt_capacity * sizeof(int));
                    } bt_indices[bt_size++] = self->index;
                }

                while (1) {
                    if (my_regex_checker_verify_outer(self, snd)) {
                        free(bt_indices);
                        return 1;
                    }
                    if (bt_size == 0) {
                        free(bt_indices);
                        return 0;
                    }
                    self->index = bt_indices[--bt_size];
                }
            }

            self->index = old;
            return 0;
        }

        case MY_REGEX_OPT:
        {   
            my_regex_item* snd = expr->next;
            my_regex_item* ofst = my_regex_item_get_child(expr);
            my_regex_checker_verify_outer(self, ofst);
            return my_regex_checker_verify_outer(self, snd);
        }

        case MY_REGEX_CLOSE:
        {
            my_regex_item* snd = expr->next;
            my_regex_item* body = my_regex_item_get_child(expr);
            if (my_regex_checker_verify_outer(self, body))
                return my_regex_checker_verify_outer(self, snd);
            else return 0;
        }

        case MY_REGEX_SELECT:
        {
            my_regex_item* snd = expr->next;
            my_regex_item* ofst = my_regex_item_get_child(expr);

            if (ofst->next != NULL)
            {
                my_regex_item* osnd = my_regex_item_init(MY_REGEX_ALT, "select");
                osnd->next = snd;
                my_regex_item_add_child(osnd, ofst->next);
                ofst->next = NULL;
                int tr = my_regex_checker_verify_outer(self, ofst);
                ofst->next = my_regex_item_get_child(osnd);

                if (tr) return my_regex_checker_verify_outer(self, snd);
                else return my_regex_checker_verify_outer(self, osnd);
            }
            else
            {
                if (my_regex_checker_verify_outer(self, ofst))
                    return my_regex_checker_verify_outer(self, snd);
                else return 0;
            }
            break;
        }

        case MY_REGEX_PR:
        {
            int c;
            int gotten;
            char* exp_name;
            char act_name[2];
            
            exp_name = (char*)expr->name;
            c = snprintf(act_name, 2 * sizeof(char), "%c", self->str_in[self->index]);
            if (c < 0) my_regex_error("error in string operation");
            
            if (strlen(exp_name) == 2 && exp_name[0] == '\\')
                gotten = act_name[0] == exp_name[1];
            else
            if (strlen(exp_name) == 3 && exp_name[1] == '-')
                gotten = (exp_name[0] <= act_name[0]) && (act_name[0] <= exp_name[2]);
            else
                gotten = (strcmp((const char*)act_name, (const char*)exp_name) == 0);

            if (gotten) {
                self->index++;
                return my_regex_checker_verify_outer(self, expr->next);
            } else return 0;
        }
    }

    return 0;
}

static int my_regex_checker_verify_outer(my_regex_checker* self, my_regex_item* expr)
{
    int old = self->index;
    if (!my_regex_checker_verify_inner(self, expr)) {
        self->index = old;
        return 0;
    } else return 1;
}

int my_regex_checker_verify(my_regex_checker* self, const char* str)
{
    self->str_in = str;
    return (my_regex_checker_verify_outer(self, self->reg) && self->index >= strlen(self->str_in));
}

my_regex_item* my_regex_item_init(int type, const char* name)
{
    my_regex_item* k = (my_regex_item*)malloc(sizeof(my_regex_item));

    k->par = NULL;
    k->next = NULL;
    k->prev = NULL;
    k->type = type;
    k->levels.count = 0;
    k->levels.capacity = 1;
    k->levels.levels = (my_regex_item**)calloc(1, sizeof(my_regex_item*));

    if (name == NULL) {
        switch (type)
        {
            case MY_REGEX_PR: 
                my_regex_error("primitive must have name");
                break;
            case MY_REGEX_OPT: 
                k->name = "opt";
                break;
            case MY_REGEX_ALT: 
                k->name = "alt";
                break;
            case MY_REGEX_REP:
                k->name = "rep";
                break;
            case MY_REGEX_CLOSE:
                k->name = "closed";
                break;
            case MY_REGEX_SELECT:
                k->name = "select";
                break;
            default:
                my_regex_error("illegal regex structure");
        }
    } else k->name = name;

    return k;
}

my_regex_checker* my_regex_checker_init(const char* str1)
{
    my_regex_checker* k;
    k = (my_regex_checker*)malloc(sizeof(my_regex_checker));

    k->index = 0;
    k->reg = NULL;
    k->str_in = NULL;
    k->str_reg = str1;
    my_regex_checker_load(k);

    return k;
}

#undef MY_REGEX_PR
#undef MY_REGEX_OPT
#undef MY_REGEX_REP
#undef MY_REGEX_ALT
#undef MY_REGEX_CLOSE
#undef MY_REGEX_SELECT