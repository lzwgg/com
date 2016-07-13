
// serialCommDlg.cpp : implementation file
//

#include "stdafx.h"
#include "serialComm.h"
#include "serialCommDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CserialCommDlg dialog

UINT baud_sum[3] = {9600, 19200, 115200};
UINT data_bits[3] = {7, 8, 9};
UINT stops_bits[3] = {0, 1, 2};

void CserialCommDlg::SetBaud(void)
{
	m_Baud = baud_sum[m_Combo_Baud.GetCurSel()];
}

void CserialCommDlg::SetDataBit(void)
{
	m_Databits = data_bits[m_Combo_Databit.GetCurSel()];
}

void CserialCommDlg::SetStopBit(void)
{
	m_Stopsbits = stops_bits[m_Combo_Stopbit.GetCurSel()];
}

CserialCommDlg::CserialCommDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CserialCommDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CserialCommDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PORT, m_Combo_Port);
	DDX_Control(pDX, IDC_COMBO_BAUD, m_Combo_Baud);
	DDX_Control(pDX, IDC_COMBO_CHECK, m_Combo_Checkbit);
	DDX_Control(pDX, IDC_COMBO_DATA, m_Combo_Databit);
	DDX_Control(pDX, IDC_COMBO_STOP, m_Combo_Stopbit);
	DDX_Control(pDX, IDC_BTN_CONNECT, m_Btn_Connect);
	DDX_Control(pDX, IDC_EDIT_FILE, m_Edit_File);
	DDX_Control(pDX, IDC_CUSTOM_WAVE1, m_ChartWave1);
	DDX_Control(pDX, IDC_CUSTOM_WAVE2, m_ChartWave2);
	DDX_Control(pDX, IDC_CUSTOM_WAVE3, m_ChartWave3);
	DDX_Control(pDX, IDC_CUSTOM_WAVE4, m_ChartWave4);
}

BEGIN_MESSAGE_MAP(CserialCommDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CserialCommDlg::OnBnClickedBtnConnect)
	ON_CBN_SELCHANGE(IDC_COMBO_STOP, &CserialCommDlg::OnCbnSelchangeComboStop)
	ON_CBN_SELCHANGE(IDC_COMBO_DATA, &CserialCommDlg::OnCbnSelchangeComboData)
	ON_CBN_SELCHANGE(IDC_COMBO_CHECK, &CserialCommDlg::OnCbnSelchangeComboCheck)
	ON_CBN_SELCHANGE(IDC_COMBO_BAUD, &CserialCommDlg::OnCbnSelchangeComboBaud)
	ON_CBN_SELCHANGE(IDC_COMBO_PORT, &CserialCommDlg::OnCbnSelchangeComboPort)
	ON_BN_CLICKED(IDC_BTN_SAVE, &CserialCommDlg::OnBnClickedBtnSave)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDOK, &CserialCommDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BTN_SEND, &CserialCommDlg::OnBnClickedBtnSend)
END_MESSAGE_MAP()


// CserialCommDlg message handlers

