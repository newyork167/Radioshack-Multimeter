#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <pthread.h>

typedef struct thread
{
  int Running;
  int fd;
  pthread_t thread;
  char buffer[1024];
  struct thread *next;
  struct thread *prev;
} thread;

struct thread *freelist;
struct thread *worklist;
pthread_mutex_t freelist_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t worklist_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Parse and return file handle for http get request */
int OpenFile (char *buf, int n)
{
  int i, j;
  char *index = "index.html";
  char path[1024];

  if (buf == NULL || n < 10)
    return 0;

  // Ensure that the string starts with "GET "
  if (!(buf[0] == 'G' && buf[1] == 'E' && buf[2] == 'T' && buf[3] == ' '))
    return 0;

  // copy the string
  for (i = 0; i < n && buf[i + 4] != ' '; i++)
    path[i] = buf[i + 4];

  // put null on end of path
  path[i] = '\x00';

  // if the syntax doesn't have a space following a path then return
  if (buf[i + 4] != ' ' || i == 0)
    return 0;

  // if the last char is a slash, append index.html 
  if (path[i - 1] == '/')
    for (j = 0; j < 12; j++)
      path[i + j] = index[j];

  printf ("Found path: \"%s\"\n", path);

  // the +1 removes the leading slash
  return open (path + 1, O_RDONLY);
}

void HandleResonse (int fd, char *buf, int n)
{
  char badresponse[1024] = 
    "HTTP/1.1 404 Not Found\r\n\r\n<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\r\n<TITLE>Not Found</TITLE></HEAD><BODY>\r\n<H1>Not Found</H1>\r\n</BODY></HTML>\r\n\r\n";
  char goodresponse[1024] = "HTTP/1.1 200 OK\r\n\r\n";
  int fh;
  fh = OpenFile (buf, n);

  if (fh > 0)
    {
      struct stat stat_buf;	/* hold information about input file */
      send (fd, goodresponse, strlen (goodresponse), 0);
      /* size and permissions of fh */
      fstat (fh, &stat_buf);
      sendfile (fd, fh, NULL, stat_buf.st_size);
      close (fh);
    }
  else
    {
      send (fd, badresponse, strlen (badresponse), 0);
    }
}

void *handleRequest (void *arg)
{
  struct thread *t = (thread *) arg;
  int n;

  for (;;)
    {
      while (!t->Running)
	{
	  usleep (2000);
	}

      n = recv (t->fd, t->buffer, 1023, 0);

      if (n > 0)
	{  // got a request, process it.
	  HandleResonse (t->fd, t->buffer, n);
	}
	
      close (t->fd);
      t->Running = 0;

      pthread_mutex_lock(&worklist_mutex);
      if (worklist == t)
	worklist = t->next;
      if (t->next)
	t->next->prev = t->prev;
      if (t->prev)
	t->prev->next = t->next;
      pthread_mutex_unlock(&worklist_mutex);

      pthread_mutex_lock(&freelist_mutex);
      t->next = freelist;
      if (freelist)
	freelist->prev = t;
      freelist = t;
      pthread_mutex_unlock(&freelist_mutex);
    }
}

void ThreadInitialize (int ThreadCount)
{
  struct thread *t;
  int i;

  freelist = worklist = NULL;

  for (i = 0; i < ThreadCount; i++)
    {
      t = (struct thread *) calloc (1, sizeof(thread));

      pthread_mutex_lock(&freelist_mutex);
      // add new thread structure to freelist 
      t->next = freelist;
      freelist = t;
      pthread_mutex_unlock(&freelist_mutex);

      if (pthread_create (&t->thread, NULL, &handleRequest, t) != 0)
	{
	  perror ("pthread create failed\n");
	  exit (1);
	}
    }
}

void ThreadCheck ()
{
  pthread_mutex_lock(&worklist_mutex);
  struct thread *t = worklist;
  while (t != NULL)
    {
      if (!t->Running)
	{
	  // remove thread structure from worklist 
	  if (worklist == t)
	    worklist = t->next;
	  if (t->next)
	    t->next->prev = t->prev;
	  if (t->prev)
	    t->prev->next = t->next;

	  pthread_mutex_unlock(&worklist_mutex);
	  pthread_mutex_lock(&freelist_mutex);
	  // add thread structure to freelist 
	  t->next = freelist;
	  if (freelist)
	    freelist->prev = t;
	  freelist = t;
	  pthread_mutex_unlock(&freelist_mutex);
	  return;
	}
      t = t->next;
    }
  pthread_mutex_unlock(&worklist_mutex);
}

void ThreadHandleResponse (int fd)
{
  struct thread *t;

  /* wait for any child process to free up */
  while (1)
    {
      pthread_mutex_lock(&freelist_mutex);
      if (freelist != NULL)
	{
	  // remove thread structure from freelist 
	  t = freelist;
	  freelist = t->next;
	  if (freelist)
	    freelist->prev = NULL;
	  pthread_mutex_unlock(&freelist_mutex);

	  pthread_mutex_lock(&worklist_mutex);
	  // add thread structure to worklist 
	  t->next = worklist;
	  if (worklist)
	    worklist->prev = t;
	  worklist = t;
	  pthread_mutex_unlock(&worklist_mutex);

	  // setup thread and run it
	  t->fd = fd;
	  t->Running = 1;
	  return;
	}
      pthread_mutex_unlock(&freelist_mutex);

      ThreadCheck ();
      sched_yield();
    }
}

