#include "List.h"


//add entry to linked list
void add_sync_entry(sync_info_mem_store** sync_list, const char* src, const char* tgt) {
    sync_info_mem_store* entry = malloc(sizeof(sync_info_mem_store));
    strcpy(entry->source_dir, src);
    strcpy(entry->target_dir, tgt);
    entry->last_sync_time = 0;
    entry->active = 0;
    entry->error_count = 0;
    entry->next = *sync_list;
    *sync_list = entry;
}

//check if entry exists in linked list
sync_info_mem_store* exists_sync_entry(sync_info_mem_store* sync_list, const char* src, const char* tgt) {
    sync_info_mem_store* current = sync_list;
    while (current!= NULL) {
        if (strcmp(current->source_dir, src) == 0) {
            if (strcmp(current->target_dir, tgt) == 0) {
                return current; //entry exists
            } else return NULL;       //error code, cannot have same source and different target
        }
        current = current->next;
    }
    return NULL;   //does not exist
}

//delete entry from linked list using source directory
void delete_sync_entry(sync_info_mem_store** sync_list, const char* src) {
    sync_info_mem_store* current = *sync_list;
    sync_info_mem_store* prev = NULL;
    while (current != NULL) {
        if (strcmp(current->source_dir, src) == 0) {
            if (prev == NULL) {
                *sync_list = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}