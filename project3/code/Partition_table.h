#include<stdio.h> 
#include<stdlib.h> 
#include<string.h>

typedef struct node{ 
	char* data; 
	struct node *left; 
	struct node *right; 
}node;

/* newNode() allocates a new node with the given data and NULL left and  
right pointers. */
struct node* newNode(char* data) { 
// Allocate memory for new node  
	struct node* node = (struct node*)malloc(sizeof(struct node)); 
	printf("creating a new node and data is %s\n", data);
// Assign data to this node 
	node->data = data; 

// Initialize left and right children as NULL 
	node->left = NULL; 
	node->right = NULL; 
	return(node); 
} 


void insert_node(char* data, node** root){
	node* new_node_ptr = newNode(data);
	while(1){
		if(*root == NULL){
			*root = new_node_ptr;
			return;
		} else {
			if(strcmp(data,(*root)->data)<=0){
				insert_node(data,&((*root)->left));
			}else{
				insert_node(data,&((*root)->right));
			}
		}
	}
}


//this need to be re-write
void free_tree(node* root){
	while(1)
	if(root->left==NULL && root->right == NULL){
		free(root);
		return;
	}
	if(root->left!=NULL){
		root = root->left;
		free_tree(root);
	} else {
		if(root->right!=NULL){
			root = root->right;
			free_tree(root);
		}
	}
}

