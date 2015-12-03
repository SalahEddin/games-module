/*******************************************
	SimpleThread.cpp

	Basic Windows threading techniques
	User must guess a key to press - input
	occurs in a seperate thread
********************************************/

#include <stdlib.h>
#include <conio.h>
#include <string>
#include <iostream>
using namespace std;

#include <process.h> // Use standard library to create / destroy threads
#include <windows.h> // Use windows functions for other thread control
// Important note: Windows also defines functions to create and destroy threads (CreateThread,
// ExitThread). However, you should use the process.h versions (_beginthreadex & _endthreadex)
// if you are using the standard C libraries such as stdlib.h above. Other windows threading
// functions are OK though


/////////////////////////
// Types / Data

// Initialisation data to create our thread. We can choose any kind of data for initialisation
// (including none). The data passed via a void* into the thread, so casting is required
struct SThreadData
{
	string message; // Message to display at start of game
	char   letter;  // Letter that needs to be guessed
};

// Keep track of number of guesses - global variable accessible by both threads
int NumGuesses;


/////////////////////////
// Thread code

// Entry point for the thread created below - consider it the main function of the thread. The function name here 
// is passed to the _beginthreadex function. The parameter is a void* (can point at anything) it is used to point
// at any initialisation data required - in this case we want to pass a SThreadData struct.
unsigned int __stdcall ThreadMain( void* pData )
{
	// The initialisation data is passed in as a void* for complete flexibility - now cast to correct type
	SThreadData *pThreadData = reinterpret_cast<SThreadData*>( pData );

	// Output message
	cout << pThreadData->message << endl;

	// Keep inputing keys until correct letter is pressed
	NumGuesses = 0;
	char c;
	do
	{
		c = _getch();
		++NumGuesses; // Increment global variable - accessible from main process

		

	} while (c != pThreadData->letter);

	// Output result
	cout << "Guessed '" << pThreadData->letter << "' in " << NumGuesses << " tries" << endl;

	return 0;
}


/////////////////////////
// Main process code

// Usual main function
int main()
{
	// Randomise the random number generator - use system time as seed
	srand( timeGetTime() ); // Note: timeGetTime needs winmm.lib

	// Allocate data to pass to the thread
	SThreadData* pThreadData = new SThreadData;
	pThreadData->message = "Guess the correct letter...";
	pThreadData->letter = 'a' + rand() % 26; // Select random letter

	// Create a new thread, starting at the function ThreadMain
	// Using process.h function rather than Windows function - see note at top
	HANDLE hThread = // Handle of the thread, used to interact with it
		reinterpret_cast<HANDLE>(_beginthreadex( nullptr,        // Default security attributes
		                                         0,           // Default stack size - OK unless using deep recursion
		                                         // or large local variables, both use stack memory
		                                         ThreadMain/********MISSING - not 0*/,  // Thread entry function 
		                                         reinterpret_cast<void*> (pThreadData)		/********MISSING - not 0*/, // Data to initialise thread
		                                         0,           // Initial thread state (0 = running)
		                                         nullptr));      // Can optionally return a thread UID, not needed here
	if (hThread == nullptr)
	{
		return -1; // Failure creating thread
	}

	// Wait until the thread has finished. The second parameter is a timeout - the maximum time
	// that will be waited. Wait functions can be used for synchronisation objects too, such as
	// mutexes, semaphores, timers, etc. There are also functions to wait for multiple objects
	//WaitForSingleObject( hThread, INFINITE /********MISSING - not 0, want to wait forever if necessary*/ );
	
	while (WaitForSingleObject(hThread, 2000) == WAIT_TIMEOUT)
	{
		cout << "I haven't all day day, guess it now!" << endl
			<< "You guessed:" << NumGuesses << " times" << endl;
	}

	// Close the thread handle
	CloseHandle( hThread );

	// Free the initialisation data - this needs to be done *after* the thread has used the
	// data. Do this too early and the data will be deleted before it is used
	delete pThreadData;

	cout << "Press Spacebar to Finish";
	while (_getch() != ' ' );
	return 0;
}
