//////////////////////////////////////////////////////////////////////////  
/// COPYRIGHT NOTICE  
/// Copyright (c) 2009, ���пƼ���ѧtickTick Group  ����Ȩ������  
/// All rights reserved.  
///   
/// @file    SerialPort.cpp    
/// @brief   ����ͨ�����ʵ���ļ�  
///  
/// ���ļ�Ϊ����ͨ�����ʵ�ִ���  
///  
/// @version 1.0     
/// @author  ¬��    
/// @E-mail��lujun.hust@gmail.com  
/// @date    2010/03/19  
///   
///  
///  �޶�˵����  
//////////////////////////////////////////////////////////////////////////  
 
#include "StdAfx.h"  
#include "SerialPort.h"  
#include <process.h>  
#include <iostream>  
 
/** �߳��˳���־ */   
bool CSerialPort::s_bExit = false;  
/** ������������ʱ,sleep���´β�ѯ�����ʱ��,��λ:�� */   
const UINT SLEEP_TIME_INTERVAL = 0;  
 
void InitMonitorData (void);

CSerialPort::CSerialPort(void)  
: m_hListenThread(INVALID_HANDLE_VALUE)  
{  
    m_hComm = INVALID_HANDLE_VALUE;  
    m_hListenThread = INVALID_HANDLE_VALUE;  
	fildes = NULL;
	useFileFlag = 0;
	InitMonitorData();
    InitializeCriticalSection(&m_csCommunicationSync);  
 
}  
 
// store the monitor data, 4*50*8=1600 bytes
double monitorData[4][50] = {0.0};
// store the monitor Count less than 50
UINT	monitorCount = 0;

void InitMonitorData (void)
{
	monitorCount = 0;
	memset(monitorData, 0, 1600);
}

void AddMonitorData (float *fdata, double (*monitorData)[50])
{
	UINT iter;

	if (monitorCount < 50)
	{
		monitorData[0][monitorCount] = fdata[0];
		monitorData[1][monitorCount] = fdata[1];
		monitorData[2][monitorCount] = fdata[2];
		monitorData[3][monitorCount] = fdata[3];
		monitorCount ++;
		return;
	}

	// shift all the data to left, and delete the first ones
	for (iter = 0; iter < 50; iter ++)
	{
		monitorData[0][iter] = monitorData[0][iter+1];
		monitorData[1][iter] = monitorData[1][iter+1];
		monitorData[2][iter] = monitorData[2][iter+1];
		monitorData[3][iter] = monitorData[3][iter+1];
	}
	monitorData[0][49] = fdata[0];
	monitorData[1][49] = fdata[1];
	monitorData[2][49] = fdata[2];
	monitorData[3][49] = fdata[3];
}

UINT TransferChar2Float (char *data, float *fdata)
{
	union float_data {float dataf; char varf[4];} FloatData;
	FloatData.varf[3] = data[0];
	FloatData.varf[2] = data[1];
	FloatData.varf[1] = data[2];
	FloatData.varf[0] = data[3];
	*fdata = FloatData.dataf;
	return 0;
}

#if 0
UINT TransferChar2Float (char *data, float *fdata)
{
	float out_data = 0.0;
	int data_2, data_3, data_4;
	int data_exp = ((data[0] && 0x7f) << 1)+ (data[1] >> 7) - 0x7f;
	data_2 = data[1];
	data_3 = data[2];
	data_4 = data[3];
	if (data_exp == -127)
	{
		out_data = 0.0;
		/*
		out_data = (float) ((( //(0x0001 << 23) +
			((data_2 && 0x7f) << 16) +
			(data_3 << 8) +
			data_4) * 1.0) / (0x0001 << (23-data_exp)));
			*/
	}
	else if (!data_exp)
	{
		AfxMessageBox(_T("����Ϊ�����"));
	}
	else
	{
		out_data = (float) ((((0x0001 << 23) +
			((data_2 && 0x7f) << 16) +
			(data_3 << 8) +
			data_4) * 1.0) / (0x0001 << (23-data_exp)));		
	}
	*fdata = out_data;
	//MessageBox(NULL, _T("timer"), NULL, 0);
	return 0;
}
#endif

CSerialPort::~CSerialPort(void)  
{
	if (fildes)
	{
		useFileFlag = 0;
		fclose(fildes);
	}
    CloseListenTread();
    ClosePort();  
    DeleteCriticalSection(&m_csCommunicationSync);  
}  
 
