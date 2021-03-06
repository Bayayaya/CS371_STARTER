#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include "Partition_table.h"

typedef struct{
	char* key;
	int val;
} kv_pair;

typedef struct fancy_iter{
	tree_iterator reg_iter;
	node* peek_node;
}fancy_iter;

typedef struct{
	pthread_cond_t cond;
	pthread_mutex_t lock; 
	void * space_ptr;
	bool is_empty;
} buffer;

//this struct will save all the mr_state that need to be passed to MREmit or MRReduce
typedef struct {
	void * thing;
	buffer* buffer_arr;
	int num_mappers;
	int num_reducers;
	node** root_ptr;
	fancy_iter* f_iter_arr;
} mr_state;

typedef void (*map_func)(mr_state mr_info, FILE* file, char* val);
typedef void (*reduce_func)(mr_state mr_info, char* key, void* something, int id);
typedef void (*pt_reduce_func)(mr_state mr_info, void* something, int id);
void MRReduce(mr_state mr_info, void* something, int id);
void MREmit(mr_state mr_info, char* new_word, int val);

typedef struct {
	map_func map;
//everything that MREmit & reduce needs
	mr_state mr_info;
	FILE* file;
	char* val;
	int id;
} map_caller_struct;

void* map_caller(void* arg){
	map_caller_struct* typed_arg;
	typed_arg = (map_caller_struct*)arg;
	// if(typed_arg->id == 0){
	// 	sleep(1);
	// } 
	typed_arg->map(typed_arg->mr_info, typed_arg->file,typed_arg->val);
	//MREmit(typed_arg->mr_info, NULL, 1);
	pid_t tid = gettid();
	printf("mapper caller %d return \n", tid);
	return NULL;
}

void fancy_iter_init(fancy_iter* f_iter, node* root){
	node_iter_init(&(f_iter->reg_iter), root);
	f_iter->peek_node = next_node(&(f_iter->reg_iter));
}

char* fancy_iter_peek(fancy_iter* f_iter){
	if(f_iter->peek_node==NULL){
		return NULL;
	}
	return f_iter->peek_node->data;
}

void* fancy_iter_next(fancy_iter* f_iter, char* key){
	if(f_iter->peek_node==NULL || strcmp(key,f_iter->peek_node->data)!=0){
		return NULL;
	} 

	node* ret = f_iter->peek_node;
	f_iter->peek_node = next_node(&f_iter->reg_iter);
	return (void*)"VAL SHOULD BE HERE";
}

typedef struct{
	reduce_func reduce;
	pt_reduce_func MRReduce;
	mr_state mr_info;
	char* key;
	void* something;
	int id;
}reducer_caller_struct;

void* reducer_caller(void* arg){
	reducer_caller_struct* typed_arg;
	typed_arg = (reducer_caller_struct*)arg;
	mr_state mr_info = typed_arg->mr_info;
	int id = typed_arg->id;

	while(1){
		char* key = fancy_iter_peek(&mr_info.f_iter_arr[id]);
		if(key==NULL){
			return NULL;
		}
		typed_arg->reduce(typed_arg->mr_info, key, typed_arg->something, id);
	}
}

void* pt_reducer_caller(void* arg){
	reducer_caller_struct* typed_arg;
	typed_arg = (reducer_caller_struct*)arg;
	typed_arg->MRReduce(typed_arg->mr_info, typed_arg->something, typed_arg->id);
	return NULL;
}

