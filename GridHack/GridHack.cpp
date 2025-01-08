
#include "stdafx.h"
#include <windows.h>
#include <winuser.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <tlhelp32.h>
#include <stdio.h>
#include <math.h>

using namespace std;
using namespace std::chrono;

DWORD GetModuleBase(const wchar_t * ModuleName, DWORD ProcessId) {
	MODULEENTRY32 ModuleEntry = { 0 };
	HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessId);

	if (!SnapShot)
		return NULL;

	ModuleEntry.dwSize = sizeof(ModuleEntry);

	if (!Module32First(SnapShot, &ModuleEntry))
		return NULL;

	do {
		if (!wcscmp(ModuleEntry.szModule, ModuleName)) {
			CloseHandle(SnapShot);
			return (DWORD)ModuleEntry.modBaseAddr;
		}
	} while (Module32Next(SnapShot, &ModuleEntry));

	CloseHandle(SnapShot);
	return NULL;
}
HWND hwnd = NULL;
DWORD pid;
HANDLE phandle;
DWORD BaseAddress;

DWORD LTBaseAddress;
DWORD PTBaseAddress;
DWORD weightBaseAddress;

DWORD flashbackBaseAddress;
DWORD flashbackOffsets[5] = {0xFC,0x9C,0x14,0x2C,0x2C};
DWORD flashbackBuffer = 0;
int flashbacks;

DWORD offtrackBaseAddress;
DWORD offtrackOffsets[3] = { 0x750,0x3C,0xF0};
DWORD offtrackBuffer = 0;
bool offtrack;


DWORD LToffset = 0x8C8;
DWORD LTBuffer = 0;
float LTvalue = 0; 


DWORD PToffset = 0xA78;
DWORD PTBuffer = 0;
float PTvalue = 0;


DWORD weightOffset = 0x11E0;
DWORD weightBuffer = 0;
float weight = 0;
float temp_weight = 0;

bool carFound = 0;
short wheelsFound = 0;

bool weight_changed_by_user = false;

float lmul;
float pmul;
float incrcap = 8.0f;
float speedcap = 1200.0f;
float speedmin = 1.15f;
float desiredfps = 30.0f;
float desiredft = 1000.0f / desiredfps;
float info_ft = 100.0f;
float lspeed;
float pspeed;
float np = 0;
float maxSpeedIncrement = 36.0f;
float frametime = 1.0f;
float last_mass = 0;
int temp;
float last_mass_helper;
bool rshiftaction = 0;
bool KEY_VK_RSHIFT_PRESSED_BEFORE = false;
bool KEY_VK_F_PRESSED_BEFORE = false;
bool KEY_VK_C_PRESSED_BEFORE = false;
bool KEY_VK_R_PRESSED_BEFORE = false;
bool KEY_VK_V_PRESSED_BEFORE = false;
float base_mass = 0;
float saved_mass = 0;
float massratio;
POINT p;
float horizontalPositive = 0;
float verticalPositive = 0;

float mouseVertOffsetPerc = 0;
float mouseHoriOffsetPerc = 0;


void restoreFlashbacks(HANDLE phandle)
{
	flashbackBuffer = 0;

	if (flashbackBuffer == 0)
	{
		ReadProcessMemory(phandle, (void*)flashbackBaseAddress, &flashbackBuffer, 4, NULL);
		flashbackBuffer += flashbackOffsets[0];
		ReadProcessMemory(phandle, (void*)flashbackBuffer, &flashbackBuffer, 4, NULL);
		flashbackBuffer += flashbackOffsets[1];
		ReadProcessMemory(phandle, (void*)flashbackBuffer, &flashbackBuffer, 4, NULL);
		flashbackBuffer += flashbackOffsets[2];
		ReadProcessMemory(phandle, (void*)flashbackBuffer, &flashbackBuffer, 4, NULL);
		flashbackBuffer += flashbackOffsets[3];
		ReadProcessMemory(phandle, (void*)flashbackBuffer, &flashbackBuffer, 4, NULL);
		flashbackBuffer += flashbackOffsets[4];
		
	}
	ReadProcessMemory(phandle, (void*)flashbackBuffer, &flashbacks, 4, NULL);
	
	if (flashbacks == 1 || flashbacks == 0)
	{
		flashbacks = 420;
		WriteProcessMemory(phandle, (void*)flashbackBuffer, &flashbacks, sizeof(flashbacks), 0);
	}
	
}

