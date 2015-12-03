// Main function for a Windows form application. The same functionality that has
// been in WinMain in all our other programs. 
//
// This is a .NET program. There are some C++ extensions used here, mostly to do
// with safe memory management (the ^ and % syntax, gcnew). The .NET libraries
// are also used to access to all the .NET functions (the using namespace entries).
//
// How much you need to use these features depends on how much code you write on
// the form side of the application. As an aside, the .NET libraries mirror the
// libraries that are available in C#, that you might remember from the 1st year

#include <windows.h>
#include "ParticleForm.h"

using namespace System;
using namespace System::Windows::Forms;


/****| INFO |***********************************************************/
// Ugly externs to get access to D3D / graphics app functions.
// This allows us to use the DirectX code with minimal code changes. A
// better system would use OO, perhaps use a GraphicsManager or similar
/***********************************************************************/
extern bool D3DSetup(HWND hWnd);
extern void D3DShutdown();
extern bool SceneSetup();
extern void SceneShutdown();
extern void RenderScene();
extern void UpdateScene();
extern void ResetParticles();


// Declaration of idle event, which will handle the render/update loop (see below)
void OnApplicationIdle(Object^ sender, EventArgs^ e);


//**** Main function
[STAThread]
void Main(array<String^>^ args)
{
	// Enabling Windows visual effects before any controls are created
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	// Create the windows form
	ParticleTool::ParticleForm form;

	/****| INFO |********************************************************************/
	// We mirror the initialisation / shutdown that was in the WinMain function
	// in the original code (still visible at the end of DX10Particles.cpp)
	/********************************************************************************/
	if (D3DSetup((HWND)(form.renderPanel->Handle.ToPointer()))) // Getting the handle of the form element we wish to render onto
	{
		if (SceneSetup())
		{
			// Add an idle event handler - this calls the OnApplicationIdle function whenever the form is idle
			Application::Idle += gcnew EventHandler(OnApplicationIdle); // gcnew is the .NET version of new. Automatically deleted

			// Execute the form, will stay in this function until we exit the form
			Application::Run(%form);

			SceneShutdown();
		}
		D3DShutdown();
	}
}


/****| INFO |*******************************************************************/
// This function is called whenever the form is idle (most of the time). It
// simply renders and updates (the panel) until a message arrives for the 
// window, which may be user input, window move, system shutdown etc.
// At that point it leaves the function, Windows will process the message, and
// will return here when done
/*******************************************************************************/
void OnApplicationIdle(Object^ sender, EventArgs^ e)
{
	MSG msg;
	while (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		RenderScene();
		UpdateScene();
	}
}