void MRRun(char* path, map_func map, reduce_func reduce, int num_mappers, int num_reducers){
	//open path directory and open all the files in directory
	int file_num = 0;
	FILE* file_list[100];
	while(1){
		//file path format: 10_splits/cybersla.0n
		char* file_name = (char*)malloc(strlen("cybersla.00")+1);
		sprintf(file_name,"cybersla.%02d",file_num);
		char * file_path = (char*)malloc(strlen(path)+strlen(file_name)+2);
		sprintf(file_path,"%s/%s",path,file_name);
		printf("print path %s\n",file_path );

		FILE* fp = fopen(file_path, "r");
		if( fp != NULL){
			file_list[file_num] = fp;
			file_num ++;
		} else {
			break;
		}

	}

	mr_state mr_info;
	mr_info.num_mappers = num_mappers;
	mr_info.num_reducers = num_reducers;
	printf("mr_info reducers %d\n", mr_info.num_reducers);

	//create buffer for each map_thread
	buffer buffer[num_reducers];
	mr_info.buffer_arr = buffer;
	//set the is_empty flag in buffer
	for(int i=0; i<num_reducers; i++){
		buffer[i].is_empty=true;
		pthread_mutex_init(&(buffer[i].lock),NULL);
		pthread_cond_init(&(buffer[i].cond),NULL);
	}

	
	node* root[num_reducers];
	for(int i = 0; i<num_reducers; i++){
		root[i] = NULL;
	}
	mr_info.root_ptr = root;

	for(int i = 0; i<num_reducers; i++){
		printf("root* %d is %p\n",i,root[i]);
	}

	
	//create pt_reducer threads
	pthread_t pt_reducer_threads[num_reducers];
	reducer_caller_struct pt_reducer_caller_wrapper[num_reducers];
	
	for(int i = 0; i<num_reducers; i++){

		//TODO: need to figure out argv that passed into map
		pt_reducer_caller_wrapper[i].MRReduce = MRReduce;
		pt_reducer_caller_wrapper[i].mr_info = mr_info;
		pt_reducer_caller_wrapper[i].id = i;
		pt_reducer_caller_wrapper[i].something = NULL;
		printf("pthread_create pt_reducer %d\n", i);
		if(pthread_create(&pt_reducer_threads[i], NULL, pt_reducer_caller, (void*)&pt_reducer_caller_wrapper[i]) != 0){
			fprintf(stderr, "error: Cannot create pt_reducer thread # %d\n", i);
			break;
		}
	}

	//pass each file in file list to different map_caller_wrapper
	map_caller_struct map_caller_wrapper[num_mappers];
	//create mapper threads
	pthread_t mapper_threads[num_mappers];


	int assinged_files=0;
	while(assinged_files<=file_num){
		int thread_created = num_mappers;
		for(int j=0; j<num_mappers; j++){
			map_caller_wrapper[j].file = file_list[assinged_files];
			map_caller_wrapper[j].map = map;
			map_caller_wrapper[j].mr_info = mr_info;
			map_caller_wrapper[j].id=j;
			assinged_files++;
			if(assinged_files>file_num){
				//after we assign all the files, break out of for loop
				thread_created = file_num%num_mappers;
				break;
			}
			//call map
			if(pthread_create(&mapper_threads[j], NULL, map_caller, (void*)&map_caller_wrapper[j]) != 0){
				printf("error: Cannot create mapper thread # %d\n", j);
			}
		}

		//join all mapper_thread
		for(int j=0; j<thread_created; j++){
			if(pthread_join(mapper_threads[j],NULL) != 0){
				printf("error: Cannot join thread # %d\n", j);
			}
		}
		//next round of assingment for all mappers
	}

	//send NULL signal into each buffer and close it
	for(int i=0; i<num_reducers; i++){
		pthread_mutex_lock(&(mr_info.buffer_arr[i].lock));
		while(!mr_info.buffer_arr[i].is_empty){
			pthread_cond_wait(&(mr_info.buffer_arr[i].cond),&(mr_info.buffer_arr[i].lock));
		}
		mr_info.buffer_arr[i].is_empty = false;
		printf("put %p in buffer num %d \n",NULL,i );
		mr_info.buffer_arr[i].space_ptr = (void*) NULL;
		pthread_cond_signal (&(mr_info.buffer_arr[i].cond));
		pthread_mutex_unlock (&(mr_info.buffer_arr[i].lock));
	}

	for(int i=0; i<num_reducers; i++){
		if(pthread_join(pt_reducer_threads[i],NULL) != 0){
			printf("error: Cannot join pt_reducer_thread # %d\n", i);
		}
	}

	//print_tree(root[0]);
	//print_tree(root[1]);

	//test tree iterator

	// tree_iterator tree_iter_arr[num_reducers];
	// for(int i = 0; i<num_reducers; i++){
	// 	node_iter_init(&tree_iter_arr[i],root[i]);
	// }
	// node* node_got;
	// while((node_got = next_node(&tree_iter_arr[0]))!=NULL){
	// 	printf("iter tree 0 node %s\n", node_got->data);
	// }

	// while((node_got = next_node(&tree_iter_arr[1]))!=NULL){
	// 	printf("iter tree 1 node %s\n", node_got->data);
	// }

	fancy_iter f_iter[num_reducers];
	for(int i=0; i<num_reducers; i++){
		fancy_iter_init(&f_iter[i],root[i]);
	}

	mr_info.f_iter_arr = f_iter;


	//create reducer threads
	pthread_t reducer_threads[num_reducers];
	reducer_caller_struct reducer_caller_wrapper[num_reducers];

	for(int i = 0; i<num_reducers; i++){
		//TODO: need to figure out argv that passed into map
		reducer_caller_wrapper[i].reduce = reduce;
		reducer_caller_wrapper[i].mr_info = mr_info;
		reducer_caller_wrapper[i].id = i;
		reducer_caller_wrapper[i].something = NULL;
		if(pthread_create(&reducer_threads[i], NULL, reducer_caller, (void*)&reducer_caller_wrapper[i]) != 0){
			fprintf(stderr, "error: Cannot create reducer thread # %d\n", i);
			break;
		}
	}

	for(int i=0; i<num_reducers; i++){
		if(pthread_join(reducer_threads[i],NULL) != 0){
			printf("error: Cannot join pt_reducer_thread # %d\n", i);
		}
	}


}