void unOfftrack(HANDLE phandle)
{

	offtrackBuffer = 0;
	
	ReadProcessMemory(phandle, (void*)offtrackBaseAddress, &offtrackBuffer, 4, NULL);
	offtrackBuffer += offtrackOffsets[0];
	ReadProcessMemory(phandle, (void*)offtrackBuffer, &offtrackBuffer, 4, NULL);
	offtrackBuffer += offtrackOffsets[1];
	ReadProcessMemory(phandle, (void*)offtrackBuffer, &offtrackBuffer, 4, NULL);
	offtrackBuffer += offtrackOffsets[2];


	ReadProcessMemory(phandle, (void*)offtrackBuffer, &offtrack, 4, NULL);
	
	if (offtrack == 1)
	{
		offtrack = 0;
		WriteProcessMemory(phandle, (void*)offtrackBuffer, &offtrack, sizeof(offtrack), 0);
	}

}

void writeWheenRotation(HANDLE phandle, float rotation)
{
	WriteProcessMemory(phandle, (void*)PTBuffer, &rotation, sizeof(rotation), 0);
	WriteProcessMemory(phandle, (void*)LTBuffer, &rotation, sizeof(rotation), 0);
}


void writeLeftWheenRotation(HANDLE phandle, float rotation)
{
	WriteProcessMemory(phandle, (void*)LTBuffer, &rotation, sizeof(rotation), 0);
}

void writeRightWheenRotation(HANDLE phandle, float rotation)
{
	WriteProcessMemory(phandle, (void*)PTBuffer, &rotation, sizeof(rotation), 0);
}

void readWheelRotation(HANDLE phandle)
{

	LTBuffer = 0;

	ReadProcessMemory(phandle, (void*)LTBaseAddress, &LTBuffer, 4, NULL);
	LTBuffer += LToffset;

	PTBuffer = 0;
	
	ReadProcessMemory(phandle, (void*)PTBaseAddress, &PTBuffer, 4, NULL);
	PTBuffer += PToffset;
	

	ReadProcessMemory(phandle, (void*)PTBuffer, &PTvalue, sizeof(PTvalue), 0);
	ReadProcessMemory(phandle, (void*)LTBuffer, &LTvalue, sizeof(LTvalue), 0);

	/*
	if (abs(LTvalue - PTvalue) > 20 || (LTvalue == 0 && PTvalue != 0) || (PTvalue == 0 && LTvalue != 0))
	{
		LTBuffer = 0;
		PTBuffer = 0;
	}*/
}

void resetBuffers()
{
	weightBuffer = 0;
	PTBuffer = 0;
	LTBuffer = 0;
	flashbackBuffer = 0;
	offtrackBuffer = 0;
}

void readWeight(HANDLE phandle)
{

	weightBuffer = 0;

	ReadProcessMemory(phandle, (void*)weightBaseAddress, &weightBuffer, 4, NULL);
	weightBuffer += weightOffset;

	ReadProcessMemory(phandle, (void*)weightBuffer, &weight, sizeof(weight), 0);		
	//return;

	
}

void setBaseWeight(HANDLE phandle)
{
	if (!weight_changed_by_user && weight != base_mass && last_mass >= 1 && base_mass != last_mass)
	{
		base_mass = last_mass;
		cout << "BASE MASS SET TO: " << last_mass << endl;
	}
}

