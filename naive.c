#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_SIZE 17100000 //max number of words
#define TOP 10 // to find top 10 frequent words

int WORDS_NUM= 0;  //Total number of words in the loaded file
int frequent_words=0;
int MAX_WORD_SIZE=0;

typedef struct result_cell* cell;
struct result_cell{
  int frequency;
  char* word;
} ;
//Heap functions
typedef struct node1* HeapNode;//to find most frequent words easily
typedef struct node2* MinHeap;//for Dijkstra Search for buildings
struct node1 {
    char* word;
    int frequency;
};
struct node2 {
    int capacity;
    int size;
    HeapNode* Array;
    //Double pointer, HeapNode is pointing a struct node, and there is a pointer pointing the array of those HeapNodes
};

char** allocate_words ();
void deallocate_words (char **words);
void readFile (char** words);
void find_top_words (cell* counted_words, MinHeap myHeap);
cell create_cell (char* word, int frequency);
int check_results (char *word, cell results []);//checks if a specific word in the results list
int indexOf_min_frequency (cell results []); //returns the index of word min frequency
void count_frequency (char** words, cell* counted_words);
void free_counted_words (cell* counted_words);
//heap
HeapNode createHeapNode (char* word, int frequency);
MinHeap createHeap (int capacity);
void EnqueueHeap (MinHeap myHeap, char* word, int frequency);
void DequeueHeap (MinHeap myHeap);//frees the dequeued node
int isFullHeap (MinHeap myHeap);
int isEmptyHeap (MinHeap myHeap);
void free_heap(MinHeap myHeap);
void print_heap (MinHeap myHeap);

int main (){
    printf("Srarting\n");

    //create an array to store words read from file
    char** words= allocate_words ();

    //create an array of structs (words + frequency), to find words frequency
    cell* counted_words= (cell*)malloc(MAX_SIZE*sizeof(struct result_cell));

    //load date from file and store it in "words" array
    readFile (words);
    printf("file has been read successfully.\n");

    //find how many times each word has repeated
    count_frequency(words,counted_words);

    //we will use heap to store top 10 words
    MinHeap myHeap= createHeap(TOP);
    find_top_words(counted_words,myHeap);
    print_heap(myHeap);

    //free cells that have been allocated dynamically
    //free_heap(myHeap);
    deallocate_words(words);
    free_counted_words(counted_words);

    printf("\nDynamic allocated memory have been freed\n");
    return 0;
}
//heap functions
HeapNode createHeapNode (char* word, int frequency){
    HeapNode myNode;
    myNode= (HeapNode) malloc (sizeof(struct node1));
    if (myNode==NULL){
        printf("OUT OF MEMORY");
        exit (0);
    }
    myNode->word= (char*)malloc((strlen(word)+1)*sizeof(char));
    strcpy(myNode->word, word);
    myNode->frequency= frequency;
    return myNode;
}
MinHeap createHeap (int capacity){
    if (capacity<2){//since the index 0 is unused in heaps
        do {
            printf("Sorry, the size is too small!!\n");
            printf ("Sorry, you must choose a capacity larger than 1\n");
            printf("Enter the capacity\n");
            scanf ("%d", &capacity);
        }
        while(capacity<2);
    }
    MinHeap myHeap;
    myHeap= (MinHeap) malloc (sizeof(struct node2));
    if (myHeap==NULL){
        printf("OUT OF MEMORY\n");
        exit(0);
    }

    myHeap->Array= (HeapNode*) malloc (sizeof(struct node1) * (capacity+1));//capacity+1 since index=0 is unused
    if (myHeap->Array==NULL){
        printf("OUT OF MEMORY\n");
        exit(0);
    }
    myHeap->capacity=capacity;
    myHeap->size=0;
    return myHeap;
}
void EnqueueHeap (MinHeap myHeap, char* word, int frequency){
    if (isFullHeap(myHeap)){
        printf("Sorry, Heap (Priority Queue) is Full !!\n");
        return;
    }

    HeapNode newNode= createHeapNode(word, frequency);
    (myHeap->size)++;//since we want to use the first empty position (index)

    myHeap->Array[myHeap->size]= newNode;

    for (int i=myHeap->size; ( (i>1) && myHeap->Array[i/2]->frequency > myHeap->Array[i]->frequency ) ; i/=2){//1!=1, to prevent accessing index=0 (1/2 =0)
        //myHeap->Array[i/2]: represents the parent of the node
        //the increment is i/=2 to keep tracking the parent roots, from a parent to another to find the needed position

        //here, we swap the node with its parent/s until finding the accepted position
        HeapNode temp= myHeap->Array[i];
        myHeap->Array[i]= myHeap->Array[i/2];
        myHeap->Array[i/2]= temp;
    }
}
void DequeueHeap (MinHeap myHeap){
    if (myHeap->size==0){//means it is empty
        return;
    }
    HeapNode deletedNode= myHeap->Array[1];//we always delete the root (which is in index=1)

    //by the following two ways of incrementing, I can access the needed chaild in the next lower level.
    int i=1;
    while ( 2*i <= myHeap->size ){

        //we set the base case it to access the left child
        //then we check who is the smallest, left child or right child?
        //then to swap it with the parent
        int childIndex= 2*i;

        //to check if the right child is smaller than the left one
        if ( (childIndex<myHeap->size) &&  (myHeap->Array[childIndex+1]->frequency < myHeap->Array[childIndex]->frequency) ){
            childIndex++;
        }

        //to reach here, means I must swap with the min child

        myHeap->Array[i]= myHeap->Array[childIndex];


        i=childIndex;
    }

    //Reaching here, means that the roots are swapped will to keep MinHeap property
    //now we will swap elements existing after the empty position

    for (int j=i; j<myHeap->size; j++){
        myHeap->Array[j]= myHeap->Array[j+1];
    }
    (myHeap->size)--;

    //return the Dequeued node
    free (deletedNode);
}
int isFullHeap (MinHeap myHeap){
    return myHeap->size== (myHeap->capacity);
}
int isEmptyHeap (MinHeap myHeap){
    return myHeap->size== 0;
}
void free_heap(MinHeap myHeap){
    if (!myHeap)
        return;

    for (int i=0; i<(myHeap->size)+1; i++){
        if (myHeap->Array[i]){
            if (myHeap->Array[i]->word)
                free(myHeap->Array[i]->word);

            free(myHeap->Array[i]);

        }
    }
    free (myHeap);
}
void print_heap (MinHeap myHeap){
    if (!myHeap)
        return;
    for (int i=1; i<myHeap->size; i++){//start from 1 since index 0 is unused in heap
        if (myHeap->Array[i])
            printf("%-10s --  %10d times\n", myHeap->Array[i]->word, myHeap->Array[i]->frequency);
    }
}

