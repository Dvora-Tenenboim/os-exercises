#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fnmatch.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>


typedef struct List_node list_node;
struct List_node
{
	char *path;
	list_node *next;
};

//----------Global varibles-------------
pthread_mutex_t qlock;
pthread_mutex_t readdir_lock;
pthread_mutex_t stat_lock;
pthread_mutex_t wait_lock;
pthread_cond_t not_empty;
pthread_cond_t threads_finished;
int threads_counter;
list_node* fifo_head;
list_node* fifo_tail;
pthread_t* thread_ids;
int threads_num;
int num_of_found_files = 0;

//----------Functions declarations-------------
void* print_pathes(void* params);
void treat_file(const char* path, const char *str);
list_node* create_node(const char *path);
void delete_node(list_node* pnode);
int isEqual(const char *path, const char *str);
void init_mutexes();
void destroy_mutexes();
void init_conditions();
void destroy_conditions();
void unlock_mutexses();
void create_threads(char* str);
void cancel_threads();
void care_if_fifo_empty();
void enqueue(char* str);
void dequeue();
bool is_dir(struct stat dir_stat);
void free_queue();
void my_signal_handler(int signum);
void register_signal_handling();

//--------------Auxilary Functions-----------------

list_node* create_node(const char *path)
{
	list_node* new_node = malloc(sizeof(list_node));
	new_node -> path = malloc(strlen(path)+1);
	strcpy(new_node -> path, path);
	new_node -> next = NULL;
	return new_node;
}
void delete_node(list_node* pnode)
{
	free(pnode->path);
	free(pnode);
	pnode=NULL;
}

}*/
int isEqual(const char* path, const char* str)
{
	const char* tmp_str;
	const char* tmp_path;
	while (*path != '\0')
	{
		tmp_str = str;
		tmp_path = path;
		while (true)
		{
			if (*tmp_str == '\0')
			{
				return true;
			}
			if (*tmp_path == '\0')
			{
				return false;
			}
			if (*tmp_str == *tmp_path)
			{
				tmp_str++;
				tmp_path++;
			}
			else
			{
				break;
			}
		}
		path++;
	}
	return false;
}
void treat_file(const char* path, const char *str)
{
	struct stat file;
	pthread_mutex_lock(&stat_lock);
	stat(path, &file);
	pthread_mutex_unlock(&stat_lock);
	if(isEqual(path,str))
	{
		printf("%s\n", path);
		__sync_fetch_and_add(&num_of_found_files, 1);
	}
}

void create_threads(char* str)
{
	int rc;
	thread_ids=malloc(sizeof(pthread_t)*threads_num);
	for (int i=0; i<threads_num; i++)
	{
		rc=pthread_create(&thread_ids[i],NULL,print_pathes,(void*)str);
		if(rc)
		{
			printf("error in pthread_create(): %s\n",strerror(rc));
			exit(1);
		}
	}
}

void cancel_threads()
{
	for (int i=0; i<threads_num; i++)
	{
		pthread_cancel(thread_ids[i]);
		unlock_mutexses();
	}
	free(thread_ids);
	thread_ids = NULL;
}

void care_if_fifo_empty()
{
	while(!fifo_head)
	{
		pthread_testcancel();
		if(threads_counter==1)
		{
			printf("Done searching, found %d files\n",num_of_found_files);
			pthread_cond_signal(&threads_finished);
		}
		__sync_fetch_and_sub(&threads_counter, 1);
		pthread_cond_wait(&not_empty, &qlock);
		__sync_fetch_and_add(&threads_counter, 1);
	}
}
void enqueue(char* str)
{
	pthread_mutex_lock(&qlock);
	list_node* new_node = create_node(str);
	if (!fifo_tail)
	{
		fifo_head=new_node;
		fifo_tail=new_node;
	}
	else
	{
		fifo_tail->next = new_node;
		fifo_tail=fifo_tail->next;
	}
	pthread_cond_signal(&not_empty);
	pthread_mutex_unlock(&qlock);
	pthread_testcancel();
}

void dequeue()
{
	if(fifo_head == fifo_tail)
	{
		fifo_head=NULL;
		fifo_tail=NULL;
	}
	else
	{
		fifo_head = fifo_head->next;
	}
}

