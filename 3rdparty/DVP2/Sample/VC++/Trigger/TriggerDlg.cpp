// TriggerDlg.cpp :  Implementation file

#include "stdafx.h"
#include "Trigger.h"
#include "TriggerDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// DVP API rely on. 
#ifdef _M_X64
#pragma comment(lib, "../src/lib/x64/DVPCamera64.lib")
#else
#pragma comment(lib, "../src/lib/x86/DVPCamera.lib")
#endif

// CTriggerDlg dialog

// Callback function for acquiring the video stream.
int CTriggerDlg::OnDrawPicture(dvpHandle handle,
	dvpStreamEvent event,
	void* pContext,
	dvpFrame* pFrame,
	void* pBuffer)
{
	CTriggerDlg* pDlg = ((CTriggerDlg*)pContext);


	// Get current clock
	pDlg->m_CurGrabClock = GetTickCount();

	bool bDisplay = false;
	if (pDlg->m_dfDisplayCount != 0)
	{
		// count the time of image acquisition
		unsigned int GrabMs = (pDlg->m_CurGrabClock - pDlg->m_StartGrabClock);

		// Calculate whether the current frame is displayed
		if (GrabMs - pDlg->m_dfDisplayCount * 33.3f >= 33)
		{
			bDisplay = true;
		}
	}
	else
	{
		bDisplay = true;
		pDlg->m_StartGrabClock = pDlg->m_CurGrabClock;
	}

	if (bDisplay || pDlg->m_bTriggerMode)
	{
		// It demonstrates the usual video drawing,and it is not recommended to take a longer time operation in the callback function,
		// in order to avoid affecting the frame rate and the real-time of acquiring images.
		// The acquired image data buffer is valid only before the function returns,so the buffer pointer should not be passed out, 
		// however, the user can malloc memory and copy image data.
		dvpStatus status = dvpDrawPicture(pFrame, pBuffer, pDlg->GetDlgItem(IDC_STATIC_PREVIEW)->GetSafeHwnd(), nullptr, nullptr);
		ASSERT(status == DVP_STATUS_OK);
	}

	// "return 0" represents the image data has been discarded and no longer been used.
	return 0;
}


CTriggerDlg::CTriggerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTriggerDlg::IDD, pParent)
	, m_TriggerDelay(0)
	, m_TriggerFilter(0)
	, m_TriggerLoop(0)
{
	m_handle = 0;
	dvpDoubleDescr descrd = { 0, };
	m_DelayDescr = m_FilterDescr = m_LoopDescr = descrd;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}


void CTriggerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_INPUT_IO, m_InputIo);
	DDX_Control(pDX, IDC_COMBO_TRIGGER_INPUT_TYPE, m_TriggerInputType);
	DDX_Control(pDX, IDC_COMBO_STROBE_OUTPUT_TYPE, m_StrobeOutputType);
	DDX_Control(pDX, IDC_COMBO_OUTPUT_IO, m_OutputIo);
	DDX_Text(pDX, IDC_EDIT_TRIGGER_DELAY, m_TriggerDelay);
	DDV_MinMaxDouble(pDX, m_TriggerDelay, m_DelayDescr.fMin, m_DelayDescr.fMax);
	DDX_Text(pDX, IDC_EDIT_TRIGGER_FILTER, m_TriggerFilter);
	DDV_MinMaxDouble(pDX, m_TriggerFilter, m_FilterDescr.fMin, m_FilterDescr.fMax);
	DDX_Text(pDX, IDC_EDIT_TRIGGER_LOOP, m_TriggerLoop);
	DDV_MinMaxDouble(pDX, m_TriggerLoop, m_LoopDescr.fMin, m_LoopDescr.fMax);
	DDX_Control(pDX, IDC_COMBO_DEVICES, m_listDevices);
}


