//Team Project Smith, Kreiser, Graham
//August 22, 2018

#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <vector>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_PORT "4444"

void processCommand(std::vector<char> buffer, std::vector<char>& outputBuffer);
void setDriveLetter(std::string directory);
void listFiles(std::vector<char>& outputBuffer, std::string directory);
void printSystem(std::vector<char>& outputBuffer);

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult;

	//Sets the socket needed for the connection
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ServerSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		std::cout << "WSAStartup failed with error: " << iResult << std::endl;
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);
	// Accepts a listener socket
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// Accept a client socket
	ServerSocket = accept(ListenSocket, NULL, NULL);
	if (ServerSocket == INVALID_SOCKET)
	{
		std::cout << "accept failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// No longer need listening socket
	closesocket(ListenSocket);
	// Receive until the peer shuts down the connection
	do {
		std::vector<char> inputBuffer(5000);
		std::vector<char> outputBuffer;
		//Receives any incoming buffers from the server
		iResult = recv(ServerSocket, inputBuffer.data(), inputBuffer.size(), 0);
		std::cout << "Data has been received." << std::endl;
		//If the result is not -1, then resize inputBuffer to make sure that it can hold the bytes incoming
		if (iResult != -1)
		{
			inputBuffer.resize(iResult);
		}
		//If the result is not 0, information was pulled and can be processed
		if (iResult != 0)
		{
			//Central function that handles switch/cases for all the commands
			processCommand(inputBuffer, outputBuffer);
			std::cout << std::endl;
			//If the command asks for a result (at this stage, all will), the result needs to be sent back 
			//to the server
			iResult = send(ServerSocket, outputBuffer.data(), outputBuffer.size(), 0);
		}
		//If 0 bytes has been sent, then the server has shutdown and so should the client
		else
		{
			break;
		}
	} while (true);

	// shutdown the connection since we're done
	iResult = shutdown(ServerSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ServerSocket);
	WSACleanup();
	return 0;
}
//Central function that handles switch/cases for all the commands
void processCommand(std::vector<char> buffer, std::vector<char>& outputBuffer)
{
	std::string directory = "C:\\*";
	std::string command = buffer.data();
	setDriveLetter(directory);
	switch (buffer[0])
	{
	case 'd':
		//Calls the function to list files based on a directory provided
		//C:\ is used as a default
		//The outputBuffer has all the files and directory information listed
		listFiles(outputBuffer, directory);
		break;
	}
}
//Function to build a basic file structure outline of the client machine
void listFiles(std::vector<char>& outputBuffer, std::string directory)
{
	WIN32_FIND_DATA fileData;
	__int64 filesize = 0;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	//Locates the file information for the specificed directory
	//Default will be the C:\ Drive
	hFind = FindFirstFile(directory.c_str(), &fileData);
	do
	{
		//First section handles directories
		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//String used to build the line showing the directory information
			std::string build;
			build = build + "  " + fileData.cFileName + "   <DIR>\n";
			for (auto& element : build)
			{
				//All characters are added into the buffer vector
				outputBuffer.push_back(element);
			}
		}
		//Next section handles files
		else
		{
			//Type to hold file size, it is recorded in bytes
			LARGE_INTEGER data;
			data.LowPart = fileData.nFileSizeLow;
			data.HighPart = fileData.nFileSizeHigh;
			filesize = data.QuadPart;
			//String used to build the line showing the file information
			std::string build;
			build = build + "  " + fileData.cFileName + "   ";
			//Converts the file size to a type that can be printed
			build += static_cast<std::ostringstream*>(&(std::ostringstream() << filesize))->str();
			build += " bytes\n";
			for (auto& element : build)
			{
				//All characters are added into the buffer vector
				outputBuffer.push_back(element);
			}
		}
		//Once there are no files to output, return the function
	} while (FindNextFile(hFind, &fileData) != 0);
}
void setDriveLetter(std::string directory)
{
	std::string tempDrive;	//temp choice for drive
	char tempLetter = 'c';

	std::cout << "Current drive is %s." << directory << std::endl; //Displays current drive 
	std::cout << "Would you like to change drives? ";	//Prompts user for change if desired
	std::getline(std::cin, tempDrive);//Takes in user's choice 
	//If users choice is any of the below options, user chooses new letter
	if (tempDrive == "y" || tempDrive == "Y" || tempDrive == "yes" || tempDrive == "Yes" || tempDrive == "YES")
	{
		std::cout << "Enter the new drive letter: ";	//Accepts new drive letter
		std::cin >> tempLetter;
		std::cin.clear();
		//Error checking for new drive letter
		if (isalpha((int)tempLetter) == false) //checks tempLetter to see if a letter
		{
			std::cout << "You entered invalid characters!" << std::endl;
			setDriveLetter(directory);//invalid character entry recalls setDriveLetter function
		}
	}
	else
	{
		directory = ("%c:\\*", toupper((int)tempLetter));//User indicated current drive letter is good.
	}
}
void printSystem(std::vector<char>& outputBuffer)
{

}