bool CSerialPort::InitPort( UINT portNo /*= 1*/,UINT baud /*= CBR_9600*/,char parity /*= 'N'*/,  
                            UINT databits /*= 8*/, UINT stopsbits /*= 1*/,DWORD dwCommEvents /*= EV_RXCHAR*/ )  
{  
 
    /** ��ʱ����,���ƶ�����ת��Ϊ�ַ�����ʽ,�Թ���DCB�ṹ */   
    char szDCBparam[50];  
    sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);  
 
    /** ��ָ������,�ú����ڲ��Ѿ����ٽ�������,�����벻Ҫ�ӱ��� */   
    if (!openPort(portNo))  
    {  
        return false;  
    }  
 
    /** �����ٽ�� */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** �Ƿ��д����� */   
    BOOL bIsSuccess = TRUE;  
 
    /** �ڴ˿���������������Ļ�������С,���������,��ϵͳ������Ĭ��ֵ.  
     *  �Լ����û�������Сʱ,Ҫע�������Դ�һЩ,���⻺�������  
     */ 
    /*if (bIsSuccess )  
    {  
        bIsSuccess = SetupComm(m_hComm,10,10);  
    }*/ 
 
    /** ���ô��ڵĳ�ʱʱ��,����Ϊ0,��ʾ��ʹ�ó�ʱ���� */ 
    COMMTIMEOUTS  CommTimeouts;  
    CommTimeouts.ReadIntervalTimeout         = 0;  
    CommTimeouts.ReadTotalTimeoutMultiplier  = 0;  
    CommTimeouts.ReadTotalTimeoutConstant    = 0;  
    CommTimeouts.WriteTotalTimeoutMultiplier = 0;  
    CommTimeouts.WriteTotalTimeoutConstant   = 0;   
    if ( bIsSuccess)  
    {  
        bIsSuccess = SetCommTimeouts(m_hComm, &CommTimeouts);  
    }  
 
    DCB  dcb;  
    if ( bIsSuccess )  
    {  
        // ��ANSI�ַ���ת��ΪUNICODE�ַ���  
        DWORD dwNum = MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, NULL, 0);  
        wchar_t *pwText = new wchar_t[dwNum] ;  
        if (!MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, pwText, dwNum))  
        {  
            bIsSuccess = TRUE;  
        }  
 
        /** ��ȡ��ǰ�������ò���,���ҹ��촮��DCB���� */   
        bIsSuccess = GetCommState(m_hComm, &dcb) && BuildCommDCB(pwText, &dcb) ;  
        /** ����RTS flow���� */   
        dcb.fRtsControl = RTS_CONTROL_ENABLE;   
 
        /** �ͷ��ڴ�ռ� */   
        delete [] pwText;  
    }  
 
    if ( bIsSuccess )  
    {  
        /** ʹ��DCB�������ô���״̬ */   
        bIsSuccess = SetCommState(m_hComm, &dcb);  
    }  
          
    /**  ��մ��ڻ����� */ 
    PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);  
 
    /** �뿪�ٽ�� */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return bIsSuccess==TRUE;  
}  
 
bool CSerialPort::InitPort( UINT portNo ,const LPDCB& plDCB )  
{  
    /** ��ָ������,�ú����ڲ��Ѿ����ٽ�������,�����벻Ҫ�ӱ��� */   
    if (!openPort(portNo))  
    {  
        return false;  
    }  
      
    /** �����ٽ�� */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** ���ô��ڲ��� */   
    if (!SetCommState(m_hComm, plDCB))  
    {  
        return false;  
    }  
 
    /**  ��մ��ڻ����� */ 
    PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);  
 
    /** �뿪�ٽ�� */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return true;  
}  
 
void CSerialPort::ClosePort()  
{  
    /** ����д��ڱ��򿪣��ر��� */ 
    if( m_hComm != INVALID_HANDLE_VALUE )  
    {  
        CloseHandle( m_hComm );  
        m_hComm = INVALID_HANDLE_VALUE;  
    }  
}  
 
bool CSerialPort::openPort( UINT portNo )  
{  
    /** �����ٽ�� */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** �Ѵ��ڵı��ת��Ϊ�豸�� */   
    char szPort[50];  
    sprintf_s(szPort, "COM%d", portNo);  
 
    /** ��ָ���Ĵ��� */   
    m_hComm = CreateFileA(szPort,  /** �豸��,COM1,COM2�� */   
              GENERIC_READ | GENERIC_WRITE, /** ����ģʽ,��ͬʱ��д */     
              0,                            /** ����ģʽ,0��ʾ������ */   
              NULL,                         /** ��ȫ������,һ��ʹ��NULL */   
              OPEN_EXISTING,                /** �ò�����ʾ�豸�������,���򴴽�ʧ�� */   
              0,      
              0);      
 
    /** �����ʧ�ܣ��ͷ���Դ������ */   
    if (m_hComm == INVALID_HANDLE_VALUE)  
    {  
        LeaveCriticalSection(&m_csCommunicationSync);  
        return false;  
    }  
 
    /** �˳��ٽ��� */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return true;  
}  
 