void increaseWeight(HANDLE phandle, float w)
{
	
	temp_weight = weight + w;
	if (temp_weight <= 0)
	{
		temp_weight = 1;
	}
	WriteProcessMemory(phandle, (void*)weightBuffer, &temp_weight, sizeof(temp_weight), 0);
}

void setWeight(HANDLE phandle, float w)
{
	weightBuffer = 0;
	ReadProcessMemory(phandle, (void*)weightBaseAddress, &weightBuffer, 4, NULL);
	weightBuffer += weightOffset;

	WriteProcessMemory(phandle, (void*)weightBuffer, &w, sizeof(w), 0);
	
}

float getLowerWheelSpeed()
{
	if (lspeed > pspeed) return pspeed;
	else return lspeed;
}

void stabilizeWheelSpeed(float &lspeed, float &pspeed, float threshold)
{
	if (abs(lspeed - pspeed) > threshold)
	{
		if (lspeed > pspeed) lspeed = pspeed;
		else pspeed = lspeed;
	}
	
}

void readMouseOffsets()
{
	mouseHoriOffsetPerc = 1+((horizontalPositive - p.x) / horizontalPositive);
	mouseVertOffsetPerc = 1+((verticalPositive - p.y) / verticalPositive);
}

void help()
{
	cout << "Q/E - Increase/Decrease mass\nR - Reset mass\nShift - Basic boost\nLCtrl - Linear boost\nRCtrl - Prev saved mass\nEnter - Rocket\nTAB - Settings refresh" << endl;
}

void init()
{
	//resetBuffers();
	hwnd = FindWindowA(NULL, "GRID 2");
	GetWindowThreadProcessId(hwnd, &pid);
	while (!hwnd)
	{
		hwnd = FindWindowA(NULL, "GRID 2");
		GetWindowThreadProcessId(hwnd, &pid);
		resetBuffers();
		cout << "Waiting for GRID 2...";
		Sleep(5000);
		system("CLS");
	}
	


	phandle = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
	BaseAddress = GetModuleBase(L"grid2_avx.exe", pid);

	LTBaseAddress = BaseAddress + 0x00E723E0;
	PTBaseAddress = BaseAddress + 0x00E72A68;
	weightBaseAddress = BaseAddress + 0x00E72B48;
	flashbackBaseAddress = BaseAddress + 0x00DE0220;
	offtrackBaseAddress = BaseAddress + 0x00E009A0;

	readWheelRotation(phandle);
	readWeight(phandle);
	base_mass = weight;
}


void mouseNitro()
{

	lspeed = LTvalue*mouseVertOffsetPerc;
	pspeed = PTvalue *mouseVertOffsetPerc;


	if (lspeed - LTvalue > maxSpeedIncrement) lspeed = LTvalue + maxSpeedIncrement;
	if (pspeed - PTvalue > maxSpeedIncrement) pspeed = PTvalue + maxSpeedIncrement;

	if (lspeed > speedcap) lspeed = speedcap;
	if (lspeed > speedcap) pspeed = speedcap;

	stabilizeWheelSpeed(lspeed, pspeed, 10.0f);

	writeLeftWheenRotation(phandle, lspeed);
	writeRightWheenRotation(phandle, pspeed);

}

void GetDesktopResolution(float& horizontalPositive, float& verticalPositive)
{
	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);
	horizontalPositive = (desktop.right)/1.2f;
	verticalPositive = (desktop.bottom)/1.2f;
}