long Partitioner (char* key, int num_partitions){
	int len = strlen(key);
	long hash = 5381;
	long c;
	for(int i=0; i<len; i++){
		c = (long)key[i];
		hash = hash*33+c;
	}
	long ret = hash % num_partitions;
	if(ret<0){
		return ret+num_partitions;
	} else {
		return ret;
	}
}

void MREmit(mr_state mr_info, char* new_word, int val){
	//MREmit will take the key val pair and send it to buffer
	//use hash function to decide which buffer it should send to
	//then take lock, check is_empty,send new_word, set condvar
	int num_partitions = mr_info.num_reducers;
	int buffer_num = Partitioner(new_word,num_partitions);
	assert(buffer_num >= 0);
	assert(buffer_num<mr_info.num_reducers);
	buffer* buffer_arr = mr_info.buffer_arr;
	// printf("deadlock MREmit %s is going to take lock\n", new_word ); 
	pthread_mutex_lock(&(buffer_arr[buffer_num].lock));
	while(!buffer_arr[buffer_num].is_empty){
		pthread_cond_wait(&(buffer_arr[buffer_num].cond),&(buffer_arr[buffer_num].lock));
	}
	// printf("deadlock MREmit %s has lock now\n", new_word );

	buffer_arr[buffer_num].is_empty = false;
	//printf("put %s in buffer num %d \n",new_word,buffer_num );
	buffer_arr[buffer_num].space_ptr = (void*) new_word;
	// printf("deadlock MREmit %s is going to set signal\n", new_word );

	pthread_cond_broadcast (&(buffer_arr[buffer_num].cond));
	// printf("deadlock MREmit %s is going to release lock\n", new_word );

	pthread_mutex_unlock (&(buffer_arr[buffer_num].lock));
}

void MRReduce(mr_state mr_info, void* something, int id){
	//MRReduce will take char* from buffer and add it to partition_table
	buffer* buffer_arr = mr_info.buffer_arr;
	char* new_word;
	while(1){
		//printf("deadlock MRReduce %d is going to take lock for %s\n",id, new_word);
		pthread_mutex_lock(&(buffer_arr[id].lock));
		while(buffer_arr[id].is_empty){
			pthread_cond_wait(&(buffer_arr[id].cond),&(buffer_arr[id].lock));
		}
		//printf("deadlock MRReduce %d has lock for %s now\n",id, new_word);
		new_word = (char*)buffer_arr[id].space_ptr;
		//printf("MRReduce receive word %s from buffer num %d\n",new_word, id );
		buffer_arr[id].is_empty = true;
		//printf("deadlock MRReduce %d set signal for %s now\n",id, new_word);
		pthread_cond_signal (&(buffer_arr[id].cond));
		//printf("deadlock MRReduce %d is going to release lock for %s now\n",id, new_word);
		pid_t tid = gettid();
		//printf("reducer tid is %d\n", tid);
		pthread_mutex_unlock (&(buffer_arr[id].lock));
		if(new_word == NULL){
			break;
		}else {
			insert_node(new_word,&mr_info.root_ptr[id]);
		}
	}

	//printf("reducer %d return \n", id);
	return;
}

//malloc space for string that read from file. The receiver need to free buffer.
char* get_word(FILE * file){
	int ch;
	int temp_size = 2;
	int word_len = 0;
	char* temp = (char*)calloc(temp_size, sizeof(char));
	while((ch = fgetc(file)) != EOF && ch != (int)' ' && ch != (int)'\t' && ch != (int)'\n' && ch != (int)'\0'){
		//make sure temp_size has space to topy next ch and NULL
		if(word_len>temp_size-2){
			temp_size *= 2;
			temp = (char*) realloc(temp, temp_size*sizeof(char));
			
		} 
		temp[word_len] = (char)ch;
		word_len++;
	}
	temp[word_len]='\0';
	if(word_len==0 && ch ==EOF){
		temp =NULL;
	}	
	return temp;
}


void map(mr_state mr_info, FILE* file, char* val){

	while(1){
		char* new_word = get_word(file);

		if(new_word != NULL && new_word[0] != '\0'){
			//printf("before MREmit, new_word is %s\n",new_word);
			MREmit(mr_info, new_word, 1);
		}else {
			if(new_word == NULL){

				//printf("before return in map\n");
				return;
			}
		}
	}
}


void* MRGetNext(mr_state mr_info, char* key, int id){

	return fancy_iter_next(&mr_info.f_iter_arr[id],key);

}

void MRPostProcess(mr_state mr_info, char* key, int count){

	printf("key is %s, count is %d\n",key,count);

}

void reduce(mr_state mr_info, char* key, void* something, int id){
	//printf("this is reducer number %d\n", id);
	node * root_ptr = mr_info.root_ptr[id];
	//print_tree(root_ptr);

	int count = 0;
	while((MRGetNext(mr_info,key,id))!=NULL){
		count++;
		
	}
	MRPostProcess(mr_info,key,count);
}

int main (int argc, char *argv[]){
		//printf("New Test Start!\n");
		MRRun("../10_splits", map, reduce, 5, 5);
	return 0;
}
