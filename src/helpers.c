#include "helpers.h"

void bgDeleter(void* data) {
    bgentry_t* bg_entry = data;
    if (bg_entry != NULL) {
        free_job(bg_entry->job);
    }
}

void historyDeleter(void* data) {
    char* line_cmd = data;
    if (line_cmd != NULL) {
        free(line_cmd);
    }
}

void DestroyHistory(list_t** list) {
    if (list == NULL || *list == NULL) { return; }
    node_t* current = (*list)->head;
    node_t* temp;
    while (current != NULL) {
        (*list)->deleter(current->data);
        temp = current->next;
        free(current);
        current = temp;
    }
    // Makes pointer to list_t memory available
    free(*list);
}

void DestroyBG(list_t** list) {
    if (list == NULL || *list == NULL) { return; }
    node_t* current = (*list)->head;
    node_t* temp;
    while (current != NULL) {
        (*list)->deleter(current->data);
        free(current->data);
        temp = current->next;
        free(current);
        current = temp;
    }
    // Makes pointer to list_t memory available
    free(*list);
}

int redirection_invalid_error(char* x1, char* x2, char* x3) {
    if (x2 != NULL) {
        if (!strcmp(x1, x2)) { return 1; }
    }
    if (x3 != NULL) {
        if (!strcmp(x1, x3)) { return 1; }
    }
    return 0;
}

void bgRemoveTerminated(list_t* list, pid_t wpid) {
    if (list != NULL) {
        node_t* head = NULL;
        // Start at the beginning of the list
        node_t* current = list->head;
        node_t* previous = NULL;
        node_t* temp;
        while (current != NULL) {
            bgentry_t* bgentry = current->data;
            if ((bgentry)->pid == wpid) {
                fprintf(stdout, BG_TERM, bgentry->pid, bgentry->job->line);
                list->length--;
                if (previous != NULL) {
                    previous->next = current->next;
                }
                list->deleter(current->data);
                free(current->data);
                temp = current->next;
                free(current);
                current = temp;
            }
            else {
                if (previous == NULL) {
                    // Sets head of list
                    head = current;
                }
                previous = current;
                // Continue iteration
                current = current->next;
            }
        }
        // Update list head
        list->head = head;
    }
}

void* removeTail(list_t* list) {
    if (list->length == 0) {
        return NULL;
    } else if (list->length == 1) {
        return removeHead(list);
    }

    void* retval = NULL;
    node_t* head = list->head;
    node_t* current = head;

    while (current->next->next != NULL) { 
        current = current->next;
    }

    retval = current->next->data;
    list->deleter(current->next->data);
    free(current->next);
    current->next = NULL;

    list->length--;

    return retval;
}

void* removeHead(list_t* list) {
    node_t** head = &(list->head);
    void* retval = NULL;
    node_t* next_node = NULL;

    if (list->length == 0) {
        return NULL;
    }

    next_node = (*head)->next;
    retval = (*head)->data;
    list->length--;

    node_t* temp = *head;
    *head = next_node;
    list->deleter(temp->data);
    free(temp);

    return retval;
}

void exitOnRedirectionError(job_info* job, char* line) {
    fprintf(stderr, "%s", RD_ERR);
    free_job(job);  
    free(line);
    validate_input(NULL);
    exit(EXIT_FAILURE);
}
