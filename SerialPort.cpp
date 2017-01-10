//////////////////////////////////////////////////////////////////////////  
/// COPYRIGHT NOTICE  
/// Copyright (c) 2009, 华中科技大学tickTick Group  （版权声明）  
/// All rights reserved.  
///   
/// @file    SerialPort.cpp    
/// @brief   串口通信类的实现文件  
///  
/// 本文件为串口通信类的实现代码  
///  
/// @version 1.0     
/// @author  卢俊    
/// @E-mail：lujun.hust@gmail.com  
/// @date    2010/03/19  
///   
///  
///  修订说明：  
//////////////////////////////////////////////////////////////////////////  
 
#include "StdAfx.h"  
#include "SerialPort.h"  
#include <process.h>  
#include <iostream>  
 
/** 线程退出标志 */   
bool CSerialPort::s_bExit = false;  
/** 当串口无数据时,sleep至下次查询间隔的时间,单位:秒 */   
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
		AfxMessageBox(_T("数据为无穷大"));
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
 
    /** 临时变量,将制定参数转化为字符串形式,以构造DCB结构 */   
    char szDCBparam[50];  
    sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);  
 
    /** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */   
    if (!openPort(portNo))  
    {  
        return false;  
    }  
 
    /** 进入临界段 */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** 是否有错误发生 */   
    BOOL bIsSuccess = TRUE;  
 
    /** 在此可以设置输入输出的缓冲区大小,如果不设置,则系统会设置默认值.  
     *  自己设置缓冲区大小时,要注意设置稍大一些,避免缓冲区溢出  
     */ 
    /*if (bIsSuccess )  
    {  
        bIsSuccess = SetupComm(m_hComm,10,10);  
    }*/ 
 
    /** 设置串口的超时时间,均设为0,表示不使用超时限制 */ 
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
        // 将ANSI字符串转换为UNICODE字符串  
        DWORD dwNum = MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, NULL, 0);  
        wchar_t *pwText = new wchar_t[dwNum] ;  
        if (!MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, pwText, dwNum))  
        {  
            bIsSuccess = TRUE;  
        }  
 
        /** 获取当前串口配置参数,并且构造串口DCB参数 */   
        bIsSuccess = GetCommState(m_hComm, &dcb) && BuildCommDCB(pwText, &dcb) ;  
        /** 开启RTS flow控制 */   
        dcb.fRtsControl = RTS_CONTROL_ENABLE;   
 
        /** 释放内存空间 */   
        delete [] pwText;  
    }  
 
    if ( bIsSuccess )  
    {  
        /** 使用DCB参数配置串口状态 */   
        bIsSuccess = SetCommState(m_hComm, &dcb);  
    }  
          
    /**  清空串口缓冲区 */ 
    PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);  
 
    /** 离开临界段 */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return bIsSuccess==TRUE;  
}  
 
bool CSerialPort::InitPort( UINT portNo ,const LPDCB& plDCB )  
{  
    /** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */   
    if (!openPort(portNo))  
    {  
        return false;  
    }  
      
    /** 进入临界段 */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** 配置串口参数 */   
    if (!SetCommState(m_hComm, plDCB))  
    {  
        return false;  
    }  
 
    /**  清空串口缓冲区 */ 
    PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);  
 
    /** 离开临界段 */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return true;  
}  
 
void CSerialPort::ClosePort()  
{  
    /** 如果有串口被打开，关闭它 */ 
    if( m_hComm != INVALID_HANDLE_VALUE )  
    {  
        CloseHandle( m_hComm );  
        m_hComm = INVALID_HANDLE_VALUE;  
    }  
}  
 
bool CSerialPort::openPort( UINT portNo )  
{  
    /** 进入临界段 */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** 把串口的编号转换为设备名 */   
    char szPort[50];  
    sprintf_s(szPort, "COM%d", portNo);  
 
    /** 打开指定的串口 */   
    m_hComm = CreateFileA(szPort,  /** 设备名,COM1,COM2等 */   
              GENERIC_READ | GENERIC_WRITE, /** 访问模式,可同时读写 */     
              0,                            /** 共享模式,0表示不共享 */   
              NULL,                         /** 安全性设置,一般使用NULL */   
              OPEN_EXISTING,                /** 该参数表示设备必须存在,否则创建失败 */   
              0,      
              0);      
 
    /** 如果打开失败，释放资源并返回 */   
    if (m_hComm == INVALID_HANDLE_VALUE)  
    {  
        LeaveCriticalSection(&m_csCommunicationSync);  
        return false;  
    }  
 
    /** 退出临界区 */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return true;  
}  
 
bool CSerialPort::OpenListenThread()  
{  
    /** 检测线程是否已经开启了 */   
    if (m_hListenThread != INVALID_HANDLE_VALUE)  
    {  
        /** 线程已经开启 */   
        return false;  
    }  
 
    s_bExit = false;  
    /** 线程ID */   
    UINT threadId;  
    /** 开启串口数据监听线程 */   
    m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, &threadId);  
    if (!m_hListenThread)  
    {  
        return false;  
    }  
    /** 设置线程的优先级,高于普通线程 */   
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
        /** 通知线程退出 */   
        s_bExit = true;  
 
        /** 等待线程退出 */   
        Sleep(10);  
 
        /** 置线程句柄无效 */   
        CloseHandle( m_hListenThread );  
        m_hListenThread = INVALID_HANDLE_VALUE;  
    }  
    return true;  
}  
 
UINT CSerialPort::GetBytesInCOM()  
{  
    DWORD dwError = 0;  /** 错误码 */   
    COMSTAT  comstat;   /** COMSTAT结构体,记录通信设备的状态信息 */   
    memset(&comstat, 0, sizeof(COMSTAT));  
 
    UINT BytesInQue = 0;  
    /** 在调用ReadFile和WriteFile之前,通过本函数清除以前遗留的错误标志 */   
    if ( ClearCommError(m_hComm, &dwError, &comstat) )  
    {  
        BytesInQue = comstat.cbInQue; /** 获取在输入缓冲区中的字节数 */   
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
    /** 得到本类的指针 */   
    CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);  
 
    // 线程循环,轮询方式读取串口数据  
    while (!pSerialPort->s_bExit)   
    {  
        UINT BytesInQue = pSerialPort->GetBytesInCOM();  
        /** 如果串口输入缓冲区中无数据,则休息一会再查询 */   
        if ( BytesInQue == 0 )  
        {  
            Sleep(SLEEP_TIME_INTERVAL);  
            continue;  
        }  
 
        /** 读取输入缓冲区中的数据并输出显示 */ 
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
 
    /** 临界区保护 */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** 从缓冲区读取一个字节的数据 */   
    bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);  
    if ((!bResult))  
    {   
        /** 获取错误码,可以根据该错误码查出错误原因 */   
        DWORD dwError = GetLastError();  
 
        /** 清空串口缓冲区 */   
        PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);  
        LeaveCriticalSection(&m_csCommunicationSync);  
 
        return false;  
    }  
 
    /** 离开临界区 */   
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
 
    /** 临界区保护 */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** 向缓冲区写入指定量的数据 */   
    bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);  
    if (!bResult)    
    {  
        DWORD dwError = GetLastError();  
        /** 清空串口缓冲区 */   
        PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);  
        LeaveCriticalSection(&m_csCommunicationSync);  
 
        return false;  
    }  
 
    /** 离开临界区 */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return true;  
}
