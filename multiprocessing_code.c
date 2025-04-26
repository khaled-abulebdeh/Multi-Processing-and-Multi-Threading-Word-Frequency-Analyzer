#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/resource.h>

#define MAX_SIZE 17100000 //max number of words
#define TOP 10 // to find top 10 frequent words

int WORDS_NUM= 0;  //Total number of words in the loaded file
//int FREQUENT_WORDS=0;
int MAX_WORD_SIZE=0;
#define ARR_WIDTH 1000000
typedef struct word_cell* cell;
struct word_cell{
  int frequency;
  char* word;
} ;
struct result_cell {
	int frequency;
	char word [30];
};

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

//tree
typedef struct node* TNode;
	struct node {
	char* word ;
	TNode right;
	TNode left;
	int frequency;
};


///shared memory
// Shared memory structure containing the array and mutex
struct shared_memory {
	struct result_cell counted_words [8][ARR_WIDTH];  // Pointer to an array of pointers to result_cell

};
void initialize_shared_memory(struct shared_memory *sh_memory);
void child_process(struct shared_memory *sh_memory, int index, const char *word, int frequency);
void free_shared_memory(struct shared_memory *sh_memory);
//


HeapNode createHeapNode (char* word, int frequency);
MinHeap createHeap (int capacity);
void EnqueueHeap (MinHeap myHeap, char* word, int frequency);
void DequeueHeap (MinHeap myHeap);//frees the dequeued node
int isFullHeap (MinHeap myHeap);
int isEmptyHeap (MinHeap myHeap);
void free_heap(MinHeap myHeap);
void print_heap (MinHeap myHeap);
//

char** allocate_words ();
void deallocate_words (char **words);
void readFile (char** words);
void find_top_words (TNode combined_words, MinHeap myHeap);
cell create_cell (char* word, int frequency);
int check_results (char *word, cell results []);//checks if a specific word in the results list
int indexOf_min_frequency (cell results []); //returns the index of word min frequency
void count_frequency (int process_num, char** words,int start, int end, struct shared_memory *sh_memory);
void free_counted_words (cell* counted_words);

///tree
TNode createTNode();
TNode makeEmpty (TNode T);
TNode insertToTree (const char* word,int frequency, TNode T);
TNode findTNode (char* word, TNode T);
void printInOrder (TNode T);
//