BOOL CserialCommDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	UINT iter;
	TCHAR temp[32] = _T("");

	// add port 1 ~ 9
	for (iter = 1; iter < 10; iter++)
	{
		memset(temp, 0, 32);
		swprintf_s(temp, 32, _T("COM%d"), iter);
		if (1 == iter)
			m_Combo_Port.AddString(temp);
		else
			m_Combo_Port.InsertString(iter - 1, temp);
	}
	m_Combo_Port.SetCurSel(3);

	// add baud rate
	for (iter = 0; iter < 3; iter++)
	{
		memset(temp, 0, 32);
		swprintf_s(temp, 32, _T("%d"), baud_sum[iter]);
		if (!iter)
			m_Combo_Baud.AddString(temp);
		else
			m_Combo_Baud.InsertString(iter, temp);
	}
	m_Combo_Baud.SetCurSel(2);

	// add check bit
	m_Combo_Checkbit.AddString(_T("No"));
	m_Combo_Checkbit.InsertString(1, _T("Yes"));
	m_Combo_Checkbit.SetCurSel(0);

	// add data bit
	for (iter = 0; iter < 3; iter++)
	{
		memset(temp, 0, 32);
		swprintf_s(temp, 32, _T("%d"), data_bits[iter]);
		if (!iter)
			m_Combo_Databit.AddString(temp);
		else
			m_Combo_Databit.InsertString(iter, temp);
	}
	m_Combo_Databit.SetCurSel(1);

	// add stop bit
	for (iter = 0; iter < 3; iter++)
	{
		memset(temp, 0, 32);
		swprintf_s(temp, 32, _T("%d"), stops_bits[iter]);
		if (!iter)
			m_Combo_Stopbit.AddString(temp);
		else
			m_Combo_Stopbit.InsertString(iter, temp);
	}
	m_Combo_Stopbit.SetCurSel(1);

	// set current parameter
	SetPort();
	SetParity();
	SetBaud();
	SetDataBit();
	SetStopBit();

	// add limit characters to edit box
	m_Edit_File.SetLimitText(7);
	m_Edit_File.SetWindowText(_T("1000001"));

	// make the coordinate diagram
	CChartAxis *pAxis = NULL;
	pAxis = m_ChartWave1.CreateStandardAxis(CChartCtrl::BottomAxis);
	pAxis->SetAutomatic(true);
	pAxis = m_ChartWave1.CreateStandardAxis(CChartCtrl::LeftAxis);
	pAxis->SetAutomatic(true);
	pAxis = m_ChartWave2.CreateStandardAxis(CChartCtrl::BottomAxis);
	pAxis->SetAutomatic(true);
	pAxis = m_ChartWave2.CreateStandardAxis(CChartCtrl::LeftAxis);
	pAxis->SetAutomatic(true);
	pAxis = m_ChartWave3.CreateStandardAxis(CChartCtrl::BottomAxis);
	pAxis->SetAutomatic(true);
	pAxis = m_ChartWave3.CreateStandardAxis(CChartCtrl::LeftAxis);
	pAxis->SetAutomatic(true);
	pAxis = m_ChartWave4.CreateStandardAxis(CChartCtrl::BottomAxis);
	pAxis->SetAutomatic(true);
	pAxis = m_ChartWave4.CreateStandardAxis(CChartCtrl::LeftAxis);
	pAxis->SetAutomatic(true);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CserialCommDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CserialCommDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CserialCommDlg::OnBnClickedBtnConnect()
{
	// TODO: Add your control notification handler code here
	CString name;
	GetDlgItem(IDC_BTN_CONNECT)->GetWindowText(name);

	if (!name.Compare(_T("连接")))
	{
		if(!mySerialPort.InitPort(m_Port, m_Baud, m_Parity, m_Databits, m_Stopsbits))
		{
			MessageBox(_T("初始化串口号失败！"));
			return;
		}
		if (!mySerialPort.OpenListenThread())
		{
			MessageBox(_T("监听串口失败！"));
			mySerialPort.ClosePort();
			return;
		}
		GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("取消"));
		GetDlgItem(IDC_EDIT_DEAD)->SetWindowText(_T("已连接"));
		SetTimer(1, 1000, NULL);
		GetDlgItem(IDC_BTN_SAVE)->EnableWindow(FALSE);
	}
	else if (!name.Compare(_T("取消")))
	{
		mySerialPort.CloseListenTread();
		mySerialPort.ClosePort();
		GetDlgItem(IDC_BTN_CONNECT)->SetWindowText(_T("连接"));
		GetDlgItem(IDC_EDIT_DEAD)->SetWindowText(_T("未连接"));
	}
}

void CserialCommDlg::OnCbnSelchangeComboStop()
{
	// TODO: Add your control notification handler code here
	SetStopBit();
}

void CserialCommDlg::OnCbnSelchangeComboData()
{
	// TODO: Add your control notification handler code here
	SetDataBit();
}

void CserialCommDlg::OnCbnSelchangeComboCheck()
{
	// TODO: Add your control notification handler code here
	SetParity();
}

void CserialCommDlg::OnCbnSelchangeComboBaud()
{
	// TODO: Add your control notification handler code here
	SetBaud();
}


void CserialCommDlg::OnCbnSelchangeComboPort()
{
	// TODO: Add your control notification handler code here
	SetPort();
}

extern void InitMonitorData(void);