//

void count_frequency (char** words, cell* counted_words){
    for (int i=0; i<WORDS_NUM; i++){

        int isAlreadyFound=0; //boolean to check if word is in counted_words
        for (int j=0; j<frequent_words; j++){
            if (strcasecmp(words[i], counted_words[j]->word) ==0){
                (counted_words[j]->frequency)++;
                isAlreadyFound=1; //word is found
                break;
            }

        }
        if (!isAlreadyFound){
            counted_words[frequent_words]= create_cell(words[i], 1);//frequency=1, first appearance
            frequent_words++;
        }

    }
}
void find_top_words (cell* counted_words, MinHeap myHeap){
    for (int i=0; i<frequent_words; i++){

        ///action
        //if heap is enpty or not full, enqueue this frequent word with its frequency
        if (isEmptyHeap(myHeap) || !isFullHeap(myHeap)){
            EnqueueHeap(myHeap,counted_words[i]->word, counted_words[i]->frequency);
        }
        else {
            /* if heap is full, check if min frequency in heap is less that current frequency.
                if true, dequeue then enqueue the current word
            */

            if (counted_words[i]->frequency > myHeap->Array[1]->frequency){//myHeap->Array[1]->frequency: is min value in minHeap
                DequeueHeap(myHeap);
                EnqueueHeap(myHeap,counted_words[i]->word, counted_words[i]->frequency);
            }
        }
        //
    }
}

cell create_cell (char* word, int frequency){
    cell myCell= (cell) malloc(sizeof(struct result_cell));
    if (!myCell){
        perror("Can't allocate cells!\n");
        exit(0);
    }
    myCell->word= (char*)malloc((strlen(word)+1)*sizeof(char));
    strcpy(myCell->word, word);
    myCell->frequency=frequency;
    return myCell;
}
char** allocate_words (){

    //for simplicity
    int rows= MAX_SIZE;

    //allocate array of strings
    char** words= malloc(rows*sizeof(char*));
    if (!words){
        perror ("Can't allocate words!!");
        exit(0);
    }
    return words; //double pointer char
}
void free_counted_words (cell* counted_words){
    if (!counted_words)
        return;

    for (int i=0; i<frequent_words; i++){
        if (counted_words[i])
            free (counted_words[i]);
    }

    free (counted_words);
}
void deallocate_words (char **words){

    //to free 2D array of strings
    if (!words){
        return;
    }
    //for simplicity
    int rows= MAX_SIZE;


    for (int i=0; i<rows; i++){
        if (words[i]){
          free (words[i]);
          }
    }
    free (words);
}

void readFile (char** words){

    FILE *input;
    input= fopen("temp.txt","r");

    int line_size=4096 ; // 4096 is the max num of line char in .txt file on Linux system
    char scannedLine [line_size];
    char *valid= fgets(scannedLine,line_size,input);

    int index=0;
    while (valid!= NULL){
        if (scannedLine[strlen(scannedLine)-1] == '\n')
            scannedLine[strlen(scannedLine)-1]='\0';
        char *p= strtok(scannedLine, " ");
        while (p!=NULL){
            int word_length= strlen(p);
            if (word_length > MAX_WORD_SIZE)
                MAX_WORD_SIZE = word_length;
            if (WORDS_NUM+1 == MAX_SIZE)
            	return;

            words[WORDS_NUM]= malloc((word_length+1)* sizeof(char)); // +1 cell for '\0'
            strcpy(words[WORDS_NUM],p);
            WORDS_NUM+=1 ;

            p=strtok(NULL," ");//NULL is set where " " is found
        }
        valid= fgets(scannedLine,line_size,input);//for the next line
    }
    fclose(input);
}