int main(int argc, char* argv[])
{
	//initializations

	// Open a shared memory object
	const char *shm_name = "/result_cell_shm";
	int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
	if (shm_fd == -1) {
    	perror("shm_open failed");
    	return 1;
	}

	// Set the size of the shared memory (consider space for array of pointers + mutex)
	if (ftruncate(shm_fd, sizeof(struct shared_memory)) == -1) {
    	perror("ftruncate failed");
    	return 1;
	}

	// Map the shared memory into the address space of the process
	struct shared_memory *sh_memory = (struct shared_memory *)mmap(NULL, sizeof(struct shared_memory),
                                                          	PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (sh_memory == MAP_FAILED) {
    	perror("mmap failed");
    	return 1;
	}

	// Initialize memory (e.g., fill with zeros)
	memset(sh_memory, 0, sizeof(sh_memory));

	//stack size, we want to increase stack size for the process, to avoid overflow
	struct rlimit rl;

	// Set new stack size limit (16 MB)
	rl.rlim_cur = 16* 1024 * 1024;  // 16 MB
	if (setrlimit(RLIMIT_STACK, &rl) == -1) {
    	perror("setrlimit failed");
    	return 1;
	}

	//start of code

	//create an array to store words read from file
	char** words= allocate_words ();

	//load date from file and store it in "words" array
	readFile (words);
	printf("File has been read successfully\n\n");


	printf("Enter number of processes you want to run: ");
	int n_process;
	scanf("%d", &n_process);

	if (n_process<1)
    	n_process=1;//error detectetion

	//create (num_proc) processes as childs to the original process
	//we have only one parent, all other processes are childs for one root parent ; (to ease communication)

	//all processes have the data above
	int pid;
	int process_num=0;//parent takes number=0, while childs take a num from 1 to (n_process-1),

	for (int i=0; i<n_process-1; i++){// create (n_process -1) since the original root process is counted
    	pid= fork();
    	if (pid==0){
        	process_num=i+1;
        	break;//if child, don't fork again
    	}
	}
	/*
	- Now, all processes (inludeing root) will find top 10 words in its data slot
	- Each process will execute in different data set
	- Original data array will be divided among all processes  */

	//start & end are identifires for eacg process to determine what section of data must deal with
	int start= (WORDS_NUM*process_num)/n_process;
	int end= (WORDS_NUM*(process_num+1))/n_process;

	//find how many times each word has repeated
	count_frequency (process_num,words,start, end,sh_memory);

	if (process_num!=0){//child process
    	//child has finished its job, terminate
    	exit(0);
	}
	else {//parent process

    	//parent must wait for all childs
    	while (wait(NULL)>0  || errno!=ECHILD){
    	}

    	//parent must combine results from all children
    	//we will use a tree for that
    	TNode combined_words=NULL;
    	for (int i=0; i<8; i++){
        	for (int j=0; j<ARR_WIDTH && (sh_memory->counted_words[i][j]).frequency>0 ; j++){
            	combined_words=insertToTree((sh_memory->counted_words[i][j]).word,
                                    	(sh_memory->counted_words[i][j]).frequency, combined_words);

        	}
    	}

    	//then to find top 10 frequent words among combined words, we use a heap data structure
    	MinHeap myHeap= createHeap(TOP);
    	find_top_words(combined_words ,myHeap);

    	//now, heap stores final results
    	print_heap(myHeap);

    	free_heap(myHeap);
    	makeEmpty(combined_words);

	}

	deallocate_words(words);

	return 0;
}
//tree functions
TNode createTNode (){
	TNode T= (TNode) malloc (sizeof(struct node));
	if (T==NULL){
    	printf("OUT OF MEMORY..!!\n");
    	exit (0);
	}
	T->left= T->right= NULL;
	T->frequency =1;//
	return T;
}
TNode makeEmpty (TNode T){
	if (T!=NULL){
    	makeEmpty(T->left);
    	makeEmpty(T->right);
    	free(T->word);
    	free(T);
	}
	return NULL;
}
TNode insertToTree (const char* word,int frequency ,TNode T){
	//new node
	if (T==NULL){// correct position is found
    	//creating the node to insert it
    	T= createTNode();
    	T->word= (char*)malloc((strlen(word)+1)*sizeof(char));
    	strcpy(T->word, word);
    	(T->frequency)= frequency;
	}

	else if (strcasecmp(word,T->word)<0) // word < T->word
    	T->left= insertToTree (word,frequency,T->left);

	else if (strcasecmp(word,T->word)>0) // str > T->word
    	T->right= insertToTree (word,frequency,T->right);

	else {//repeated word
    	(T->frequency)+= frequency;
	}
	return T;
}
TNode findTNode (char* word, TNode T){
	if (T==NULL){
    	return NULL;//not found
	}
	else if (strcasecmp(word,T->word)<0){ // str < T->word
    	return findTNode (word,T->left);
	}
	else if (strcasecmp(word,T->word)>0){ // str > T->word
    	return findTNode (word,T->right);
	}
	else
    	return T;// word node is found
}
void printInOrder (TNode T){
	if (T!=NULL){
    	printInOrder(T->left);
    	printf("%20s --- *%-5d\n", T->word, T->frequency);
    	printInOrder(T->right);
	}
}
//
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
    	return NULL;
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
	for (int i=0; i<(myHeap->capacity)+1; i++){
    	if (myHeap->Array[i]){
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
void find_top_words (TNode combined_words, MinHeap myHeap){

	//uses morrisTraversal to traverse all nodes

	TNode current = combined_words;

	while (current != NULL) {
    	// If current node has a left child
    	if (current->left != NULL) {
        	// Find the rightmost node in the left subtree
        	TNode pre = current->left;
        	while (pre->right != NULL && pre->right != current) {
            	pre = pre->right;
        	}

        	// If rightmost node's right pointer is null, establish a thread
        	if (pre->right == NULL) {
            	pre->right = current;  // Establish thread
            	current = current->left;  // Move to left child
        	}
        	// If rightmost node's right pointer points to current node, it's time to visit the current node
        	else {
            	pre->right = NULL;  // Remove the thread

            	///action
            	//if heap is enpty or not full, enqueue this frequent word with its frequency
            	if (isEmptyHeap(myHeap) || !isFullHeap(myHeap)){
                	EnqueueHeap(myHeap,current->word, current->frequency);
            	}
            	else {
                	/* if heap is full, check if min frequency in heap is less that current frequency.
                    	if true, dequeue then enqueue the current word
                	*/

                	if (current->frequency > myHeap->Array[1]->frequency){//myHeap->Array[1]->frequency: is min value in minHeap
                    	DequeueHeap(myHeap);
                    	EnqueueHeap(myHeap,current->word, current->frequency);
                	}
            	}
            	//
            	current = current->right;  // Move to the right child
        	}
    	}
    	// If current node doesn't have a left child, visit it and move to the right child
    	else {
            	///action
            	//if heap is enpty or not full, enqueue this frequent word with its frequency
            	if (isEmptyHeap(myHeap) || !isFullHeap(myHeap)){
                	EnqueueHeap(myHeap,current->word, current->frequency);
            	}
            	else {
                	/* if heap is full, check if min frequency in heap is less that current frequency.
                    	if true, dequeue then enqueue the current word
                	*/

                	if (current->frequency > myHeap->Array[1]->frequency){//myHeap->Array[1]->frequency: is min value in minHeap
                    	DequeueHeap(myHeap);
                    	EnqueueHeap(myHeap,current->word, current->frequency);
                	}
            	}
            	//
            	current = current->right;
    	}
	}
}
void count_frequency (int process_num, char** words,int start, int end, struct shared_memory *sh_memory){

	for (int i=start; (i<end && i<WORDS_NUM) ; i++){

    	int isAlreadyFound=0; //boolean to check if word is in counted_words
    	int j=0;
    	//ARR_WIDTH is the max width of arrays allocating counted words
    	for (j=0; j<ARR_WIDTH && (sh_memory->counted_words[process_num][j]).frequency>0 ; j++){


        	if (strcasecmp(words[i], (sh_memory->counted_words[process_num][j]).word) ==0){
            	((sh_memory->counted_words[process_num][j]).frequency)++;
            	isAlreadyFound=1; //word is found
            	break;
        	}
    	}

    	if (!isAlreadyFound && j<ARR_WIDTH){
        	strcpy((sh_memory->counted_words[process_num][j]).word, words[i]);//frequency=1, first appearance
        	(sh_memory->counted_words[process_num][j]).frequency=1;
    	}
	}
}
cell create_cell (char* word, int frequency){
	cell myCell= (cell) malloc(sizeof(struct word_cell));
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

	/*
	//allocate each string
	for (int i=0; i<rows; i++){
    	words[i]= malloc((columns+1)* sizeof(char)); // +1 cell for '\0'
    	if (!words[i]){
        	perror ("Can't allocate a word!!");
        	exit(0);
    	}
	}
	*/
	return words; //double pointer char
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






