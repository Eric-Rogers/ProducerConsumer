//name: Eric Rogers
//email: ejr140230@utdallas.edu
//course number: CS3376.001

#include <iostream>
#include "cdk.h"
#include <boost/version.hpp>
#include <boost/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/date_time.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

using namespace std;

//defining matrix variables
#define MATRIX_WIDTH 5
#define MATRIX_HEIGHT 5
#define BOX_WIDTH 15
#define MATRIX_NAME_STRING "Worker Matrix"

//define the number of threads to spawn
#define NUM_THREADS 25

//initialize barrier, used to make sure threads all initialize properly
boost::barrier barrier(NUM_THREADS+1);

//parallel arrays, the thread's id is the position in vectors where its info is stored
int quantity[NUM_THREADS];
boost::interprocess::interprocess_semaphore* semaphores[NUM_THREADS];
boost::mutex* mutex[NUM_THREADS];

//thread group used to ensure all threads finish their work
boost::thread_group tgroup;

//mutex for updating display
boost::mutex lock;

//CDK display
CDKMATRIX *myMatrix;

//updates the display, is protected with a mutex since multiple threads will be ran and trying to update at the same time
void update_cell(int xpos, int ypos, int thread_id, char thread_status, int qty)
{
  //lock mutex
  lock.lock();

  //a stringstream that will hold the string that will be in cell updated
  stringstream hold;

  hold << "tid:" << thread_id << " S:" << thread_status << " Q:"  << qty;

  //update cell
  setCDKMatrixCell(myMatrix, ypos, xpos, hold.str().c_str());

  //draw new matrix
  drawCDKMatrix(myMatrix, true);

  //unlock the lock before completing to allow for other updates
  lock.unlock();
}

//increment quantity vector element thread_id(passed in) by amount(passed in), quantity is protected by mutex
void post(int thread_id, int amount)
{
  mutex[thread_id]->lock();

  quantity[thread_id] += amount;

  mutex[thread_id]->unlock();
}

//decrement quantity vector element thread_id(passed in) by 1, quantity is protected by mutex
void wait(int thread_id)
{
  mutex[thread_id]->lock();

  quantity[thread_id]--;

  mutex[thread_id]->unlock();
}

//function the consumer threads run, takes a xpos(column) and ypos(row) and the thread's id
void consumer(int xpos, int ypos, int thread_id)
{
  //initialize cell
  update_cell(xpos, ypos, thread_id, 'B', quantity[thread_id]);
  
  //wait on the barrier for all other threads
  barrier.wait();

  //keep looping
  while(true)
    {
      mutex[thread_id]->lock();
      
      //checking if it's bin(quantity) has nothing in it( = 0)
      if(quantity[thread_id] == 0)
	update_cell(xpos, ypos, thread_id, 'W', quantity[thread_id]); //update cell with waiting status
      
      mutex[thread_id]->unlock();

      //wait on their semaphone
      semaphores[thread_id]->wait();

      //lock for checking the quantity
      mutex[thread_id]->lock();

      //check if anything in quantity. If so, start consuming. If not, exit thread.
      if(quantity[thread_id] != 0)
	{
	  //unlock after we check the quantity
	  mutex[thread_id]->unlock();
	  
	  //update cell saying it is consuming
	  update_cell(xpos, ypos, thread_id, 'C', quantity[thread_id]); 

	  //consume(wait 1 second) 
	  boost::posix_time::seconds workTime(1);
	  boost::this_thread::sleep(workTime);
	  
	  //decrement quantity
	  wait(thread_id);
	}
      else
	{
	  //unlock after we check the quantity
	  mutex[thread_id]->unlock();

	  //update that the cell is exited
	  update_cell(xpos, ypos, thread_id, 'E', quantity[thread_id]);

	  //break from loop so that thread can complete its work
	  break;
	}
    }
}

//handles SIGINT and SIGTERM signals calling for all threads to exit and join before process exits
void sigHandler(int signum)
{
  //increment all the semaphores so that they wake after they are done with their work causing them to exit(see consumer function)
  for(int i = 0; i < NUM_THREADS; i++)
    {
      semaphores[i]->post();
    }
  //make sure all threads finish their work
  tgroup.join_all();

  //sleep so we can see all exited
  sleep(2);

  //end display
  endCDK();

  //exit process
  _exit(0);
}

int main(int argc, char* argv[])
{
  //CDK variables
  WINDOW *window;
  CDKSCREEN *cdkscreen;

  //initialize variables that are associated with the CDK screen
  const char *rowTitles[MATRIX_HEIGHT+1] = {"0", "a", "b", "c", "d", "e"};
  const char *columnTitles[MATRIX_WIDTH+1] = {"0", "a", "b", "c", "d", "e"};
  int boxWidths[MATRIX_WIDTH+1] = {BOX_WIDTH, BOX_WIDTH, BOX_WIDTH, BOX_WIDTH, BOX_WIDTH, BOX_WIDTH};
  int boxTypes[MATRIX_WIDTH+1] = {vMIXED, vMIXED, vMIXED, vMIXED, vMIXED, vMIXED};

  //initialize the display
  window = initscr();
  cdkscreen = initCDKScreen(window);

  /* Start CDK Colors */
  initCDKColor();

  //create the matrix that will display
  myMatrix = newCDKMatrix(cdkscreen, CENTER, CENTER, MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_WIDTH, MATRIX_HEIGHT,
                          MATRIX_NAME_STRING, (char **) columnTitles, (char **) rowTitles, boxWidths,
			  boxTypes, 1, 1, ' ', ROW, true, true, false);

  //check if there was an errir and if so warn the user and end the process
  if (myMatrix ==NULL)
    {
      printf("Error creating Matrix\n");
      _exit(1);
    }

  //thread pointer that we use to create our threads
  boost::thread *thethread;

  //column
  int x = 1;
  
  //row
  int y = 1;

  //create our threads and add their semaphores, mutex, and quantity to their respective arrays
  for(int i=0; i < NUM_THREADS; ++i)
    {
      semaphores[i] = new boost::interprocess::interprocess_semaphore(0);
      mutex[i] = new boost::mutex;
      quantity[i] = 0;

      //create new thread
      thethread = new boost::thread(consumer, x, y, i);

      //add the new thread to the thread_group
      tgroup.add_thread(thethread);
      
      //increment x(column) if we are not at the last column(column 5) 
      if(x+1 < 6)
	x++;
      //if we get to the last column increment y(row) and reset x(column) to 1
      else
	{
	  y++;
	  x = 1;
	}
    }

  //have the producer thread(in this case our main thread) sleep so that it will most likely be the last to the barrier
  sleep(5);

  //have producer wait at barrier
  barrier.wait();

  //sleep again so that we can see threads waiting
  sleep(5);

  //initialize signal handling
  signal(SIGTERM, sigHandler);
  signal(SIGINT, sigHandler);

  //initialize random number generator
  srand(time(NULL));

  //bin is the random bin we are adding to and num is the random(10-20) number we are adding
  int bin;
  int num;
  
  //producer thread runs until we get either SIGINT or SIGTERM
  while(true)
    {
      //generate random numbers
      bin = rand() % 25;
      num = rand() % (21 - 10) + 10;
      
      //increment the bin num times
      post(bin, num);

      //post the semaphore for bin num times
      for(int i = 0; i < num; i++)
	{
	  semaphores[bin]->post();
	}
    
      //sleep for 1 second between producing
      sleep(1);
    }
}