void CTriggerDlg::UpdateControls()
{
	dvpStatus status  = DVP_STATUS_UNKNOW;

	if (IsValidHandle(m_handle))
	{
		// The device has been opened at this time.
		// Update and enable the basic controls.
		c'c
		UpdateData(FALSE);
	}
	else
	{
		// No device is opened at this time.
		// Update the basic controls.
		GetDlgItem(IDC_BUTTON_OPEN)->SetWindowTextW(_T("Open"));
		GetDlgItem(IDC_BUTTON_PLAY)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_PROPERTY)->EnableWindow(FALSE);

		if (m_listDevices.GetCount() == 0)
		{
			// No device exists.
			GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(FALSE);
		}
		else
		{
			GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(TRUE);
		}

		// Update the related controls.
		GetDlgItem(IDC_COMBO_INPUT_IO)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_TRIGGER_INPUT_TYPE)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_STROBE_OUTPUT_TYPE)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_OUTPUT_IO)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_TRIGGER_FILTER)->EnableWindow(FALSE);

		GetDlgItem(IDC_STATIC_LOOP)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_TRIGGER_LOOP)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_TRIGGER)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_LOOP)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_TRIG_IN)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_STROBE_OUT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_TRIGGER_FIRE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_APPLY_LOOP)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_APPLY_FILTER)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_PREVIEW)->InvalidateRect(NULL);

		GetDlgItem(IDC_BUTTON_APPLY_DELAY)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_TRIGGER_DELAY)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_DELAY)->EnableWindow(FALSE);
	}

	SendMessage(WM_SIZE);
}


BEGIN_MESSAGE_MAP(CTriggerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CTriggerDlg::OnBnClickedButtonOpen)
	ON_CBN_SELCHANGE(IDC_COMBO_INPUT_IO, &CTriggerDlg::OnCbnSelchangeComboInputIo)
	ON_CBN_SELCHANGE(IDC_COMBO_TRIGGER_INPUT_TYPE, &CTriggerDlg::OnCbnSelchangeComboTriggerInputType)
	ON_BN_CLICKED(IDC_BUTTON_PROPERTY, &CTriggerDlg::OnBnClickedButtonProperty)
	ON_CBN_SELCHANGE(IDC_COMBO_OUTPUT_IO, &CTriggerDlg::OnCbnSelchangeComboOutputIo)
	ON_BN_CLICKED(IDC_BUTTON_TRIGGER_FIRE, &CTriggerDlg::OnBnClickedButtonTriggerFire)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CTriggerDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_CHECK_LOOP, &CTriggerDlg::OnBnClickedCheckLoop)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CHECK_TRIGGER, &CTriggerDlg::OnBnClickedCheckTrigger)
	ON_CBN_SELCHANGE(IDC_COMBO_STROBE_OUTPUT_TYPE, &CTriggerDlg::OnCbnSelchangeComboStrobeOutputType)
	ON_BN_CLICKED(IDC_BUTTON_SCAN, &CTriggerDlg::OnBnClickedButtonScan)
	ON_BN_CLICKED(IDC_BUTTON_APPLY_LOOP, &CTriggerDlg::OnBnClickedButtonApplyLoop)
	ON_BN_CLICKED(IDC_BUTTON_APPLY_DELAY, &CTriggerDlg::OnBnClickedButtonApplyDelay)
	ON_BN_CLICKED(IDC_CHECK_TRIG_IN, &CTriggerDlg::OnBnClickedCheckTrigIn)
	ON_BN_CLICKED(IDC_CHECK_STROBE_OUT, &CTriggerDlg::OnBnClickedCheckStrobeOut)
	ON_BN_CLICKED(IDC_BUTTON_APPLY_FILTER, &CTriggerDlg::OnBnClickedButtonApplyFilter)
	ON_BN_CLICKED(IDC_CHECK_USERDEFINEDNAME, &CTriggerDlg::OnBnClickedCheckUserdefinedname)
END_MESSAGE_MAP()


BOOL CTriggerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon.
	SetIcon(m_hIcon, FALSE);		// Set small icon.

	// Initialize open mode
	// false: user dvpOpenByName open the camear
	// true : user dvpOpenByUserId open the camear
	m_bUserDefinedName = false;
	m_dfDisplayCount = 0;
	CheckDlgButton(IDC_CHECK_USERDEFINEDNAME, BST_UNCHECKED);

	// Scan devices that have been connected to a computer in advance. 
	OnBnClickedButtonScan();

	// Timer for updating the current information of the frame rate.
	SetTimer(0, 500, NULL);
	return TRUE;
}


void CTriggerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}


HCURSOR CTriggerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CTriggerDlg::OnBnClickedButtonTriggerFire()
{
	if (IsValidHandle(m_handle))
	{
		// Once execution of this function is equivalent to the generation of an external trigger.
		// Note:If the exposure time is very long, clicking on the "Soft Trigger the Fire" too quick may cause the failure of the trigger,
		// because the previous frame probably in a state of exposing continuously or output incompletely.
		dvpStatus status = dvpTriggerFire(m_handle);
	}
}