int main()
{
	
	GetDesktopResolution(horizontalPositive, verticalPositive);
	
	init();



	chrono::steady_clock::time_point last_key_read;
	chrono::steady_clock::time_point last_info_print;


	while (1)
	{
		auto start = chrono::high_resolution_clock::now();
		

		unOfftrack(phandle);
		restoreFlashbacks(phandle);
		

		if (GetAsyncKeyState(VK_TAB) < 0) // Spacja - stop
		{
			system("CLS");
			init();
			
		}

		
		
		
		
		chrono::duration<float, std::milli> time_since_last_key_read = chrono::high_resolution_clock::now() - last_key_read;
		chrono::duration<float, std::milli> time_since_last_info_print = chrono::high_resolution_clock::now() - last_info_print;

		

		

		if (time_since_last_key_read.count() >= desiredft-1)
		{
			
			//GetCursorPos(&p);
			//readMouseOffsets();
			weight_changed_by_user = false;

			readWeight(phandle);
			readWheelRotation(phandle);

			if (weight == 0) setWeight(phandle, 1);
		
			massratio = weight / base_mass;
			if (massratio < 2) massratio = 1;

			last_key_read = chrono::high_resolution_clock::now();
			
			if (GetAsyncKeyState(VK_LSHIFT) < 0 || GetAsyncKeyState(VK_LBUTTON) < 0) // BOOST
			{
				
				//lmul = (36 / ((LTvalue + 1)) + 1)*massratio;
				//pmul = (36 / ((PTvalue + 1)) + 1)*massratio;
				lmul = weight*0.5;//(36 / ((LTvalue + 1)) + 1)*massratio;
				pmul = weight*0.5;//(36 / ((PTvalue + 1)) + 1)*massratio;


				if (lmul < speedmin) lmul = speedmin;
				if (pmul < speedmin) pmul = speedmin;
				lspeed = LTvalue + weight*0.1 *massratio;
				pspeed = PTvalue + weight*0.1 *massratio;

				cout << "\n\n [+ " << weight*0.1 << endl << endl;

				if (lspeed - LTvalue > maxSpeedIncrement) lspeed = LTvalue + maxSpeedIncrement;
				if (pspeed - PTvalue > maxSpeedIncrement) pspeed = PTvalue + maxSpeedIncrement;

				if (lspeed > speedcap) lspeed = speedcap;
				if (lspeed > speedcap) pspeed = speedcap;

				stabilizeWheelSpeed(lspeed, pspeed, 10.0f);



				np = (lmul + pmul) / 2;
				writeLeftWheenRotation(phandle, lspeed);
				writeRightWheenRotation(phandle, pspeed);

			}


			if (GetAsyncKeyState(VK_RETURN) < 0 || GetAsyncKeyState(VK_RBUTTON) < 0) //  ULTRA BOOST
			{
				

				lmul = (15 / ((LTvalue + 1)) + 1)*(15 / ((LTvalue + 1)) + 1)*massratio;
				pmul = (15 / ((PTvalue + 1)) + 1)*(15 / ((PTvalue + 1)) + 1)*massratio;


				lspeed = (LTvalue + 1)*lmul;
				pspeed = (PTvalue + 1)*pmul;

				if (lspeed < 0) lspeed = lspeed*-1;
				if (pspeed < 0) pspeed = pspeed*-1;


				if (lspeed > speedcap) lspeed = speedcap;
				if (pspeed > speedcap) pspeed = speedcap;

				if (abs(lspeed - LTvalue) < 100) lspeed = LTvalue + 100;
				if (abs(pspeed - PTvalue) < 100) pspeed = PTvalue + 100;
				stabilizeWheelSpeed(lspeed, pspeed, 0.5f);

				np = (lmul + pmul) / 2;
				writeLeftWheenRotation(phandle, lspeed);
				writeRightWheenRotation(phandle, pspeed);

			}

			if (GetAsyncKeyState(VK_RSHIFT) < 0) // RCTRL - LINEAR +70 *MASA
			{

				lspeed = LTvalue + 70 * massratio;
				pspeed = PTvalue + 70 * massratio;



				if (lspeed > speedcap) lspeed = speedcap;
				if (pspeed > speedcap) pspeed = speedcap;

				stabilizeWheelSpeed(lspeed, pspeed, 0.5f);

				np = (lmul + pmul) / 2;
				writeLeftWheenRotation(phandle, lspeed);
				writeRightWheenRotation(phandle, pspeed);

			}

			if (GetAsyncKeyState(VK_SPACE) < 0) writeWheenRotation(phandle, -1); // Space - stop

			
			if (GetAsyncKeyState(0x45) < 0 || GetAsyncKeyState(VK_OEM_7) < 0) // Q - decrease weight
			{
				weight_changed_by_user = true;
				increaseWeight(phandle, (float)150);
			}
			if (GetAsyncKeyState(0x51) < 0 || GetAsyncKeyState(VK_OEM_1) < 0) // E - increase weight
			{
				weight_changed_by_user = true;
				increaseWeight(phandle, (float)-150);
			}



			if ((GetAsyncKeyState(0x46) < 0 || GetAsyncKeyState(VK_OEM_2) < 0) && !KEY_VK_F_PRESSED_BEFORE) // F - increase weight af
			{
				KEY_VK_F_PRESSED_BEFORE = true;
				weight_changed_by_user = true;
				increaseWeight(phandle, (float)15000);
			}else if (!(GetAsyncKeyState(0x46) < 0)) KEY_VK_F_PRESSED_BEFORE = false;
	
				
			if ((GetAsyncKeyState(0x43) < 0 || GetAsyncKeyState(VK_OEM_PERIOD) < 0) && !KEY_VK_C_PRESSED_BEFORE)// C - decrease weight af
			{
				KEY_VK_C_PRESSED_BEFORE = true; 
				weight_changed_by_user = true;
				increaseWeight(phandle, (float)-15000);
			}else if (!(GetAsyncKeyState(0x43) < 0)) KEY_VK_C_PRESSED_BEFORE = false;

			
			if (GetAsyncKeyState(0x56) < 0 && !KEY_VK_V_PRESSED_BEFORE)
			{
				KEY_VK_V_PRESSED_BEFORE = true;
				weight_changed_by_user = true;
				setWeight(phandle, weight*-1);
			}else if (!(GetAsyncKeyState(0x56) < 0)) KEY_VK_V_PRESSED_BEFORE = false;

		
			if (GetAsyncKeyState(0x52) < 0 && !KEY_VK_R_PRESSED_BEFORE) // R - reset masy
			{
				KEY_VK_R_PRESSED_BEFORE = true;
				weight_changed_by_user = true;
				
				if (weight != base_mass)
				{
					saved_mass = weight;
					cout << "USTAWILEM SAVED MASS NA " << saved_mass;
					setWeight(phandle, base_mass);
				}
				else if (saved_mass) setWeight(phandle, saved_mass);

				
			}else if (!(GetAsyncKeyState(0x52) < 0)) KEY_VK_R_PRESSED_BEFORE = false;



			if (!weight_changed_by_user) last_mass = weight;
			
		}



		auto end = std::chrono::high_resolution_clock::now();
		chrono::duration<float, std::milli> elapsed = end - start;
	
		if (elapsed.count() < desiredft) this_thread::sleep_for(chrono::duration<double, std::ratio<1, 1000>>(2.5f-elapsed.count()));
		
		
		end = std::chrono::high_resolution_clock::now();
		elapsed = end - start;
		frametime = elapsed.count();
		

		if (time_since_last_info_print.count() >= info_ft)
		{

			last_info_print = chrono::high_resolution_clock::now();
			cout << "\n\n\n         [L-SPD] " << LTvalue << endl;
			cout << "         [P-SPD] " << PTvalue << endl;
			cout << "          [MASA] " << weight << "kg" << endl;
			cout << "    [FLASHBANGI] " << flashbacks << endl;
			cout << "    [Frame Time] " << frametime << endl;
			cout << "[Key Frame Time] " << time_since_last_key_read.count() << endl;
			cout << "           [FPS] " << 1000 / frametime << endl;
			cout << "          [MYSZ] " << p.x << ", " << p.y << endl;
			cout << "   [MYSZ OFFSET] " << mouseHoriOffsetPerc << ", " << mouseVertOffsetPerc << endl;
		}
	}


    return 0;
}
 
