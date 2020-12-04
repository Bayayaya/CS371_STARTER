	/**
* 1. C program to count number of characters, words and lines in a text file.
* 2. Open all files in directry and count differet words and times
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

//create a buffer and put word in it, return pointer to buffer. The receiver need to free buffer.
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

/*
 * Logic to count characters, words and lines.
 */
int wordcount(FILE* file, int* characters, int* words, int* lines, char*** r_word_arr_ptr, int** r_wordnum_arr_ptr){
	int r =0;
	int saved_words = 0;
	int arr_size = 100;
	char** word_arr_ptr = (char**)calloc(arr_size, sizeof(char*));
	int* wordnum_arr_ptr = (int*)calloc(arr_size, sizeof(int));

	
	while(1){
		next_loop:
		char* new_word = get_word(file);
		//printf("new_word return %s\n", new_word);
		if(new_word != NULL && new_word[0] != '\0' ){
			//compare new_word with existing words, if exist, increase wordnum_arr count
			for(int i =0; i<saved_words; i++){
				
				if(word_arr_ptr[i] != NULL){
					if(strcmp(new_word, word_arr_ptr[i])==0){

						wordnum_arr_ptr[i]++;
						goto next_loop;
					}
				}
				
			}
			//new_word is not in exiting word list, add it to list
			//if saved_words >= arr_size then grow array before put new word in array
			
			if(saved_words >= arr_size-1){
				arr_size *= 2;
				word_arr_ptr = (char**)realloc(word_arr_ptr, arr_size*sizeof(char*));

				wordnum_arr_ptr = (int*)realloc(wordnum_arr_ptr, arr_size*sizeof(int));
			}
			word_arr_ptr[saved_words] = new_word;
			wordnum_arr_ptr[saved_words]=1;
			saved_words++;
		}
		if(new_word==NULL){
			break;
		}
	}

	*words = saved_words;
	*r_word_arr_ptr = word_arr_ptr;
	*r_wordnum_arr_ptr = wordnum_arr_ptr;

	return r;

}

int main()
{
	FILE * file, * result;
	char path[100];

//char ch;
	int characters, words, lines;
	int r;


/* Input path of files to merge to third file */
	printf("Enter source file path: ");
	scanf("%s", path); //"10_splits"
	int file_num = 0;

	//open result file
			result = fopen("countresult.out", "w");

			if (result == NULL)
			{
				printf("\nUnable to create result file.\n");
				exit(EXIT_FAILURE);
			}




	while(1){
		//file path format: 10_splits/cybersla.0n
		char* file_name = (char*)malloc(strlen("cybersla.00")+1);

		sprintf(file_name,"cybersla.%02d",file_num);
		char * file_path = (char*)malloc(strlen(path)+strlen(file_name)+2);
		sprintf(file_path,"%s/%s",path,file_name);
		printf("print path %s\n",file_path );
		fflush(stdout);
/* Open source files in 'r' mode */
		file = fopen(file_path, "r");


/* Check if file opened successfully */
		if (file == NULL)
		{
			printf("\nUnable to open file. '%s'\n",file_path);
			printf("Please check if file exists and you have read privilege.\n");

			exit(EXIT_FAILURE);
		}
		
		file_num++;

		characters = 0;
		words = 0; 
		lines = 0;

		char** word_arr_ptr;
		int* wordnum_arr_ptr;

		r = wordcount(file,&characters,&words,&lines,&word_arr_ptr,&wordnum_arr_ptr);
		assert(r==0);

		/* Print file statistics */
				fprintf(result, "\n");
				fprintf(result, "Total words      = %d\n", words);
				for(int i = 0; i< words; i++){
					printf("printing word %d \n",i );
					fprintf(result, "%d	",i+1);
					fprintf(result, "		|%s|", word_arr_ptr[i]);
					fprintf(result, "	|%d|\n",wordnum_arr_ptr[i]);
				}	



/* Close files to release resources */
		fclose(file);

	}


	fclose(result);
	return 0;
}