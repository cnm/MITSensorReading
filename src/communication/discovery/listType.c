#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include "listType.h"

void CreateList(LList *list)
{
    list->NumEl = 0;
    list->pFirst = NULL;
    list->pLast = NULL;
}

LElement *AddToList(void *item, LList *list)
{
    //check inputs
    assert(item!=NULL); 
    assert(list!=NULL);
    //Create generic element to hold item ptr
    LElement *NewEl;
    NewEl = (LElement *)malloc(sizeof(NewEl));  //create generic element
    assert(NewEl != NULL);
    list->NumEl = list->NumEl + 1;
    NewEl->data = item;
    if (list->NumEl == 1)
    {
      list->pFirst = NewEl;
      NewEl->prev = NULL;
      NewEl->next = NULL;
    }
    else
    {
      NewEl->prev = list->pLast;
      NewEl->next = NULL;
      list->pLast->next = NewEl;
    }
    list->pLast = NewEl;
    return NewEl;
}

bool DelFromList(unsigned short num, LList * list){
	unsigned short i;
	LElement *curr;
	bool deleted = false;
	//check inputs
    assert(num <= list->NumEl); 
    assert(list!=NULL);
    
    for (i=1, curr=list->pFirst; i <= num; i++, curr=curr->next){
		if (i == num){
			if (curr->next != NULL){
				curr->prev->next = curr->next;
				curr->next->prev = curr->prev;
			}else{
				curr->prev->next = NULL;
			}
			free(curr);
			deleted = true;
		}
	}
	return deleted;
}
