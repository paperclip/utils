#include <stdlib.h>
#include <stdbool.h>

struct slist {
    void *data;
    struct slist *next;
};

struct slist *slist_append(struct slist *list, void *data)
{
    struct slist *nl = malloc(sizeof(*list));
    if (nl) {
        nl->data = data;
        nl->next = list;
    }

    return nl;
}

void slist_free(struct slist *list, void (*cleanup)(void *data))
{
    while (list) {
        if (list->data)
            cleanup(list->data);
        free(list);
        struct slist *next = list->next;
        list = next;
    }
}

struct slist *slist_remove(struct slist *list, void *data)
{
    if (!list)
        return NULL;

    struct slist *prev = NULL;
    struct slist *head = list;

    do {
        if (list->data == data) {
            if (prev)
                prev->next = list->next;
            else
                head = list->next;
            free(list);
            break;
        }
        prev = list;
        list = list->next;
    } while (list);

    return head;
}

void slist_foreach(
        struct slist *list,
        void (*handler)(void *data, void *userdata),
        void *userdata)
{
    while (list) {
        handler(list->data, userdata);
        list = list->next;
    }
}

struct slist *slist_find(struct slist *list, void *data)
{
    while (list) {
        if (list->data == data)
            break;
        list = list->next;
    }

    return list;
}

struct slist *slist_find_custom(
        struct slist *list,
        bool (*check)(void *data, void *userdata),
        void *userdata)
{
    while (list) {
        if (check(list->data, userdata))
            break;
        list = list->next;
    }

    return list;
}
