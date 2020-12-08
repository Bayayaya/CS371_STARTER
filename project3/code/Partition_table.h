#include<stdio.h> 
#include<stdlib.h> 
#include<string.h>
#include<assert.h>

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
	if(*root == NULL){
		*root = newNode(data);
		printf("before return in insert_node, data is %s\n",data);
		return;
	} else {
		if(strcmp(data,(*root)->data)<=0){
			insert_node(data,&((*root)->left));
			printf("go to left\n");
		}else{
			insert_node(data,&((*root)->right));
			printf("go to right\n");
		}
	}
}

void print_tree_helper(node* root, int spaces){
	if(root == NULL){
		return;
	}
	print_tree_helper(root->left,spaces+1);
	for(int i =0;i<spaces; i++){
		printf("  ");
	}
	printf("print node %s\n",root->data);
	print_tree_helper(root->right,spaces+1);
}

void print_tree(node* root){
	print_tree_helper(root,0);
}

typedef struct iterator{
	node* node_stack[1000];
	//the current position in array
	int current;
	node* last_return;
}tree_iterator;

void node_iter_init(tree_iterator* iter, node* root){
	iter->current = 0;
	iter->last_return = NULL;
	iter->last_node_is_right = false;
	while(root!=NULL){
		iter->node_stack[iter->current]=root;
		iter->current ++;
		root = root->left;
	}
} 

void add_with_all_left_children(tree_iterator* iter, node* node_to_add){
	while(node_to_add != NULL){
		iter->node_stack[iter->current] = node_to_add;
		iter->current ++;
		node_to_add = node_to_add->left;
	}
}


node* next_node(tree_iterator * iter){
	printf("start next_node\n");
	node* ret = NULL;

	while(iter->current != 0){
		if(iter->node_stack[(iter->current)-1]!=NULL){
			printf("iter current %d data is %s\n", iter->current, iter->node_stack[(iter->current)-1]->data);
		}
		if(iter->node_stack[(iter->current)-1]!=NULL && iter->last_return == NULL){
		//Push left node into stack until we push NULL onto stack
			printf("push left node because last_return = NULL \n");
			iter->node_stack[iter->current] = iter->node_stack[(iter->current)-1]->left;
			assert(iter->node_stack[(iter->current)-1]->left ==NULL);
			iter->current ++;
		} else

		if(iter->node_stack[(iter->current)-1]!=NULL && iter->node_stack[(iter->current)-1]->left == iter->last_return){
		//Push left NULL into stack until we push NULL onto stack
			printf("push NULL \n");
			iter->node_stack[iter->current] = NULL;
			iter->current ++;
		} else

		if(iter->node_stack[(iter->current)-1]!=NULL){
		//Push left node into stack until we push NULL onto stack
			printf("push left node\n");
			iter->node_stack[iter->current] = iter->node_stack[(iter->current)-1]->left;
			iter->current ++;
		}
		else{
				//node[current-1] == NULL; pop
			assert(iter->node_stack[(iter->current)-1]==NULL);
			iter->current--;
			ret = iter->node_stack[(iter->current)-1];
			iter->last_return = ret;
			printf("which node %s\n", ret->data);
			iter->current--;
				//push right node to stack
			
			if(iter->node_stack[(iter->current)]->right != NULL){
				iter->node_stack[(iter->current)] = iter->node_stack[(iter->current)]->right;
				iter->current++;
			}
			printf("iter current is %d before return \n",iter->current);
			return ret;
		}
	}
	return ret;
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

