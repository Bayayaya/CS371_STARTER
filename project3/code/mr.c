#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>

//this struct will save all the mr_state that need to be passed to MREmit or MRReduce
typedef struct {
	void * thing;
} mr_state;

typedef void (*map_func)(mr_state mr_info, FILE* file, char* val);
typedef void (*reduce_func)(mr_state mr_info, char* key, void* something);

typedef struct {
	map_func map;
//everything that MREmit & reduce needs
	mr_state mr_info;
	FILE* file;
	char* val;
} map_caller_struct;

void* map_caller(void* arg){
	map_caller_struct* typed_arg;
	typed_arg = (map_caller_struct*)arg;
	typed_arg->map(typed_arg->mr_info, typed_arg->file,typed_arg->val);
	return NULL;
}

typedef struct{
	reduce_func reduce;
	mr_state mr_info;
	char* key;
	void* something;
}reducer_caller_struct;

void* reducer_caller(void* arg){
	reducer_caller_struct* typed_arg;
	typed_arg = (reducer_caller_struct*)arg;
	typed_arg->reduce(typed_arg->mr_info, typed_arg->key, typed_arg->something);
	return NULL;
}

typedef struct{
	char* key;
	int val;
} kv_pair;

typedef struct{
	pthread_cond_t cond;
	pthread_mutex_t lock; 
	void * space_ptr;
	bool is_empty;
} buffer;


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
	
	//pass each file in file list to different map_caller_wrapper
	map_caller_struct map_caller_wrapper[num_mappers];
	//create mapper threads
	pthread_t mapper_threads[num_mappers];

	//create buffer for each map_thread
	buffer buffer[num_mappers];

	int assinged_files=0;
	while(assinged_files<=file_num){
		int thread_created = num_mappers;
		for(int j=0; j<num_mappers; j++){
			map_caller_wrapper[j].file = file_list[assinged_files];
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


	pthread_t reducer_threads[num_reducers];
	for(int i = 0; i<num_reducers; i++){
		//TODO: need to figure out argv that passed into map
		if(pthread_create(&reducer_threads[i], NULL, reducer_caller, NULL) != 0){
			fprintf(stderr, "error: Cannot create reducer thread # %d\n", i);
			break;
		}
	}

}
