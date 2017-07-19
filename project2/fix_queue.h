#ifndef _QUEUE_H
#define _QUEUE_H
#include<stdio.h>
#include<stdlib.h>
typedef struct Node {
    char* data;
    int size;
    struct Node* next;
} Node;


// To Enqueue an integer
void Enqueue(Node** proot, Node* new_node) {
  if(*proot == NULL){
    *proot = new_node;
  }
  else{
    Node* tmp_node = *proot;
    while(tmp_node->next){
      tmp_node = tmp_node->next;
    }
    tmp_node->next = new_node;
  }
}

// To Dequeue an integer.
void Dequeue(Node** proot) {
  Node* temp = *proot;
  if(*proot == NULL) {
    return;
  }
  *proot = (*proot)->next;
  free(temp->data);
  free(temp);
}

Node* GetK(Node* root, int k){
  if(root == NULL) {
    return NULL;
  }
  Node* temp = root;
  while(k > 0 && temp->next){
    temp = temp->next;
    k--;
  }
  if(!k){
    return temp;
  }
  else{
    return NULL;
  }
}
#endif