bool CSerialPort::OpenListenThread()  
{  
    /** ����߳��Ƿ��Ѿ������� */   
    if (m_hListenThread != INVALID_HANDLE_VALUE)  
    {  
        /** �߳��Ѿ����� */   
        return false;  
    }  
 
    s_bExit = false;  
    /** �߳�ID */   
    UINT threadId;  
    /** �����������ݼ����߳� */   
    m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, &threadId);  
    if (!m_hListenThread)  
    {  
        return false;  
    }  
    /** �����̵߳����ȼ�,������ͨ�߳� */   
    if (!SetThreadPriority(m_hListenThread, THREAD_PRIORITY_ABOVE_NORMAL))  
    {  
        return false;  
    }  

	// init the time
	timeCounter = timeDiff = 0;

	// init the transfer state
	dataTransferFlag = 0;

    return true;  
}  
 
bool CSerialPort::CloseListenTread()  
{     
    if (m_hListenThread != INVALID_HANDLE_VALUE)  
    {  
        /** ֪ͨ�߳��˳� */   
        s_bExit = true;  
 
        /** �ȴ��߳��˳� */   
        Sleep(10);  
 
        /** ���߳̾����Ч */   
        CloseHandle( m_hListenThread );  
        m_hListenThread = INVALID_HANDLE_VALUE;  
    }  
    return true;  
}  
 
UINT CSerialPort::GetBytesInCOM()  
{  
    DWORD dwError = 0;  /** ������ */   
    COMSTAT  comstat;   /** COMSTAT�ṹ��,��¼ͨ���豸��״̬��Ϣ */   
    memset(&comstat, 0, sizeof(COMSTAT));  
 
    UINT BytesInQue = 0;  
    /** �ڵ���ReadFile��WriteFile֮ǰ,ͨ�������������ǰ�����Ĵ����־ */   
    if ( ClearCommError(m_hComm, &dwError, &comstat) )  
    {  
        BytesInQue = comstat.cbInQue; /** ��ȡ�����뻺�����е��ֽ��� */   
    }  
 
    return BytesInQue;  
}  

void recordTime(char *time)
{ 
	int len;
	SYSTEMTIME st;
	CString strDate, strTime;
	TCHAR temp[40] = {0};
	GetLocalTime(&st);
	strDate.Format(_T("%4d-%2d-%2d"), st.wYear, st.wMonth, st.wDay);
	strTime.Format(_T("%02d:%02d:%02d"), st.wHour, st.wMinute, st.wSecond);
	_tcscpy(temp, strTime);
	len = WideCharToMultiByte(CP_ACP, 0, temp, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, temp, -1, time, len, NULL, NULL);
}