void CTriggerDlg::OnOK()
{
	if (IsValidHandle(m_handle))
	{
		dvpStatus status = dvpSaveConfig(m_handle, NULL);
		ASSERT(status == DVP_STATUS_OK);
	}

	CDialogEx::OnOK();
}


void CTriggerDlg::OnBnClickedCheckTrigger()
{
	if (IsValidHandle(m_handle))
	{
		dvpStatus status;
		dvpStreamState state;
		status = dvpGetStreamState(m_handle, &state);
		ASSERT(status == DVP_STATUS_OK);

		if (state == STATE_STARTED)
		{
			// Stop the video stream.
			status = dvpStop(m_handle);
			ASSERT(status == DVP_STATUS_OK);
		}

		// Enable/disable the trigger mode.
		status = dvpSetTriggerState(m_handle, ((CButton*)GetDlgItem(IDC_CHECK_TRIGGER))->GetCheck() ? true : false);
		ASSERT(status == DVP_STATUS_OK);

		if (state == STATE_STARTED)
		{
			// Start the video stream.
			status = dvpStart(m_handle);
			ASSERT(status == DVP_STATUS_OK);
		}

		UpdateControls();
	}
}


void CTriggerDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// Close the device at the moment of  destroying the window.
	if (IsValidHandle(m_handle))
	{
		dvpStatus status = dvpStop(m_handle);
		ASSERT(status == DVP_STATUS_OK);

		status = dvpClose(m_handle);
		ASSERT(status == DVP_STATUS_OK);
		m_handle = 0;
	}

}


