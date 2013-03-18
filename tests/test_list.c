#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "librepo/rcodes.h"
#include "librepo/util.h"
#include "librepo/list.h"

#include "fixtures.h"
#include "testsys.h"
#include "test_list.h"

START_TEST(test_list_append)
{
    lr_List elem, list = NULL;

    list = lr_list_append(list, (void *) 1);
    fail_if(list == NULL);
    list = lr_list_append(list, (void *) 2);
    fail_if(list == NULL);

    elem = list;
    fail_if(elem->data != (void *) 1);
    elem = lr_list_next(elem);
    fail_if(elem->data != (void *) 2);
    elem = lr_list_next(elem);
    fail_if(elem != NULL);

    lr_list_free(list);
}
END_TEST

START_TEST(test_list_prepend)
{
    lr_List elem, list = NULL;

    list = lr_list_prepend(list, (void *) 1);
    fail_if(list == NULL);
    list = lr_list_prepend(list, (void *) 2);
    fail_if(list == NULL);

    elem = list;
    fail_if(elem->data != (void *) 2);
    elem = lr_list_next(elem);
    fail_if(elem->data != (void *) 1);
    elem = lr_list_next(elem);
    fail_if(elem != NULL);

    lr_list_free(list);
}
END_TEST

START_TEST(test_list_next_last_prev)
{
    lr_List elem, list = NULL;

    elem = lr_list_next(list);
    fail_if(elem != NULL);
    elem = lr_list_prev(list);
    fail_if(elem != NULL);

    list = lr_list_append(list, (void *) 1);
    fail_if(list == NULL);
    list = lr_list_append(list, (void *) 2);
    fail_if(list == NULL);

    // Check _next
    elem = list;
    fail_if(elem->data != (void *) 1);
    elem = lr_list_next(elem);
    fail_if(elem->data != (void *) 2);
    elem = lr_list_next(elem);
    fail_if(elem != NULL);
    elem = lr_list_next(elem);
    fail_if(elem != NULL);

    // Check _last
    elem = lr_list_last(NULL);
    fail_if(elem != NULL);
    elem = lr_list_last(list);
    fail_if(elem->data != (void *) 2);

    // Check _first
    elem = lr_list_first(NULL);
    fail_if(elem != NULL);
    elem = lr_list_last(list);
    elem = lr_list_first(elem);
    fail_if(elem->data != (void *) 1);

    // Check _prev
    elem = lr_list_last(elem);
    elem = lr_list_prev(elem);
    fail_if(elem->data != (void *) 1);
    elem = lr_list_prev(elem);
    fail_if(elem != NULL);

    lr_list_free(list);
}
END_TEST

void lr_my_free(void *data) { lr_free(data); }

START_TEST(test_list_free_full)
{
    void *data;
    lr_List list = NULL;
    lr_list_free_full(list, lr_my_free);

    data = lr_malloc(sizeof(int));
    *((int *) data) = 1;
    list = lr_list_prepend(list, data);
    data = lr_malloc(sizeof(int));
    *((int *) data) = 2;
    list = lr_list_prepend(list, data);
    lr_list_free_full(list, lr_my_free);
}
END_TEST

START_TEST(test_list_length)
{
    lr_List list = NULL;
    size_t len;

    len = lr_list_length(list);
    fail_if(len != 0);

    list = lr_list_prepend(list, NULL);
    len = lr_list_length(list);
    fail_if(len != 1);

    list = lr_list_prepend(list, NULL);
    len = lr_list_length(list);
    fail_if(len != 2);

    lr_list_free(list);
}
END_TEST

START_TEST(test_list_remove)
{
    lr_List list = NULL;

    list = lr_list_remove(list, (void *) 2);
    fail_if(list != NULL);

    // Remove the first element
    list = lr_list_prepend(list, (void *) 1);
    list = lr_list_remove(list, (void *) 2);
    fail_if(lr_list_length(list) != 1);
    list = lr_list_remove(list, (void *) 1);
    fail_if(list != NULL);

    // Remove the last element
    list = lr_list_prepend(list, (void *) 4);
    list = lr_list_prepend(list, (void *) 3);
    list = lr_list_prepend(list, (void *) 2);
    list = lr_list_prepend(list, (void *) 1);
    fail_if(lr_list_length(list) != 4);
    list = lr_list_remove(list, (void *) 1);
    fail_if(lr_list_length(list) != 3);
    fail_if(list->data != (void *) 2);
    list = lr_list_remove(list, (void *) 4);
    fail_if(lr_list_length(list) != 2);
    fail_if(lr_list_next(list)->next != NULL);
    list = lr_list_remove(list, (void *) 3);
    list = lr_list_remove(list, (void *) 2);
    fail_if(list != NULL);

    // Remove element from the center
    list = lr_list_prepend(list, (void *) 3);
    list = lr_list_prepend(list, (void *) 2);
    list = lr_list_prepend(list, (void *) 1);
    fail_if(lr_list_length(list) != 3);
    list = lr_list_remove(list, (void *) 2);
    fail_if(lr_list_length(list) != 2);
    fail_if(list->data != (void *) 1);
    fail_if(lr_list_next(list)->data != (void *) 3);
    lr_list_free(list);
}
END_TEST

Suite *
list_suite(void)
{
    Suite *s = suite_create("list");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_list_prepend);
    tcase_add_test(tc, test_list_append);
    tcase_add_test(tc, test_list_next_last_prev);
    tcase_add_test(tc, test_list_free_full);
    tcase_add_test(tc, test_list_length);
    tcase_add_test(tc, test_list_remove);
    suite_add_tcase(s, tc);
    return s;
}
