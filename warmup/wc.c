#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include "common.h"
#include "wc.h"
// #define MAX 2147483647
// #define DEBUG

//Duplcate the Hash algo in Java
typedef struct node {
    long startPtr;
    long endPtr;
    struct node *next;
    int occur_times;
} Node;

struct wc {
    /* you can define this struct to have whatever fields you want. */
    Node **hashtable;
    char *word_array;
    long size;
};

bool compair_str(char *word_array, long start1, long end1, long start2, long end2) {
#ifdef DEBUG
    printf("%ld,%ld;%ld,%ld\n", start1,  end1,  start2,  end2);
#endif
    if(end1 - start1 != end2 - start2)
        return false;

#ifdef DEBUG
    assert(end1 > start1 && end2 > start2);
    printf("string 1:");
    for(int i = start1; i < end1 ; i++) {
        printf("%c", word_array[i]);
    }
    printf("\n");
    printf("string 2:");
    for(int i = start2; i < end2 ; i++) {
        printf("%c", word_array[i]);
    }
    printf("\n");
#endif

    for(int offset = 0 ; start1 + offset < end1; offset++) {
        if(word_array[offset + start1] != word_array[offset + start2])
            return false;
    }
    return true;
}

void print_string(struct wc *wc, long start, long end) {
    for (long x = start; x < end ; x++) {
        printf("%c", wc->word_array[x]);
    }
    return;
}
struct wc *
wc_init(char *word_array, long size) {
    struct wc *wc;
    long MAX = size;
    wc = (struct wc *)malloc(sizeof(struct wc));
    wc->hashtable = (Node **)malloc(sizeof(Node *) * MAX);
    wc->size = MAX;
    assert(wc);
    memset(wc->hashtable, 0, sizeof(Node *) * MAX);
    wc->word_array = malloc(sizeof(char) * (size + 1));
    memcpy(wc->word_array, word_array, sizeof(char) * (size + 1));
    unsigned long key = 0;
    long last_end = 0;

#ifdef DEBUG
    printf("started, size = %ld\n", size);
    printf("word_array addr = %p\n", word_array);
#endif

    for (long cur = 0 ; cur < size ; cur++) {
#ifdef DEBUG
        printf("%c", word_array[cur]);
#endif

        if (isspace(word_array[cur])) {
#ifdef DEBUG
            printf("\nCurrent key: %ld \n", key % MAX);
#endif

            Node *startingNode = wc->hashtable[key];

#ifdef DEBUG
            printf("Current starting ptr: %p \n", startingNode);
#endif

            if (startingNode) {
                bool found = false;
                Node *last = NULL;
                while(startingNode != NULL) {
                    if (compair_str(word_array, startingNode->startPtr, startingNode->endPtr, last_end, cur)) {
                        found = true;
                        break;
                    }
                    last = startingNode;
                    startingNode = startingNode->next;
                }

                if (!startingNode) {
                    startingNode = last;
                }


                if (found) {
                    startingNode -> occur_times++;
#ifdef DEBUG
                    printf("Ins to already exists ones\n");
#endif
                } else {
                    startingNode -> next = malloc(sizeof(Node));
                    startingNode -> next -> occur_times = 1;
                    //Node * curNode = startingNode -> next;
                    startingNode -> next -> startPtr = last_end ;
                    startingNode -> next -> endPtr = cur;
                    // startingNode = startingNode -> next;
                }
            } else {
                startingNode = malloc(sizeof(Node));
                wc->hashtable[key % MAX] = startingNode;
                startingNode -> occur_times = 1;
                startingNode -> startPtr = last_end ;
                startingNode -> endPtr = cur;
            }

#ifdef DEBUG
            printf("Add to HT: %ld ~ %ld, key: %ld, size:%ld\n", startingNode -> startPtr, startingNode -> endPtr, key % MAX, cur - last_end);
#endif

            key = 0 ;
            while(cur < size - 1 && isspace(word_array[cur + 1])) {
                cur++;
            }
            last_end = cur + 1;
        } else {
            key = (key << 5) + key + word_array[cur]; // http://www.cse.yorku.ca/~oz/hash.html
            key %= MAX;
        }

    }
    return wc;
}

void
wc_output(struct wc *wc) {
#ifdef DEBUG
    printf("Start Output\n");
#endif

    for (int cur = 0; cur < wc->size ; cur++) {
        if (wc->hashtable[cur]) {
            Node *startPoint = wc->hashtable[cur];
            while(startPoint) {
                print_string(wc, startPoint->startPtr, startPoint->endPtr);
                printf(":%d\n", startPoint->occur_times);
                startPoint = startPoint->next;
            }
        }
    }
}

void
wc_destroy(struct wc *wc) {
    for(long i = 0 ; i < wc->size ; i++) {
        if (wc->hashtable[i]) {
            Node *nextptr = wc->hashtable[i]->next;
            while(nextptr) {
                nextptr = nextptr -> next;
                free(nextptr);
            }
        }
    }
    free(wc -> word_array);
    free(wc -> hashtable);
    free(wc);
}