void CTriggerDlg::OnTimer(UINT_PTR nIDEvent)
{

	if (nIDEvent == 0)
	{
		if (IsValidHandle(m_handle))
		{
			// Update the information of the frame rate.
			dvpFrameCount count;
			dvpStatus status = dvpGetFrameCount(m_handle, &count);
			ASSERT(status == DVP_STATUS_OK);

			CString str;
			if (m_dfDisplayCount > 0 && IsDlgButtonChecked(IDC_CHECK_TRIGGER) == BST_UNCHECKED)
			{
				str.Format(_T("%s [%d frames, %.3f fps, Display %.3f fps]"),
					m_strFriendlyName,
					count.uFrameCount,
					count.fFrameRate,
					m_dfDisplayCount * 1000.0 / (m_CurGrabClock - m_StartGrabClock));
			}
			else
			{
				str.Format(_T("%s [%d frames, %.3f fps]"),
					m_strFriendlyName,
					count.uFrameCount,
					count.fFrameRate);
			}
			SetWindowText(str);
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CTriggerDlg::OnBnClickedCheckLoop()
{
	bool bLoop = ((CButton*)GetDlgItem(IDC_CHECK_LOOP))->GetCheck() ? true : false;

	dvpStatus status = dvpSetSoftTriggerLoopState(m_handle, bLoop);
	ASSERT(status == DVP_STATUS_OK);
	UpdateControls();
}


void CTriggerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// Adjust the video graphic window automatically according to the size of the window.
	UpdataPreWindowSize(cx, cy);
}


void CTriggerDlg::UpdataPreWindowSize(int x, int y)
{
	if (IsValidHandle(m_handle))
	{
		dvpRegion roi;
		dvpStatus status = dvpGetRoi(m_handle, &roi);

		CRect rc, rcPreview, rcOld;
		GetDlgItem(IDC_STATIC_PREVIEW)->GetWindowRect(rcOld);
		ScreenToClient(rcOld);

		GetClientRect(rc);
		rc.left += rcOld.left;
		rc.right -= 6;
		rc.top += rcOld.top;
		rc.bottom -= 6;

		rcPreview.SetRect(0, 0, roi.W, roi.H);

		if (rc.Width() * rcPreview.Height() >
			rc.Height() * rcPreview.Width())
		{
			rcPreview.SetRect(
				0,
				0,
				rc.Height() * roi.W / roi.H,
				rc.Height());
		}
		else
		{
			rcPreview.SetRect(
				0,
				0,
				rc.Width(),
				rc.Width() * roi.H / roi.W);
		}

		rcPreview.MoveToXY(rc.TopLeft());

		GetDlgItem(IDC_STATIC_PREVIEW)->MoveWindow(rcPreview);
	}
}


void CTriggerDlg::OnBnClickedButtonScan()
{
	dvpStatus status;
	dvpUint32 i, n = 0;

	// "n" represents the number of cameras that is enumerated successfully, drop-down list contains each camera's FriendlyName.
	m_listDevices.ResetContent();

	// Get the number of cameras that has been connected to a computer.
	status = dvpRefresh(&n);
	ASSERT(status == DVP_STATUS_OK);
	m_CamCount = 0;
	if (status == DVP_STATUS_OK)
	{
		// Enumeration of up to 16 cameras.
		if (n > 16)
			n = 16;


		memset(m_info, 0, sizeof(m_info));
		for (i = 0; i < n; i++)
		{
			// Acquire each camera's information one by one.
			status = dvpEnum(i, &m_info[i]);
			ASSERT(status == DVP_STATUS_OK);

			if (status == DVP_STATUS_OK)
			{
				// GUI need UNICODE,but the information acquired from cameras is ANSI,so convert the character set from ANSI to UNICODE.
				int item = CB_ERR;
				if (!m_bUserDefinedName)
				{
					item = m_listDevices.AddString(CString(m_info[m_CamCount].FriendlyName));
				}
				else
				{
					// check User Defined Name is null
					if (strlen(m_info[m_CamCount].UserID) == 0)
						continue;
					item = m_listDevices.AddString(CString(m_info[m_CamCount].UserID));
				}
				if (item != CB_ERR)
				{
					m_listDevices.SetItemData(item, m_CamCount);
				}
				m_CamCount++;
			}
		}

		if (i != 0)
		{
			m_listDevices.SetCurSel(0);
		}

		UpdateControls();
	}
}


void CTriggerDlg::OnBnClickedButtonOpen()
{
	dvpStatus status = DVP_STATUS_OK;
	CString strName;

	if (!IsValidHandle(m_handle))
	{
		m_listDevices.GetWindowText(strName);
		if (strName != "")
		{
			// Open the pointed device by the selected camera.

			if (m_bUserDefinedName)
			{
				status = dvpOpenByUserId(CStringA(strName), OPEN_NORMAL, &m_handle);
			}
			else
			{
				status = dvpOpenByName(CStringA(strName), OPEN_NORMAL, &m_handle);
			}
			ASSERT(status == DVP_STATUS_OK);

			if (status != DVP_STATUS_OK)
			{
				MessageBox(_T("Open the device failed"));
			}
			else
			{
				m_strFriendlyName = strName;

				// If it needs to display images ,the user should register a callback function and finish the operation of drawing pictures in the registered callback function.
				// Note: Drawing pictures in the callback function maybe generate some delays for acquiring image data by the use of "dvpGetFrame".
				status = dvpRegisterStreamCallback(m_handle, OnDrawPicture, STREAM_EVENT_PROCESSED, this);
				ASSERT(status == DVP_STATUS_OK);
			}
		}
	}
	else
	{
		dvpStreamState state;
		dvpGetStreamState(m_handle, &state);
		if (state == STATE_STARTED)
		{
			m_dfDisplayCount = 0;
			status = dvpStop(m_handle);
			ASSERT(status == DVP_STATUS_OK);

		}

		status = dvpClose(m_handle);
		ASSERT(status == DVP_STATUS_OK);
		m_handle = 0;
		GetDlgItem(IDC_STATIC_PREVIEW)->InvalidateRect(NULL, 1);
	}

	UpdateControls();
}


void CTriggerDlg::OnBnClickedButtonPlay()
{
	if (IsValidHandle(m_handle))
	{
		dvpStreamState state;
		dvpStatus status;

		// Implement a button to start and stop according to the current video's status.
		status = dvpGetStreamState(m_handle, &state);
		ASSERT(status == DVP_STATUS_OK);

		if (state == STATE_STARTED)
		{

			status = dvpStop(m_handle);

		}
		else
		{
			// init display count
			m_dfDisplayCount = 0;

			status = dvpStart(m_handle);

		}

		ASSERT(status == DVP_STATUS_OK);
	}

	UpdateControls();
}


void CTriggerDlg::OnBnClickedButtonProperty()
{
	if (IsValidHandle(m_handle))
	{
		dvpStatus status = dvpShowPropertyModalDialog(m_handle, GetSafeHwnd());

		// At this time some configurations may change, synchronize it to the window GUI.
		UpdateControls();
		RECT rt = { 0 };
		GetClientRect(&rt);
		UpdataPreWindowSize(rt.right, rt.bottom);
	}
}


void CTriggerDlg::OnBnClickedButtonApplyLoop()
{
	if (IsValidHandle(m_handle))
	{
		UpdateData();

		dvpStatus status;

		if (m_TriggerLoop < m_LoopDescr.fMin)
			m_TriggerLoop = m_LoopDescr.fMin;
		if (m_TriggerLoop > m_LoopDescr.fMax)
			m_TriggerLoop = m_LoopDescr.fMax;

		status = dvpSetSoftTriggerLoop(m_handle, m_TriggerLoop);
		ASSERT(status == DVP_STATUS_OK);

		status = dvpGetSoftTriggerLoop(m_handle, &m_TriggerLoop);
		if (status == DVP_STATUS_OK)
		{
			TCHAR szBuf[128] = { 0 };
			_stprintf_s(szBuf, TEXT("%.0f"), m_TriggerLoop);
			SetDlgItemText(IDC_EDIT_TRIGGER_LOOP, szBuf);
		}
	}
}


void CTriggerDlg::OnBnClickedButtonApplyDelay()
{
	if (IsValidHandle(m_handle))
	{
		UpdateData();

		dvpStatus status;

		if (m_TriggerDelay < m_DelayDescr.fMin)
			m_TriggerDelay = m_DelayDescr.fMin;
		if (m_TriggerDelay > m_DelayDescr.fMax)
			m_TriggerDelay = m_DelayDescr.fMax;

		status = dvpSetTriggerDelay(m_handle, m_TriggerDelay);
		ASSERT(status == DVP_STATUS_OK);

		status = dvpGetTriggerDelay(m_handle, &m_TriggerDelay);
		if (status == DVP_STATUS_OK)
		{
			TCHAR szBuf[128] = { 0 };
			_stprintf_s(szBuf, TEXT("%.0f"), m_TriggerDelay);
			SetDlgItemText(IDC_EDIT_TRIGGER_DELAY, szBuf);
		}
	}
}


void CTriggerDlg::OnBnClickedCheckTrigIn()
{
	if (IsValidHandle(m_handle))
	{
		dvpUserIoInfo UserIoInfo;
		dvpUint32 i;
		dvpStatus status;

		status = dvpGetUserIoInfo(m_handle, &UserIoInfo);
		ASSERT(status == DVP_STATUS_OK);

		//选择外部触发功能
		// If ExtTriggerIn is enable,set the Input Io as the trigger input function. 
		if (((CButton*)GetDlgItem(IDC_CHECK_TRIG_IN))->GetCheck()) {
			int j = m_InputIo.GetCurSel();
			dvpInputIo InputIo;
			if (j >= 0) {
				InputIo = dvpInputIo(m_InputIo.GetItemData(j));
				//If the stream is already on,you need to stop stream before you set it and turn it on.
				status = dvpSetInputIoFunction(m_handle, InputIo, INPUT_FUNCTION_TRIGGER);
				ASSERT(status == DVP_STATUS_OK);
			}
		}
		else {
			//设置普通的输入模式
			// Set all INPUT IO as the Normal mode
			for (i = 0; i < 8; i++) {
				if (UserIoInfo.inputValid[i]) {
					//Line1 -->  Input IO
					//Line2 -->  Output IO
					status = dvpSetInputIoFunction(m_handle, dvpInputIo(INPUT_IO_1 + i), INPUT_FUNCTION_NORMAL);
					//ASSERT(status == DVP_STATUS_OK);
				}
			}
		}
	}
	UpdateControls();
}


void CTriggerDlg::OnBnClickedCheckStrobeOut()
{
	if (IsValidHandle(m_handle))
	{
		dvpUserIoInfo UserIoInfo;
		dvpUint32 i;
		dvpStatus status;

		status = dvpGetUserIoInfo(m_handle, &UserIoInfo);
		ASSERT(status == DVP_STATUS_OK);

		// If ExtStrobeOut is enable,set the Output Io as the strobe output function. 
		if (((CButton*)GetDlgItem(IDC_CHECK_STROBE_OUT))->GetCheck()) {
			int j = m_OutputIo.GetCurSel();
			dvpOutputIo OutputIo;
			if (j >= 0) {
				OutputIo = dvpOutputIo(m_OutputIo.GetItemData(j));
				status = dvpSetOutputIoFunction(m_handle, OutputIo, OUTPUT_FUNCTION_STROBE);
				ASSERT(status == DVP_STATUS_OK);
			}
		}
		else {
			// Set all OUTPUT IO as the Normal mode  
			for (i = 0; i < 8; i++) {
				if (UserIoInfo.outputValid[i]) {
					status = dvpSetOutputIoFunction(m_handle, dvpOutputIo(OUTPUT_IO_1 + i), OUTPUT_FUNCTION_NORMAL);
					ASSERT(status == DVP_STATUS_OK);
				}
			}
		}
	}
	UpdateControls();
}


void CTriggerDlg::OnCbnSelchangeComboInputIo()
{
	if (IsValidHandle(m_handle))
	{
		// Changing the trigger input IO and enabling the trigger input function is the same process.
		OnBnClickedCheckTrigIn();
	}
}


//修改闪光灯输出的输出io口
void CTriggerDlg::OnCbnSelchangeComboOutputIo()
{
	if (IsValidHandle(m_handle))
	{
		// Changing the strobe output IO and enabling the strobe output function is the same process.
		OnBnClickedCheckStrobeOut();
	}
}



//修改触发输入的类型
void CTriggerDlg::OnCbnSelchangeComboTriggerInputType()
{
	if (IsValidHandle(m_handle))
	{
		dvpTriggerInputType TriggerType = (dvpTriggerInputType)(TRIGGER_IN_OFF + m_TriggerInputType.GetCurSel());
		dvpStatus status = dvpSetTriggerInputType(m_handle, TriggerType);
		ASSERT(status == DVP_STATUS_OK);
	}
}



//修改闪光灯输出的类型
void CTriggerDlg::OnCbnSelchangeComboStrobeOutputType()
{
	if (IsValidHandle(m_handle))
	{
		dvpStrobeOutputType StrobeType = (dvpStrobeOutputType)(STROBE_OUT_OFF + m_StrobeOutputType.GetCurSel());
		dvpStatus status = dvpSetStrobeOutputType(m_handle, StrobeType);
		ASSERT(status == DVP_STATUS_OK);
	}
}




//应用  设置触发过滤filter
void CTriggerDlg::OnBnClickedButtonApplyFilter()
{
	if (IsValidHandle(m_handle))
	{
		UpdateData();

		dvpStatus status;

		//触发过滤有一个设置的范围，当小于最小值或者大于最大值，就按照触发过滤的最小值到最大值算。

		if (m_TriggerFilter < m_FilterDescr.fMin)
			m_TriggerFilter = m_FilterDescr.fMin;
		if (m_TriggerFilter > m_FilterDescr.fMax)
			m_TriggerFilter = m_FilterDescr.fMax;
		//set设置触发过滤
		status = dvpSetTriggerJitterFilter(m_handle, m_TriggerFilter);
		ASSERT(status == DVP_STATUS_OK);

		status = dvpGetTriggerJitterFilter(m_handle, &m_TriggerFilter);
		if (status == DVP_STATUS_OK)
		{
			SetDlgItemInt(IDC_EDIT_TRIGGER_FILTER, (int)m_TriggerFilter);
			
		}
	}
}



//点击，选择友好名称事件
void CTriggerDlg::OnBnClickedCheckUserdefinedname()
{
	if (m_bUserDefinedName == (IsDlgButtonChecked(IDC_CHECK_USERDEFINEDNAME) == BST_CHECKED ? true : false))
	{
		return;
	}

	m_bUserDefinedName = !m_bUserDefinedName;

	// save cur sel item
	CString strName;
	m_listDevices.GetWindowText(strName);

	// reset m_listDevices values
	m_listDevices.ResetContent();
	for (int i = 0; i < m_CamCount; i++)
	{
		int item = CB_ERR;
		if (!m_bUserDefinedName)
		{
			item = m_listDevices.AddString(CString(m_info[i].FriendlyName));
			if (strName == CString(m_info[i].UserID))
			{
				m_listDevices.SetCurSel(item);
			}
		}
		else
		{
			// check User Defined Name is null
			if (strlen(m_info[i].UserID) == 0)
				continue;

			item = m_listDevices.AddString(CString(m_info[i].UserID));
			if (strName == CString(m_info[i].FriendlyName))
			{
				m_listDevices.SetCurSel(item);
			}
		}
		if (item != CB_ERR)
		{
			m_listDevices.SetItemData(item, i);
		}
	}
}
