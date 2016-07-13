
// serialCommDlg.h : header file
//

#pragma once

#include "SerialPort.h"
#include "afxwin.h"
#include "d:\programme\serialcomm\serialcomm\serialcomm\chartclass\chartctrl.h"
#include "afxcmn.h"

// CserialCommDlg dialog
class CserialCommDlg : public CDialog
{
// Construction
public:
	CserialCommDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_SERIALCOMM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnConnect();
	void SetPort(void)	{m_Port = m_Combo_Port.GetCurSel() + 1; }
	void SetParity(void) {m_Parity = (m_Combo_Checkbit.GetCurSel())?'Y':'N'; }
	void SetBaud(void);
	void SetDataBit(void);
	void SetStopBit(void);
	CSerialPort mySerialPort;
	char m_Parity;
	UINT m_Port, m_Baud, m_Databits, m_Stopsbits;
	CComboBox m_Combo_Port;
	CComboBox m_Combo_Baud;
	CComboBox m_Combo_Checkbit;
	CComboBox m_Combo_Databit;
	CComboBox m_Combo_Stopbit;
	CButton m_Btn_Connect;
	afx_msg void OnCbnSelchangeComboStop();
	afx_msg void OnCbnSelchangeComboData();
	afx_msg void OnCbnSelchangeComboCheck();
	afx_msg void OnCbnSelchangeComboBaud();
	afx_msg void OnCbnSelchangeComboPort();
	CEdit m_Edit_File;
	CString m_File_Name;
	afx_msg void OnBnClickedBtnSave();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedOk();
	void ShowDiagramData(CChartCtrl &ChartR, UINT markFlag);
	CChartCtrl m_ChartWave1;
	CChartCtrl m_ChartWave2;
	CChartCtrl m_ChartWave3;
	CChartCtrl m_ChartWave4;
	afx_msg void OnBnClickedBtnSend();
};
