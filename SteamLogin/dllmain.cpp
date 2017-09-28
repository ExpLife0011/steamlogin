// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"
#include <string>
#include <process.h>
#include <vector>
#include "utils.h"
#include <algorithm>
void login(DWORD ecx , DWORD call);
unsigned __stdcall ThreadFunc(void* pArguments);

std::vector<DWORD> g_buttonList;

void travseButton(DWORD addr);
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: {
		DisableThreadLibraryCalls(hModule);
		_beginthreadex(NULL, NULL, &ThreadFunc, NULL, NULL, NULL);
		break;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

void login(DWORD ecxObj, DWORD callAddr)
{
	__try {
		__asm {
			MOV ECX, ecxObj;
			LEA ESI, DWORD PTR DS : [ECX - 0xC8];
			LEA ECX, DWORD PTR DS : [ESI - 0x114];
			MOV EAX, callAddr;
			CALL EAX;
		}
	}
	__except (1) {

	}

}

unsigned __stdcall ThreadFunc(void * pArguments)
{
	//��ִ��ģ��, ��Ŀ 115
	auto vgui2_s = utils::GetInstance()->GetModuleInformationEx("vgui2_s.DLL");
	if ((DWORD)vgui2_s.lpBaseOfDll < 1) {
		return 1;
	}

#ifdef _USER_DBG
	utils::GetInstance()->log("HXL: vgui2_s.lpBaseOfDll = %x ", (DWORD)vgui2_s.lpBaseOfDll);
#endif // _USER_DBG

	//��ȡ������ƫ��
	DWORD Base_ButtonTravseAddr = (DWORD)vgui2_s.lpBaseOfDll + 0x1000 + 0x1139EC;
	auto Base = utils::GetInstance()->read<DWORD>(Base_ButtonTravseAddr);
	if (!Base) {
#ifdef _USER_DBG
		utils::GetInstance()->log("HXL: û���ҵ�Base_ButtonTravseAddr ��");
#endif 
		return 1;
	}

	auto Base2 = utils::GetInstance()->read<DWORD>(Base + 0x38);
	if (!Base2) {
#ifdef _USER_DBG
		utils::GetInstance()->log("HXL: û���ҵ�Base2��");
#endif 
		return 1;
	}
	//Base2 ���� ����Ļ���ַ
	travseButton(Base2);

	//�жϿؼ��б��Ƿ�Ϊ��
	auto usernameItr = std::find_if(g_buttonList.begin(), g_buttonList.end(), 
		[](DWORD obj) {
		auto temp = utils::GetInstance()->read<DWORD>(obj - 0x70);
		if (!temp) {
			return false;
		}
		std::string tempName = (char*)(temp);
		if (0 == tempName.compare("UserNameEdit")) {
			return true;
		}
		return false;
	});

	if (g_buttonList.end() == usernameItr) {
#ifdef _USER_DBG
		utils::GetInstance()->log("HXL: cant find usernameedit!");
#endif // _USER_DBG
		return 1;
	}

	auto passwordPtr = std::find_if(g_buttonList.begin(), 
		g_buttonList.end(),
		[](DWORD obj) {
		auto temp = utils::GetInstance()->read<DWORD>(obj - 0x70);
		if (!temp) {
			return false;
		}
		std::string password = (char*)(temp);
		if (0 == password.compare("PasswordEdit")) {
			return true;
		}
		return false;
	});

	if (g_buttonList.end() == passwordPtr) {
#ifdef _USER_DBG
		utils::GetInstance()->log("HXL: cant find passwordedit!");
#endif // _USER_DBG
		return 1;
	}

	//����˺���������
	std::string usernameString("");
	std::string passwordString("");

	DWORD *pUserNameContext = new DWORD[usernameString.size()];
	for (auto i = 0; i < usernameString.size(); i++) {
		pUserNameContext[i] = usernameString.at(i);
	}

	*(DWORD*)((*usernameItr)+ 0x18) = (DWORD)pUserNameContext;
	*(DWORD*)((*usernameItr) + 0x1C) = 0x40;
	*(DWORD*)((*usernameItr) + 0x24) = usernameString.size();


	DWORD *pPasswordContext = new DWORD[passwordString.size()];
	for (auto i = 0; i < passwordString.size(); i++) {
		pPasswordContext[i] = passwordString.at(i);
	}

	*(DWORD*)((*passwordPtr) + 0x18) = (DWORD)pPasswordContext;
	*(DWORD*)((*usernameItr) + 0x1C) = 0x40;
	*(DWORD*)((*passwordPtr) + 0x24) = passwordString.size();

	//���õ�¼CALL
	//$ + 58     >0A8F8748  ASCII "SteamLoginDialog"


	auto steamui = utils::GetInstance()->GetModuleInformationEx("steamui.dll");
	if ((DWORD)steamui.lpBaseOfDll < 1) {
		return 1;
	}

	//��ȡ��������CALL��ַ
	auto callAddr = (DWORD)steamui.lpBaseOfDll + 0x1000 + 0x18FAA0;

	auto pSteamLoginDialog = std::find_if(g_buttonList.begin(), g_buttonList.end(), [](DWORD obj) {
		auto temp = utils::GetInstance()->read<DWORD>(obj - 0x70);
		if (!temp) {
			return false;
		}
		std::string password = (char*)(temp);
		if (0 == password.compare("SteamLoginDialog")) {
			return true;
		}
		return false;
	});

	if (g_buttonList.end() == pSteamLoginDialog) {
		return 1;
	}

	utils::GetInstance()->log("HXL: username = %x , password = %x , steamlogindialog = %x CALL = %x", *usernameItr, *passwordPtr, *pSteamLoginDialog, callAddr);

	login(*pSteamLoginDialog, callAddr);
	return 0;
}

void travseButton(DWORD addr)
{
	auto StartAddr = utils::GetInstance()->read<DWORD>(addr + 0x4);
	auto ButtonNum = utils::GetInstance()->read<DWORD>(addr + 0x10);

	if (!StartAddr || !ButtonNum) {
		//���������ʼ��ַ || �ؼ�����������
		return;
	}

	for (DWORD i = 0; i < ButtonNum; i++){
		auto temp = utils::GetInstance()->read<DWORD>(StartAddr + i*4);
		if (temp){
			auto objTemp = utils::GetInstance()->read<DWORD>(temp + 0x20);
			if (objTemp) {
				//��ȡ��Ҫ������ַ
				auto objTemp2 = utils::GetInstance()->read<DWORD>(objTemp);
				if (objTemp2) {
					//MOV EDX, DWORD PTR DS : [EAX]
					//��ȡ�����+4��λ��
					auto objTemp4 = utils::GetInstance()->read<DWORD>(objTemp2+ 0x4);
					if (objTemp4) {
						//�ҵ����������ַ

						//58384275    2B49 FC         SUB ECX, DWORD PTR DS : [ECX - 4]
						//58384278    83E9 44         SUB ECX, 44
						// E9
						auto ecx_1 = objTemp4 + 0x5;
						//�� objTemp4 + 0x5 ��ʼ����һֱ������һ��E9λ��
						auto index = 0;
						do {
							auto data = utils::GetInstance()->read<BYTE>(ecx_1 + index);
							if (data == 0xe9) {
								break;
							}
							index++;
						} while (1);
						DWORD ecx_2 = 0;
						memcpy(&ecx_2, (void*)(ecx_1), index);

#ifdef _USER_DBG
						utils::GetInstance()->log("HXL: objTemp4 = %x ecx_1 = %x", objTemp4, ecx_2);
#endif // _USER_DBG
						auto ecx_sub_4 = utils::GetInstance()->read<DWORD>(objTemp - 4);
						//ecx_ret ������Ҫ�� �����ַ
						DWORD ecx_ret = (objTemp - ecx_sub_4) - ecx_2;
#ifdef _USER_DBG
						utils::GetInstance()->log("HXL: ecx_ret = %x name = %s", ecx_ret, (char*)(*(DWORD*)(ecx_ret - 0x70)));
#endif // _USER_DBG
						g_buttonList.push_back(ecx_ret);
					}
				}
			}
			//�ݹ��������
			travseButton(temp);
		}
	}
}