UINT WINAPI CSerialPort::ListenThread( void* pParam )  
{  
    /** �õ������ָ�� */   
    CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);  
 
    // �߳�ѭ��,��ѯ��ʽ��ȡ��������  
    while (!pSerialPort->s_bExit)   
    {  
        UINT BytesInQue = pSerialPort->GetBytesInCOM();  
        /** ����������뻺������������,����Ϣһ���ٲ�ѯ */   
        if ( BytesInQue == 0 )  
        {  
            Sleep(SLEEP_TIME_INTERVAL);  
            continue;  
        }  
 
        /** ��ȡ���뻺�����е����ݲ������ʾ */ 
        char cRecved = 0x00;
		char fbuf[0x100] = "\0", dbuf[64] = "\0";
		TCHAR dbuf2[64] = _T("\0");
        do 
        {  
            cRecved = 0x00;  
            if(pSerialPort->ReadChar(cRecved) == true)
            {  
                // compare the time data transfered
				if (!pSerialPort->dataTransferFlag)
				{
					// the first time data transfered
					memset(pSerialPort->data, 0, 8);
					memset(pSerialPort->fdata, 0, 16);
					pSerialPort->fcounter = 0;
					pSerialPort->counter = 0;
					// get the data and flush time
					pSerialPort->dataTransferFlag = 1;
					pSerialPort->needWait50msFlag = 0;
					pSerialPort->data[pSerialPort->counter] = cRecved;
					pSerialPort->counter = (pSerialPort->counter + 1) % 4;	// make the counter in [0,1,2,3] set
					pSerialPort->timeCounter = timeGetTime();
					continue;
				}
				else
				{
					// this is not first data transfered time
					// compare current time with the previous time
					// if bigger than 10 ms then throw it away
					// if bigger than 50 ms then set the data as new frame start
					pSerialPort->timeDiff = timeGetTime() - pSerialPort->timeCounter;
					//memset(dbuf, 0, sizeof(dbuf));
					//memset(dbuf2, 0, sizeof(dbuf2));
					//sprintf_s(dbuf, 255, "%d", pSerialPort->timeDiff);
					//MultiByteToWideChar(CP_ACP, 0, dbuf, strlen(dbuf)+1, dbuf2, sizeof(dbuf2) / sizeof(dbuf2[0]));
					//MessageBox(NULL, dbuf2, 0, 0);
					//if (pSerialPort->timeDiff > 46)
					if (pSerialPort->timeDiff > 26)
					{
						// if this is a new frame
						if (!pSerialPort->fcounter && !pSerialPort->counter)
						{
							pSerialPort->needWait50msFlag = 0;
							// // get the data and flush time
							pSerialPort->data[pSerialPort->counter] = cRecved;
							pSerialPort->counter = (pSerialPort->counter + 1) % 4;	// make the counter in [0,1,2,3] set
							pSerialPort->timeCounter = timeGetTime();
							continue;
						}
						// if this is not a new frame
						// throw it away and wait another 50 ms
						// need wait 50 ms to get the new frame
					}
					else	if (pSerialPort->timeDiff < 10)
					{
						if (pSerialPort->needWait50msFlag)
						{
							// only flush the time, not get the data
							pSerialPort->timeCounter = timeGetTime();
						}
						else
						{
							// // get the data and flush time
							pSerialPort->data[pSerialPort->counter] = cRecved;
							if (3 == pSerialPort->counter)
							{
								TransferChar2Float(pSerialPort->data, &pSerialPort->fdata[pSerialPort->fcounter]);

								if (3 == pSerialPort->fcounter)
								{
									if (pSerialPort->useFileFlag)
									{
									// record the time
										recordTime(pSerialPort->time);
									// write into file
										memset(fbuf, 0, sizeof(fbuf));
										sprintf_s(fbuf, 255, "%s %.8f %.8f %.8f %.8f\r\n", pSerialPort->time, pSerialPort->fdata[0],
											pSerialPort->fdata[1], pSerialPort->fdata[2], pSerialPort->fdata[3]);
										fwrite(fbuf, strlen(fbuf), 1, pSerialPort->fildes);
									}

									// copy to diagram show
									AddMonitorData(pSerialPort->fdata, monitorData);
								}
								pSerialPort->fcounter = (pSerialPort->fcounter + 1) % 4;
							}
							pSerialPort->counter = (pSerialPort->counter + 1) % 4;	// make the counter in [0,1,2,3] set
							pSerialPort->timeCounter = timeGetTime();
						}
						continue;
					}
					// throw it away and wait another 50 ms
					// need wait 50 ms to get the new frame
				}
				memset(pSerialPort->data, 0, 8);
				memset(pSerialPort->fdata, 0, 16);
				pSerialPort->fcounter = 0;
				pSerialPort->counter = 0;
				pSerialPort->needWait50msFlag = 1;
				// only flush the time, not get the data
				pSerialPort->timeCounter = timeGetTime();
                continue;  
            }  
        }while(--BytesInQue);
    }
    return 0;
}  
 
bool CSerialPort::ReadChar( char &cRecved )  
{  
    BOOL  bResult     = TRUE;  
    DWORD BytesRead   = 0;  
    if(m_hComm == INVALID_HANDLE_VALUE)  
    {  
        return false;  
    }  
 
    /** �ٽ������� */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** �ӻ�������ȡһ���ֽڵ����� */   
    bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);  
    if ((!bResult))  
    {   
        /** ��ȡ������,���Ը��ݸô�����������ԭ�� */   
        DWORD dwError = GetLastError();  
 
        /** ��մ��ڻ����� */   
        PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);  
        LeaveCriticalSection(&m_csCommunicationSync);  
 
        return false;  
    }  
 
    /** �뿪�ٽ��� */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return (BytesRead == 1);  
 
}  
 
bool CSerialPort::WriteData( unsigned char* pData, unsigned int length )  
{  
    BOOL   bResult     = TRUE;  
    DWORD  BytesToSend = 0;  
    if(m_hComm == INVALID_HANDLE_VALUE)  
    {  
        return false;  
    }  
 
    /** �ٽ������� */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** �򻺳���д��ָ���������� */   
    bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);  
    if (!bResult)    
    {  
        DWORD dwError = GetLastError();  
        /** ��մ��ڻ����� */   
        PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);  
        LeaveCriticalSection(&m_csCommunicationSync);  
 
        return false;  
    }  
 
    /** �뿪�ٽ��� */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return true;  
}
