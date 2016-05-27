#pragma once

#include <stdbool.h>

struct slist;

struct slist *slist_append(struct slist *list, void *data);

struct slist *slist_remove(struct slist *list, void *data);

void slist_foreach(struct slist *list,
                   void (*handler)(void *data, void *userdata),
                   void *userdata);

struct slist *slist_find(struct slist *list, void *data);

struct slist *slist_find_custom(struct slist *list,
                                bool (*check)(void *data, void *userdata),
                                void *userdata);

void slist_free(struct slist *list, void (*cleanup)(void *data));