bool is_dir(struct stat dir_stat)
{
	return (dir_stat.st_mode & __S_IFMT) == __S_IFDIR;
}

void free_queue()
{
	list_node* curr = fifo_head;
	while(curr)
	{
		list_node* next = fifo_head->next;
		delete_node(curr);
		curr = next;
	}
}

void init_mutexes()
{
	pthread_mutex_init(&qlock,NULL);
	pthread_mutex_init(&readdir_lock,NULL);
	pthread_mutex_init(&stat_lock,NULL);
	pthread_mutex_init(&wait_lock,NULL);
}

void destroy_mutexes()
{
	pthread_mutex_destroy(&qlock);
	pthread_mutex_destroy(&readdir_lock);
	pthread_mutex_destroy(&stat_lock);
	pthread_mutex_destroy(&wait_lock);
}

void init_conditions()
{
	pthread_cond_init(&not_empty,NULL);
	pthread_cond_init(&threads_finished,NULL);
}
void destroy_conditions()
{
	pthread_cond_destroy(&not_empty);
	pthread_cond_destroy(&threads_finished);
}
void unlock_mutexses()
{
	pthread_mutex_unlock(&qlock);
	pthread_mutex_unlock(&readdir_lock);
	pthread_mutex_unlock(&stat_lock);
}

void my_signal_handler(int signum)
{
	pthread_cond_signal(&threads_finished);
	printf("search stopped, found %d files\n",num_of_found_files);
}

void register_signal_handling()
{
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));

    new_action.sa_handler = my_signal_handler;
    
    // Overwrite default behavior for ctrl+c
    if (sigaction(SIGINT, &new_action, NULL) == -1)
	{
        perror("Signal handle registration failed");
        exit(1);
    }
}

//------------End Auxilary Functions---------------

void* print_pathes(void* params)
{
	char* str = (char*)params;
	char* dir_path;
	DIR *dir;
	struct dirent *entry;
	struct stat dir_stat;
	pthread_mutex_lock(&qlock);
	care_if_fifo_empty();
	pthread_testcancel();
	list_node* orig_head=fifo_head;
	dir_path=fifo_head->path;
	dequeue();
	pthread_mutex_unlock(&qlock);
	dir = opendir(dir_path);
	pthread_testcancel();
	if (!dir)
	{ 
		printf("cannot open directory: %s\n",dir_path);
		__sync_fetch_and_sub(&threads_counter, 1);
		pthread_exit((void *)1);
	}
	pthread_mutex_lock(&readdir_lock);
	entry = readdir(dir);
	pthread_mutex_unlock(&readdir_lock);
	while(entry)
	{
		char buff[strlen(dir_path)+strlen(entry->d_name)+2];
		sprintf(buff,"%s/%s",dir_path, entry->d_name);
		pthread_mutex_lock(&stat_lock);
		stat(buff, &dir_stat);
		pthread_mutex_unlock(&stat_lock);
		if(strcmp(entry->d_name,"..") != 0 && strcmp(entry->d_name, ".") != 0)
		{
			if(is_dir(dir_stat))
			{
				enqueue(buff);
			}
			else
			{
				treat_file(buff, str);
			}	
		 }
		 pthread_mutex_lock(&readdir_lock);
		 entry = readdir(dir);
		 pthread_mutex_unlock(&readdir_lock);
	}
	closedir(dir);
	delete_node(orig_head);
	pthread_testcancel();
	print_pathes(str);
	return params;
}

int main(int argc, char **argv)
{ 
	if(argc < 4)
	{
		exit(1);
	}
	register_signal_handling();
	threads_num=atoi(argv[3]);
	threads_counter = threads_num;
	fifo_head=create_node(argv[1]);
	fifo_tail=fifo_head;
	char* str=argv[2];
	init_mutexes();
	init_conditions();
	create_threads(str);
	pthread_mutex_lock(&wait_lock);
	pthread_cond_wait(&threads_finished, &wait_lock);
	pthread_mutex_unlock(&wait_lock);
	cancel_threads();
	free_queue();
	destroy_mutexes();
	destroy_conditions();
	return 0;
}