void CserialCommDlg::OnBnClickedBtnSave()
{
	// TODO: Add your control notification handler code here
	CString fileName;
	m_Edit_File.GetWindowText(fileName);
	if (fileName.IsEmpty())
	{
		MessageBox(_T("请添加文件名！"));
		return;
	}
	// if it's the first time using file
	if (!m_File_Name.IsEmpty())
	{
		// the file not changed
		if (!m_File_Name.Compare(fileName))	return;
		// if changed, close the opening file and change the file name
		if (mySerialPort.fildes)
			fclose(mySerialPort.fildes);
	}
	m_File_Name = fileName;
	if (mySerialPort.fildes)
	{
		fclose(mySerialPort.fildes);
	}
	fileName.Append(_T(".txt"));
	_wfopen_s(&mySerialPort.fildes, fileName, _T("w+"));
	if (!mySerialPort.fildes)
	{
		MessageBox(_T("文件打开出错！"));
		return;
	}
	mySerialPort.useFileFlag = 1;
	GetDlgItem(IDC_BTN_SAVE)->EnableWindow(FALSE);
}

void CserialCommDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	// call the function to repaint the diagram
	ShowDiagramData(m_ChartWave1, 0);
	ShowDiagramData(m_ChartWave2, 1);
	ShowDiagramData(m_ChartWave3, 2);
	ShowDiagramData(m_ChartWave4, 3);
	//MessageBox(_T("hello"));
	CDialog::OnTimer(nIDEvent);
}

void CserialCommDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	//OnOK();
	//SetTimer(1, 200, NULL);
}

extern double monitorData[4][50];
extern UINT monitorCount;

void CserialCommDlg::ShowDiagramData(CChartCtrl &ChartR, UINT markFlag)
{
	ChartR.EnableRefresh( false ); 

	double x[50], y[50];
	UINT i;

	// get the diagram data

	if (!monitorCount)
	{
		// if no data, then return
		return;
	}

	for ( i = 0 ; i < monitorCount ; i ++ )
	{ 
		x[i] = i * monitorCount;
		y[i] = monitorData[markFlag][i];
	}

	// draw the diagram
	CChartLineSerie * pLineSerie1;
	ChartR.RemoveAllSeries();
	pLineSerie1 = ChartR.CreateLineSerie();
	pLineSerie1->SetColor(RGB(0, 0, 255));
	pLineSerie1->SetSeriesOrdering(poNoOrdering);
	pLineSerie1->AddPoints(x, y, monitorCount);

	ChartR.EnableRefresh( true );
}

// for data Belong [0-15]
char intToChar (int data)
{
	// transfer to 0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F
	if (data < 10)
	{
		return data + '0';
	}
	return data - 10 + 'A';
}

// for data Belong [0-255]

int DecimalToHexString(int data, char *pdata)
{
	int m, n, pos, temp;
	char cdata[4] = "\0";
	m = data;
	n = 16;
	pos = 0;
	if (!m)
		return 0;
	while (m && pos < 3)
	{
		temp = (m >> 4);
		if (!temp)
		{
			temp = m % n;
			if (!pos)
			{
				cdata[pos++] = intToChar(0); // [1-15]
			}
			cdata[pos++] = intToChar(temp);
		}
		else
			cdata[pos++] = intToChar(temp);
		m = m >> 4;
	}
	strncpy(pdata, cdata, 2);
	return 0;
}

void CserialCommDlg::OnBnClickedBtnSend()
{
	// TODO: Add your control notification handler code here
	CString data;
	char cdata[16] = "\0";
	int idata, len;
	GetDlgItem(IDC_EDIT_SEND)->GetWindowText(data);
	len = data.GetLength();
	if (len > 3)
	{
		MessageBox(_T("数太大"), 0, 0);
		return;
	}
	else if (!len)
	{
		MessageBox(_T("未输入数据"), 0, 0);
		return;
	}
	idata = _ttoi(data);
	if (idata > 255)
	{
		MessageBox(_T("数太大"), 0, 0);
		return;
	}
	//DecimalToHexString(idata, cdata);
	cdata[0] = idata;
	//sprintf_s(cdata, 16, "%d", idata);
	if (mySerialPort.WriteData((unsigned char*)cdata, 1) == true)
	{
		GetDlgItem(IDC_EDIT_DEAD2)->SetWindowText(_T("发送成功"));
	}
	else
	{
		GetDlgItem(IDC_EDIT_DEAD2)->SetWindowText(_T("发送失败"));
	}
}
