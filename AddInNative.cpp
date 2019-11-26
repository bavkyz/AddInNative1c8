
#include "stdafx.h"
#include <iostream>


#if defined( __linux__ ) || defined(__APPLE__)
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <iconv.h>
#include <sys/time.h>
#endif

#include <stdio.h>
#include <wchar.h>
#include "AddInNative.h"
#include <string>
#include <thread>
#include <fstream>
#include <sstream>

#define DATA_LEN 65

#define BASE_ERRNO     7

#define sPortCloseCommand  "PortIsClosed"
#define wPortCloseCommand L"PortIsClosed"

#ifdef WIN32
#pragma setlocale("ru-RU" )
#endif

//#ifndef UNICODE
//#define UNICODE 1
//#endif

// link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")
//#pragma comment(lib,"wsock32.lib")

static const wchar_t* g_PropNames[] = {
	L"PortTypeIsTCP",
	L"LocalIP",
	L"LocalPort",
	L"Status",
	L"LogFile"
};
static const wchar_t* g_MethodNames[] = {
	L"ShowInStatusLine",
	L"ExternalEvent",
	L"LoadPicture",
	L"ShowMessageBox",
	L"OpenPort",
	L"ClosePort",
	L"NotifyPort",
	L"Pause",
	L"Loopback"
};

static const wchar_t* g_PropNamesRu[] = {
	L"ПотоковыйТипПорта",
	L"IP",
	L"ЛокальныйПорт",
	L"Статус",
	L"ЛогФайл"
};
static const wchar_t* g_MethodNamesRu[] = {
	L"ПоказатьВСтрокеСтатуса",
	L"ВнешнееСобытие",
	L"ЗагрузитьКартинку",
	L"ПоказатьСообщение",
	L"ОткрытьПорт",
	L"ЗакрытьПорт",
	L"ОповеститьПорт",
	L"Пауза",
	L"Петля"
};

static const wchar_t g_kClassNames[] = L"CAddInNative"; //"|OtherClass1|OtherClass2";
static IAddInDefBase* pAsyncEvent = NULL;

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);
uint32_t getLenShortWcharStr(const WCHAR_T* Source);
static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;
static WcharWrapper s_names(g_kClassNames);

//---------------------------------------------------------------------------//
long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface)
{
	if (!*pInterface)
	{
		*pInterface = new CAddInNative;
		return (long)*pInterface;
	}
	return 0;
}
//---------------------------------------------------------------------------//
AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities)
{
	g_capabilities = capabilities;
	return eAppCapabilitiesLast;
}
//---------------------------------------------------------------------------//
long DestroyObject(IComponentBase** pIntf)
{
	if (!*pIntf)
		return -1;

	delete* pIntf;
	*pIntf = 0;
	return 0;
}
//---------------------------------------------------------------------------//
const WCHAR_T* GetClassNames()
{
	return s_names;
}

