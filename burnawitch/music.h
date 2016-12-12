#pragma once
//*******************************************************
//
//		Simple sound and music library
//		
//		Usage:	start_music(L"folder\\manowar.mp3");
//		More than one calls possible 
//
//
//********************************************************



#include <dshow.h>
#include <cstdio>
#pragma comment(lib, "strmiids.lib") 
int run_mp3_thread(LPWSTR file);
void start_music(LPWSTR file);
DWORD WINAPI runThread(LPVOID args)
{
	LPWSTR ccall = reinterpret_cast<LPWSTR>(args);
	run_mp3_thread(ccall);
	delete[] ccall;
	return 0;
}
void start_music(LPWSTR file)
{
	DWORD threadId;
	CreateThread(NULL, 0, runThread, file, 0, &threadId);
}

int run_mp3_thread(LPWSTR file)
{
	IGraphBuilder *pGraph = NULL;
	IMediaControl *pControl = NULL;
	IMediaEvent   *pEvent = NULL;

	// Initialize the COM library.
	HRESULT hr = ::CoInitialize(NULL);
	if (FAILED(hr))
	{
		::printf("ERROR - Could not initialize COM library");
		return 0;
	}

	// Create the filter graph manager and query for interfaces.
	hr = ::CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder, (void **)&pGraph);
	if (FAILED(hr))
	{
		::printf("ERROR - Could not create the Filter Graph Manager.");
		return 0;
	}

	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
	hr = pGraph->QueryInterface(IID_IMediaEvent, (void **)&pEvent);

	// Build the graph.
	hr = pGraph->RenderFile(file, NULL);
	if (SUCCEEDED(hr))
	{
		// Run the graph.
		hr = pControl->Run();
		if (SUCCEEDED(hr))
		{
			// Wait for completion.
			long evCode;
			pEvent->WaitForCompletion(200000, &evCode);

			// Note: Do not use INFINITE in a real application, because it
			// can block indefinitely.
		}
	}
	// Clean up in reverse order.
	pEvent->Release();
	pControl->Release();
	pGraph->Release();
	::CoUninitialize();
}

