/*******************************************
	SynchroniseThreads.cpp

	Basic Windows threading techniques
	Demonstrating issues with synchronising
	data between threads
********************************************/

#include <stdlib.h>
#include <iostream>
#include <sstream>
using namespace std;

#include <process.h> // Use standard library to create / destroy threads
#include <windows.h> // Use windows functions for other thread control
// Important note: Windows also defines functions to create and destroy threads (CreateThread,
// ExitThread). However, you should use the process.h versions (_beginthreadex & _endthreadex)
// if you are using the standard C libraries such as stdlib.h above. Other windows threading
// functions are OK though


/////////////////////////
// Data

// Bank balance and amount withdrawn - data will be accessed by multiple threads
int Balance;
int Withdrawn;

CRITICAL_SECTION critical_section;  // shared structure

/////////////////////////
// Thread code

// Entry point for the threads created below. No data passed through parameter (NULL)
unsigned int __stdcall WithdrawCash( void* pData )
{
	EnterCriticalSection(&critical_section);

		// Keep withdrawing money until it runs out, keep track of total withdrawn
	while (Balance >= 10)
	{
		stringstream outText; // Creating text in temp string so threads don't interupt...
		outText << "Balance: " << Balance << ", withdrawing $10\n"; // ...each others output
		cout << outText.str();
		Withdrawn += 10;
		Balance -= 10;
	}
	LeaveCriticalSection(&critical_section);
	return 0;
}


/////////////////////////
// Main process code

// Usual main function
int main()
{
	InitializeCriticalSection(&critical_section);


	// Initialise cash balance
	Balance = 250;
	cout << "Initial balance: $" << Balance << endl;

	// Will use multiple threads to withdraw the cash
	const int NumThreads = 8;
	HANDLE hThreads[NumThreads]; // Thread handles
	cout << "Withdrawing all money with " << NumThreads << " threads" << endl;

	// Create multiple WithdrawCash threads
	for (int i = 0; i < NumThreads; ++i)
	{
		hThreads[i] =
			(HANDLE)_beginthreadex(nullptr,         // Default security attributes
		                            0,            // Default stack size
		                            WithdrawCash, // Thread entry function 
		                            nullptr,         // No data needed to initialise thread
		                            0,            // Initial thread state (0 = running)
		                            nullptr );       // Returned thread UID, not needed here
		if (!hThreads[i]) 
		{
			return -1; // Failure creating thread
		}
	}

	// Wait until all threads have finished
	WaitForMultipleObjects( NumThreads, // Number of objects (threads here) being waited for
	                        hThreads,   // Pointer to array of object handles
							TRUE,       // We are waiting for all objects (rather than just one)
							INFINITE ); // Wait forever if necessary

	// Output result of the multiple threaded withdrawals
	cout << "Withdrew $" << Withdrawn << endl;

	// Close the thread handles
	for (int i = 0; i < NumThreads; ++i)
	{
		CloseHandle( hThreads[i] );
	}

	system( "pause" );
	return 0;
}