// CAddInNative
//---------------------------------------------------------------------------//
CAddInNative::CAddInNative()
{
	m_iMemory = 0;
	m_iConnect = 0;
}
//---------------------------------------------------------------------------//
CAddInNative::~CAddInNative()
{
}
//---------------------------------------------------------------------------//
bool CAddInNative::Init(void* pConnection)
{
	m_iConnect = (IAddInDefBase*)pConnection;
	return m_iConnect != NULL;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetInfo()
{
	// Component should put supported component technology version 
	// This component supports 2.0 version
	return 2000;
}
//---------------------------------------------------------------------------//
void CAddInNative::Done()
{
#if !defined( __linux__ ) && !defined(__APPLE__)
	//if(m_hTimerQueue )
	//{
	//    DeleteTimerQueue(m_hTimerQueue);
	//    m_hTimerQueue = 0;
	//}
#endif //__linux__
}
/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool CAddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{
	const wchar_t* wsExtension = L"CppNativeExtension";
	int iActualSize = ::wcslen(wsExtension) + 1;
	WCHAR_T* dest = 0;

	if (m_iMemory)
	{
		if (m_iMemory->AllocMemory((void**)wsExtensionName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(wsExtensionName, wsExtension, iActualSize);
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNProps()
{
	// You may delete next lines and add your own implementation code here
	return ePropLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindProp(const WCHAR_T* wsPropName)
{
	long plPropNum = -1;
	wchar_t* propName = 0;

	::convFromShortWchar(&propName, wsPropName);
	plPropNum = findName(g_PropNames, propName, ePropLast);

	if (plPropNum == -1)
		plPropNum = findName(g_PropNamesRu, propName, ePropLast);

	delete[] propName;

	return plPropNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetPropName(long lPropNum, long lPropAlias)
{
	if (lPropNum >= ePropLast)
		return NULL;

	wchar_t* wsCurrentName = NULL;
	WCHAR_T* wsPropName = NULL;
	int iActualSize = 0;

	switch (lPropAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_PropNames[lPropNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_PropNamesRu[lPropNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsPropName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
	}

	return wsPropName;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum)
	{
	case ePropPortTypeIsTCP:
		TV_VT(pvarPropVal) = VTYPE_BOOL;
		TV_BOOL(pvarPropVal) = m_boolPortTypeIsTCP;
		break;
	case ePropLocalIP:
		wstring_to_p(m_strStatus, pvarPropVal);
		break;
	case ePropLocalPort:
		TV_VT(pvarPropVal) = VTYPE_I4;
		TV_I4(pvarPropVal) = m_i4LocalPort;
		break;
	case ePropStatus:
		wstring_to_p(m_strStatus, pvarPropVal);
		break;
	case ePropLogFile:
		wstring_to_p(m_strLogFile, pvarPropVal);
		break;
	default:
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::SetPropVal(const long lPropNum, tVariant* varPropVal)
{
	switch (lPropNum)
	{
	case ePropPortTypeIsTCP:
		if (TV_VT(varPropVal) != VTYPE_BOOL)
			return false;
		m_boolPortTypeIsTCP = TV_BOOL(varPropVal);
		break;
	case ePropLocalIP:
		if (TV_VT(varPropVal) != VTYPE_PWSTR)
			return false;
		m_LocalIP = TV_WSTR(varPropVal);
		break;
	case ePropLocalPort:
		if (TV_VT(varPropVal) != VTYPE_I4)
			return false;
		m_i4LocalPort = TV_I4(varPropVal);
		break;
	case ePropLogFile:
		if (TV_VT(varPropVal) != VTYPE_PWSTR)
			return false;
		m_strLogFile = TV_WSTR(varPropVal);
		break;
	default:
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropReadable(const long lPropNum)
{
	switch (lPropNum)
	{
	case ePropPortTypeIsTCP:
		return true;
	case ePropLocalIP:
		return true;
	case ePropLocalPort:
		return true;
	case ePropStatus:
		return true;
	case ePropLogFile:
		return true;
	default:
		return false;
	}

	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropWritable(const long lPropNum)
{
	switch (lPropNum)
	{
	case ePropPortTypeIsTCP:
		return true;
	case ePropLocalIP:
		return true;
	case ePropLocalPort:
		return true;
	case ePropLogFile:
		return true;
	default:
		return false;
	}

	return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNMethods()
{
	return eMethLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindMethod(const WCHAR_T* wsMethodName)
{
	long plMethodNum = -1;
	wchar_t* name = 0;

	::convFromShortWchar(&name, wsMethodName);

	plMethodNum = findName(g_MethodNames, name, eMethLast);

	if (plMethodNum == -1)
		plMethodNum = findName(g_MethodNamesRu, name, eMethLast);

	delete[] name;

	return plMethodNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{
	if (lMethodNum >= eMethLast)
		return NULL;

	wchar_t* wsCurrentName = NULL;
	WCHAR_T* wsMethodName = NULL;
	int iActualSize = 0;

	switch (lMethodAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_MethodNames[lMethodNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_MethodNamesRu[lMethodNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsMethodName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
	}

	return wsMethodName;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNParams(const long lMethodNum)
{
	switch (lMethodNum)
	{
	case eMethShowInStatusLine:
		return 1;
	case eMethLoadPicture:
		return 1;
	case eLoopback:
		return 1;
	case eMethExternalEvent:
		return 0;
	case eMethNotifyPort:
		return 4;
	case eMethPause:
		return 1;
	default:
		return 0;
	}

	return 0;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum,
	tVariant* pvarParamDefValue)
{
	TV_VT(pvarParamDefValue) = VTYPE_EMPTY;

	switch (lMethodNum)
	{
	case eMethShowInStatusLine:
	case eMethShowMsgBox:
		// There are no parameter values by default 
		break;
	default:
		return false;
	}

	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::HasRetVal(const long lMethodNum)
{
	switch (lMethodNum)
	{
	case eMethLoadPicture:
	case eLoopback:
		return true;
	case eMethExternalEvent:
		return true;
	default:
		return false;
	}

	return false;
}
//---------------------------------------------------------------------------//
void ClientHandlerTCP(SOCKET& handle, const int32_t& MaxSizeOfPacket, std::wstring& Status, std::wstring& LogFile, wchar_t* data)
{
	wchar_t ErrorCode[10];

	char* packet_data;
	packet_data = (char*)malloc(MaxSizeOfPacket * sizeof(char));

	//struct sockaddr_in address;
	//int addrlen = sizeof(address);

	int received_bytes = recv(handle, packet_data, MaxSizeOfPacket, 0);

	if (received_bytes == SOCKET_ERROR)
	{
		_itow(WSAGetLastError(), ErrorCode, 10);
		Status = ErrorCode;
		data = L"";
	}
	else
	{
		//if (getsockname(handle, (struct sockaddr *)&address, &addrlen) == 0 && address.sin_family == AF_INET && addrlen == sizeof(address))
		//{
		//	char ConnectionPort[10];
		//	itoa(ntohs(address.sin_port), ConnectionPort, 10);
		//}

		mbstowcs(data, packet_data, MaxSizeOfPacket);

		//std::ofstream fout(LogFile, std::ios::app);
		//fout << packet_data << std::endl;
		//fout.close();
	}

	free(packet_data);
}
//---------------------------------------------------------------------------//
void threadListenPortTCP(SOCKET& handle, const int32_t& MaxSizeOfPacket, std::wstring& Status, std::wstring& LogFile, IAddInDefBase* Connect)
{
	wchar_t ErrorCode[10];

	wchar_t* Source, * Event, * Data;
	Source = (wchar_t*)malloc(MaxSizeOfPacket * sizeof(wchar_t));
	Event = (wchar_t*)malloc(MaxSizeOfPacket * sizeof(wchar_t));
	Data = (wchar_t*)malloc(MaxSizeOfPacket * sizeof(wchar_t));

	Status = L"the socket is listening on the port TCP";

	while (true)
	{
		SOCKET connection = accept(handle, NULL, NULL);
		if (connection == INVALID_SOCKET)
		{
			_itow(WSAGetLastError(), ErrorCode, 10);
			Status = ErrorCode;
			break;
		}

		ClientHandlerTCP(std::ref(connection), std::ref(MaxSizeOfPacket), std::ref(Status), std::ref(LogFile), Source);
		if (*Source == *wPortCloseCommand)
		{
			Status = Source;
			closesocket(connection);
			break;
		}
		ClientHandlerTCP(std::ref(connection), std::ref(MaxSizeOfPacket), std::ref(Status), std::ref(LogFile), Event);
		ClientHandlerTCP(std::ref(connection), std::ref(MaxSizeOfPacket), std::ref(Status), std::ref(LogFile), Data);
		Status = L"NotifyPort";

		Connect->ExternalEvent(Source, Event, Data);

		closesocket(connection);
	}

	free(Source);
	free(Event);
	free(Data);
}
//---------------------------------------------------------------------------//
void ClientHandlerUDP(SOCKET& handle, const int32_t& MaxSizeOfPacket, std::wstring& Status, std::wstring& LogFile, wchar_t* data)
{
	wchar_t ErrorCode[10];

	char* packet_data;
	packet_data = (char*)malloc(MaxSizeOfPacket * sizeof(char));

	sockaddr_in from;
	int fromLength = sizeof(from);

	int received_bytes = recvfrom(handle, packet_data, MaxSizeOfPacket, 0, (sockaddr*)&from, &fromLength);

	if (received_bytes == SOCKET_ERROR)
	{
		_itow(WSAGetLastError(), ErrorCode, 10);
		Status = ErrorCode;
		data = L"";
	}
	else
	{
		mbstowcs(data, packet_data, MaxSizeOfPacket);
	}

	free(packet_data);
}
//---------------------------------------------------------------------------//
void threadListenPortUDP(SOCKET& handle, const int32_t& MaxSizeOfPacket, std::wstring& Status, std::wstring& LogFile, IAddInDefBase* Connect)
{
	wchar_t* Source, * Event, * Data;
	Source = (wchar_t*)malloc(MaxSizeOfPacket * sizeof(wchar_t));
	Event = (wchar_t*)malloc(MaxSizeOfPacket * sizeof(wchar_t));
	Data = (wchar_t*)malloc(MaxSizeOfPacket * sizeof(wchar_t));

	Status = L"the socket is listening on the port UDP";

	while (true)
	{
		ClientHandlerUDP(std::ref(handle), std::ref(MaxSizeOfPacket), std::ref(Status), std::ref(LogFile), Source);
		if (*Source == *wPortCloseCommand)
		{
			Status = Source;
			break;
		}
		ClientHandlerUDP(std::ref(handle), std::ref(MaxSizeOfPacket), std::ref(Status), std::ref(LogFile), Event);
		ClientHandlerUDP(std::ref(handle), std::ref(MaxSizeOfPacket), std::ref(Status), std::ref(LogFile), Data);
		Status = L"NotifyPort";

		Connect->ExternalEvent(Source, Event, Data);
	}

	free(Source);
	free(Event);
	free(Data);
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsProc(const long lMethodNum,
	tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case eMethShowInStatusLine:
		if (m_iConnect && lSizeArray)
		{
			tVariant* var = paParams;
			m_iConnect->SetStatusLine(var->pwstrVal);
		}
		break;
	case eMethShowMsgBox:
	{
		if (eAppCapabilities1 <= g_capabilities)
		{
			IAddInDefBaseEx* cnn = (IAddInDefBaseEx*)m_iConnect;
			IMsgBox* imsgbox = (IMsgBox*)cnn->GetInterface(eIMsgBox);
			if (imsgbox)
			{
				IPlatformInfo* info = (IPlatformInfo*)cnn->GetInterface(eIPlatformInfo);
				assert(info);
				const IPlatformInfo::AppInfo* plt = info->GetPlatformInfo();
				if (!plt)
					break;
				tVariant retVal;
				tVarInit(&retVal);
				if (imsgbox->Confirm(plt->AppVersion, &retVal))
				{
					bool succeed = TV_BOOL(&retVal);
					WCHAR_T* result = 0;

					if (succeed)
						::convToShortWchar(&result, L"OK");
					else
						::convToShortWchar(&result, L"Cancel");

					imsgbox->Alert(result);
					delete[] result;

				}
			}
		}
	}
	break;
	case eMethOpenPort:
	{
		wchar_t ErrorCode[10];

		// шаг 1 - подключение библиотеки
		WSADATA WsaData;
		if (WSAStartup(MAKEWORD(2, 2), &WsaData) != NO_ERROR)
		{
			_itow(WSAGetLastError(), ErrorCode, 10);
			m_strStatus = ErrorCode;
			break;
		}

		// шаг 2 - создание сокета
		if (m_boolPortTypeIsTCP) m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		else m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (m_socket == INVALID_SOCKET)
		{
			_itow(WSAGetLastError(), ErrorCode, 10);
			m_strStatus = ErrorCode;
			WSACleanup();
			break;
		}

		// шаг 3 - связывание сокета с локальным адресом
		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_port = htons(m_i4LocalPort);
		address.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(m_socket, (const sockaddr*)&address, sizeof(sockaddr_in)) < 0)
		{
			_itow(WSAGetLastError(), ErrorCode, 10);
			m_strStatus = ErrorCode;
			closesocket(m_socket);
			WSACleanup();
			break;
		}

		// шаг 4 - ожидание подключений
		if (m_boolPortTypeIsTCP)
		{
			if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR)
			{
				_itow(WSAGetLastError(), ErrorCode, 10);
				m_strStatus = ErrorCode;
				closesocket(m_socket);
				WSACleanup();
				break;
			}
		}

		// шаг 5 - обработка пакетов, присланных клиентами
		if (m_boolPortTypeIsTCP)
		{
			std::thread thr(threadListenPortTCP, std::ref(m_socket), std::ref(m_i4MaxSizeOfPacket), std::ref(m_strStatus), std::ref(m_strLogFile), m_iConnect);
			thr.detach();
		}
		else
		{
			std::thread thr(threadListenPortUDP, std::ref(m_socket), std::ref(m_i4MaxSizeOfPacket), std::ref(m_strStatus), std::ref(m_strLogFile), m_iConnect);
			thr.detach();
		}
	}
	break;
	case eMethClosePort:
	{
		wchar_t ErrorCode[10];

		// шаг 1 - подключение библиотеки
		WSADATA WsaData;
		if (WSAStartup(MAKEWORD(2, 2), &WsaData) != NO_ERROR)
		{
			_itow(WSAGetLastError(), ErrorCode, 10);
			m_strStatus = ErrorCode;
			break;
		}

		// шаг 2 - создание сокета
		SOCKET handle;
		if (m_boolPortTypeIsTCP) handle = socket(AF_INET, SOCK_STREAM, 0);
		else handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (handle == INVALID_SOCKET)
		{
			_itow(WSAGetLastError(), ErrorCode, 10);
			m_strStatus = ErrorCode;
			WSACleanup();
			break;
		}

		// шаг 3 - определение получателя данных и самих данных
		unsigned int ip_a = 127;
		unsigned int ip_b = 0;
		unsigned int ip_c = 0;
		unsigned int ip_d = 1;
		unsigned int ip = (ip_a << 24) | (ip_b << 16) | (ip_c << 8) | ip_d;
		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(ip);
		address.sin_port = htons(m_i4LocalPort);
		const char* packet_data = sPortCloseCommand;
		const int packet_size = sizeof(packet_data);

		//шаг 4 - установка соединения
		if (m_boolPortTypeIsTCP)
		{
			if (connect(handle, (sockaddr*)&address, sizeof(sockaddr_in)) == SOCKET_ERROR)
			{
				_itow(WSAGetLastError(), ErrorCode, 10);
				m_strStatus = ErrorCode;
				closesocket(handle);
				WSACleanup();
				break;
			}
		}

		// шаг 5 - отправка данных
		int sent_bytes;
		if (m_boolPortTypeIsTCP)
		{
			sent_bytes = send(handle, packet_data, m_i4MaxSizeOfPacket, 0);
		}
		else
		{
			sent_bytes = sendto(handle, packet_data, m_i4MaxSizeOfPacket, 0, (sockaddr*)&address, sizeof(address));
		}
		//if (sent_bytes != packet_size) m_strStatus = "failed to send packet";
		//else m_strStatus = "packet sent";
		closesocket(handle);

		Sleep(100); // ждем закрытия серверного потока
		closesocket(m_socket);
		WSACleanup();
	}
	break;
	case eMethNotifyPort:
	{
		wchar_t ErrorCode[10];

		// шаг 1 - подключение библиотеки
		WSADATA WsaData;
		if (WSAStartup(MAKEWORD(2, 2), &WsaData) != NO_ERROR)
		{
			_itow(WSAGetLastError(), ErrorCode, 10);
			m_strStatus = ErrorCode;
			break;
		}

		// шаг 2 - создание сокета
		SOCKET handle;
		if (m_boolPortTypeIsTCP) handle = socket(AF_INET, SOCK_STREAM, 0);
		else handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (handle == INVALID_SOCKET)
		{
			_itow(WSAGetLastError(), ErrorCode, 10);
			m_strStatus = ErrorCode;
			WSACleanup();
			break;
		}

		// шаг 3 - определение получателя данных и самих данных
		setlocale(LC_CTYPE, "");
		const std::string ips(m_LocalIP.begin(), m_LocalIP.end());
		std::stringstream ssss(ips);
		int ip_a, ip_b, ip_c, ip_d;
		char ch; //to temporarily store the '.'
		ssss >> ip_a >> ch >> ip_b >> ch >> ip_c >> ch >> ip_d;
		unsigned int ip = (ip_a << 24) | (ip_b << 16) | (ip_c << 8) | ip_d;

		sockaddr_in address;
		address.sin_family = AF_INET;

		address.sin_addr.s_addr = htonl(ip);
		if (TV_VT(&paParams[0]) != VTYPE_I4) // определям порт получателя
		{
			m_strStatus = L"only a int can be sent to the port";
			closesocket(handle);
			WSACleanup();
			break;
		}
		address.sin_port = htons(TV_I4(&paParams[0]));
		if (TV_VT(&paParams[1]) != VTYPE_PWSTR) // определяем Источник отправителя
		{
			m_strStatus = L"only a string can be sent to the source";
			closesocket(handle);
			WSACleanup();
			break;
		}
		std::wstring data1 = TV_WSTR(&paParams[1]);
		const char* packet_data1 = wchar_to_char(data1.c_str(), CP_ACP);
		const int packet_size1 = sizeof(packet_data1);
		if (TV_VT(&paParams[2]) != VTYPE_PWSTR) // определяем Событие отправителя
		{
			m_strStatus = L"only a string can be sent to the event";
			closesocket(handle);
			WSACleanup();
			break;
		}
		std::wstring data2 = TV_WSTR(&paParams[2]);
		const char* packet_data2 = wchar_to_char(data2.c_str(), CP_ACP);
		const int packet_size2 = sizeof(packet_data2);
		if (TV_VT(&paParams[3]) != VTYPE_PWSTR) // определяем Данные отправителя
		{
			m_strStatus = L"only a string can be sent to the port";
			closesocket(handle);
			WSACleanup();
			break;
		}
		std::wstring data3 = TV_WSTR(&paParams[3]);
		const char* packet_data3 = wchar_to_char(data3.c_str(), CP_ACP);
		const int packet_size3 = sizeof(packet_data3);

		//шаг 4 - установка соединения
		if (m_boolPortTypeIsTCP)
		{
			if (connect(handle, (sockaddr*)&address, sizeof(sockaddr_in)) == SOCKET_ERROR)
			{
				_itow(WSAGetLastError(), ErrorCode, 10);
				m_strStatus = ErrorCode;
				closesocket(handle);
				WSACleanup();
				break;
			}
		}

		// шаг 5 - отправка данных
		int sent_bytes1, sent_bytes2, sent_bytes3;
		if (m_boolPortTypeIsTCP)
		{
			sent_bytes1 = send(handle, packet_data1, m_i4MaxSizeOfPacket, 0);
			sent_bytes2 = send(handle, packet_data2, m_i4MaxSizeOfPacket, 0);
			sent_bytes3 = send(handle, packet_data3, m_i4MaxSizeOfPacket, 0);
		}
		else
		{
			sent_bytes1 = sendto(handle, packet_data1, m_i4MaxSizeOfPacket, 0, (sockaddr*)&address, sizeof(address));
			sent_bytes2 = sendto(handle, packet_data2, m_i4MaxSizeOfPacket, 0, (sockaddr*)&address, sizeof(address));
			sent_bytes3 = sendto(handle, packet_data3, m_i4MaxSizeOfPacket, 0, (sockaddr*)&address, sizeof(address));
		}
		//if (sent_bytes != packet_size) m_strStatus = "failed to send packet";
		//else m_strStatus = "packet sent";
		closesocket(handle);
		WSACleanup();
	}
	break;
	case eMethPause:
	{
		if (TV_VT(paParams) != VTYPE_I4) break;
		Sleep(TV_I4(paParams));
	}
	break;
	default:
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsFunc(const long lMethodNum,
	tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	bool ret = false;
	FILE* file = 0;
	char* name = 0;
	int size = 0;
	char* mbstr = 0;
	wchar_t* wsTmp = 0;
	char* loc = 0;

	switch (lMethodNum)
	{
		// Method acceps one argument of type BinaryData ant returns its copy
	case eLoopback:
	{
		if (lSizeArray != 1 || !paParams)
			return false;

		if (TV_VT(paParams) != VTYPE_BLOB)
			return false;

		if (paParams->strLen > 0)
		{
			m_iMemory->AllocMemory((void**)&pvarRetValue->pstrVal, paParams->strLen);
			memcpy((void*)pvarRetValue->pstrVal, (void*)paParams->pstrVal, paParams->strLen);
		}

		TV_VT(pvarRetValue) = VTYPE_BLOB;
		pvarRetValue->strLen = paParams->strLen;
		return true;
	}
	break;

	case eMethLoadPicture:
	{
		if (!lSizeArray || !paParams)
			return false;

		switch (TV_VT(paParams))
		{
		case VTYPE_PSTR:
			name = paParams->pstrVal;
			break;
		case VTYPE_PWSTR:
			loc = setlocale(LC_ALL, "");
			::convFromShortWchar(&wsTmp, TV_WSTR(paParams));
			size = wcstombs(0, wsTmp, 0) + 1;
			assert(size);
			mbstr = new char[size];
			assert(mbstr);
			memset(mbstr, 0, size);
			size = wcstombs(mbstr, wsTmp, getLenShortWcharStr(TV_WSTR(paParams)));
			name = mbstr;
			setlocale(LC_ALL, loc);
			delete[] wsTmp;
			break;
		default:
			return false;
		}
	}

	file = fopen(name, "rb");

	if (file == 0)
	{
		wchar_t* wsMsgBuf;
		uint32_t err = errno;
		name = strerror(err);
		int sizeloc = mbstowcs(0, name, 0) + 1;
		assert(sizeloc);
		wsMsgBuf = new wchar_t[sizeloc];
		assert(wsMsgBuf);
		memset(wsMsgBuf, 0, sizeloc * sizeof(wchar_t));
		sizeloc = mbstowcs(wsMsgBuf, name, sizeloc);

		addError(ADDIN_E_VERY_IMPORTANT, L"AddInNative", wsMsgBuf, RESULT_FROM_ERRNO(err));
		delete[] wsMsgBuf;
		return false;
	}

	fseek(file, 0, SEEK_END);
	size = ftell(file);

	if (size && m_iMemory->AllocMemory((void**)&pvarRetValue->pstrVal, size))
	{
		fseek(file, 0, SEEK_SET);
		size = fread(pvarRetValue->pstrVal, 1, size, file);
		pvarRetValue->strLen = size;
		TV_VT(pvarRetValue) = VTYPE_BLOB;

		ret = true;
	}
	if (file)
		fclose(file);

	if (mbstr && size != -1)
		delete[] mbstr;

	break;

	case eMethExternalEvent:
	{
		pAsyncEvent = m_iConnect;
		TV_VT(pvarRetValue) = VTYPE_BOOL;
		TV_BOOL(pvarRetValue) = MyExternalEvent();
		return true;
	}
	break;

	}
	return ret;
}
//---------------------------------------------------------------------------//
// This code will work only on the client!
#if !defined( __linux__ ) && !defined(__APPLE__)
bool CAddInNative::MyExternalEvent()
{
	if (!pAsyncEvent)
		return false;

	wchar_t* who = L"ComponentNative", * what = L"test", * data = L"test";

	//wmemset(data, 0, DATA_LEN);

	return pAsyncEvent->ExternalEvent(who, what, data);

	//delete[] data;
}
#else
bool CAddInNative::MyExternalEvent()
{
	if (!pAsyncEvent)
		return false;

	WCHAR_T* who = 0, * what = 0, * data = 0;

	wmemset(data, 0, DATA_LEN);
	::convToShortWchar(&who, L"ComponentNative");
	::convToShortWchar(&what, L"test");
	::convToShortWchar(&data, L"test");

	result = pAsyncEvent->ExternalEvent(who, what, data);

	delete[] who;
	delete[] what;
	delete[] data;

	return result;
}
#endif
//---------------------------------------------------------------------------//
bool CAddInNative::wstring_to_p(std::wstring str, tVariant* val) {
	char* t1;
	TV_VT(val) = VTYPE_PWSTR;
	m_iMemory->AllocMemory((void**)&t1, (str.length() + 1) * sizeof(WCHAR_T));
	memcpy(t1, str.c_str(), (str.length() + 1) * sizeof(WCHAR_T));
	val->pstrVal = t1;
	val->strLen = str.length();
	return true;
}
//---------------------------------------------------------------------------//
char* CAddInNative::wchar_to_char(const wchar_t* Source, UINT CodePage) {
	char* Dest;
	size_t len;

	len = WideCharToMultiByte(
		CodePage,   // Code page
		0,			// Default replacement of illegal chars
		Source,		// Multibyte characters string
		-1,			// Number of unicode chars is not known
		NULL,		// No buffer yet, allocate it later
		0,			// No buffer
		NULL,		// Use system default
		NULL		// We are not interested whether the default char was used
	);

	if (len == 0) return "";
	else m_iMemory->AllocMemory((void**)&Dest, len);

	len = WideCharToMultiByte(
		CodePage,    // Code page
		0,			// Default replacement of illegal chars
		Source,		// Multibyte characters string
		-1,			// Number of unicode chars is not known
		Dest,		// Output buffer
		len,		// buffer size
		NULL,		// Use system default
		NULL		// We are not interested whether the default char was used
	);

	if (len == 0) return "";

	return Dest;
}
//---------------------------------------------------------------------------//
void CAddInNative::SetLocale(const WCHAR_T* loc)
{
#if !defined( __linux__ ) && !defined(__APPLE__)
	_wsetlocale(LC_ALL, loc);
#else
	//We convert in char* char_locale
	//also we establish locale
	//setlocale(LC_ALL, char_locale);
#endif
}
/////////////////////////////////////////////////////////////////////////////
// LocaleBase
//---------------------------------------------------------------------------//
bool CAddInNative::setMemManager(void* mem)
{
	m_iMemory = (IMemoryManager*)mem;
	return m_iMemory != 0;
}
//---------------------------------------------------------------------------//
void CAddInNative::addError(uint32_t wcode, const wchar_t* source,
	const wchar_t* descriptor, long code)
{
	if (m_iConnect)
	{
		WCHAR_T* err = 0;
		WCHAR_T* descr = 0;

		::convToShortWchar(&err, source);
		::convToShortWchar(&descr, descriptor);

		m_iConnect->AddError(wcode, err, descr, code);
		delete[] err;
		delete[] descr;
	}
}
//---------------------------------------------------------------------------//
long CAddInNative::findName(const wchar_t* names[], const wchar_t* name,
	const uint32_t size) const
{
	long ret = -1;
	for (uint32_t i = 0; i < size; i++)
	{
		if (!wcscmp(names[i], name))
		{
			ret = i;
			break;
		}
	}
	return ret;
}
//---------------------------------------------------------------------------//
uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len)
{
	if (!len)
		len = ::wcslen(Source) + 1;

	if (!*Dest)
		*Dest = new WCHAR_T[len];

	WCHAR_T* tmpShort = *Dest;
	wchar_t* tmpWChar = (wchar_t*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(WCHAR_T));
#ifdef __linux__
	size_t succeed = (size_t)-1;
	size_t f = len * sizeof(wchar_t), t = len * sizeof(WCHAR_T);
	const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
	iconv_t cd = iconv_open("UTF-16LE", fromCode);
	if (cd != (iconv_t)-1)
	{
		succeed = iconv(cd, (char**)&tmpWChar, &f, (char**)&tmpShort, &t);
		iconv_close(cd);
		if (succeed != (size_t)-1)
			return (uint32_t)succeed;
	}
#endif //__linux__
	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
	{
		*tmpShort = (WCHAR_T)*tmpWChar;
	}

	return res;
}
//---------------------------------------------------------------------------//
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len)
{
	if (!len)
		len = getLenShortWcharStr(Source) + 1;

	if (!*Dest)
		*Dest = new wchar_t[len];

	wchar_t* tmpWChar = *Dest;
	WCHAR_T* tmpShort = (WCHAR_T*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(wchar_t));
#ifdef __linux__
	size_t succeed = (size_t)-1;
	const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
	size_t f = len * sizeof(WCHAR_T), t = len * sizeof(wchar_t);
	iconv_t cd = iconv_open("UTF-32LE", fromCode);
	if (cd != (iconv_t)-1)
	{
		succeed = iconv(cd, (char**)&tmpShort, &f, (char**)&tmpWChar, &t);
		iconv_close(cd);
		if (succeed != (size_t)-1)
			return (uint32_t)succeed;
	}
#endif //__linux__
	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
	{
		*tmpWChar = (wchar_t)*tmpShort;
	}

	return res;
}
//---------------------------------------------------------------------------//
uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
	uint32_t res = 0;
	WCHAR_T* tmpShort = (WCHAR_T*)Source;

	while (*tmpShort++)
		++res;

	return res;
}
//---------------------------------------------------------------------------//

#ifdef LINUX_OR_MACOS
WcharWrapper::WcharWrapper(const WCHAR_T* str) : m_str_WCHAR(NULL),
m_str_wchar(NULL)
{
	if (str)
	{
		int len = getLenShortWcharStr(str);
		m_str_WCHAR = new WCHAR_T[len + 1];
		memset(m_str_WCHAR, 0, sizeof(WCHAR_T) * (len + 1));
		memcpy(m_str_WCHAR, str, sizeof(WCHAR_T) * len);
		::convFromShortWchar(&m_str_wchar, m_str_WCHAR);
	}
}
#endif
//---------------------------------------------------------------------------//
WcharWrapper::WcharWrapper(const wchar_t* str) :
#ifdef LINUX_OR_MACOS
	m_str_WCHAR(NULL),
#endif 
	m_str_wchar(NULL)
{
	if (str)
	{
		int len = wcslen(str);
		m_str_wchar = new wchar_t[len + 1];
		memset(m_str_wchar, 0, sizeof(wchar_t) * (len + 1));
		memcpy(m_str_wchar, str, sizeof(wchar_t) * len);
#ifdef LINUX_OR_MACOS
		::convToShortWchar(&m_str_WCHAR, m_str_wchar);
#endif
	}

}
//---------------------------------------------------------------------------//
WcharWrapper::~WcharWrapper()
{
#ifdef LINUX_OR_MACOS
	if (m_str_WCHAR)
	{
		delete[] m_str_WCHAR;
		m_str_WCHAR = NULL;
	}
#endif
	if (m_str_wchar)
	{
		delete[] m_str_wchar;
		m_str_wchar = NULL;
	}
}
//---------------------------------------------------------------------------//
