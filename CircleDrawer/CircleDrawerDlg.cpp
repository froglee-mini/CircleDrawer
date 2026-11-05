
// CircleDrawerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "CircleDrawer.h"
#include "CircleDrawerDlg.h"
#include "afxdialogex.h"

#include <cmath>
#include <atlimage.h>
#include <vector>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CString FindSolutionFolder()
{
	TCHAR buffer[MAX_PATH] = {};

	if (::GetModuleFileName(
		nullptr, buffer, MAX_PATH) == 0)
		return _T("");

	CString folder(buffer);

	// 실행 파일 이름 제거
	int pos = folder.ReverseFind(_T('\\'));
	if (pos < 0)
		return _T("");

	folder = folder.Left(pos);

	// 상위 폴더를 순서대로 검색
	while (!folder.IsEmpty())
	{
		WIN32_FIND_DATA data = {};

		CString pattern = folder + _T("\\*.sln");
		HANDLE hFind = ::FindFirstFile(pattern, &data);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			::FindClose(hFind);
			return folder;
		}

		// .slnx 형식도 지원
		pattern = folder + _T("\\*.slnx");
		hFind = ::FindFirstFile(pattern, &data);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			::FindClose(hFind);
			return folder;
		}

		// 한 단계 상위 폴더로 이동
		pos = folder.ReverseFind(_T('\\'));

		if (pos < 0)
			break;

		folder = folder.Left(pos);
	}

	return _T("");
}

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CCircleDrawerDlg 대화 상자



CCircleDrawerDlg::CCircleDrawerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CIRCLEDRAWER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCircleDrawerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PICTURE_CANVAS, m_canvas);
}

UINT RandomWorkerProc(LPVOID pParam)
{
	HWND hWnd = reinterpret_cast<HWND>(pParam);

	for (int i = 0; i < 10; i++)
	{
		// 0.5초 간격
		::Sleep(500);

		// UI 스레드에 이동 요청
		if (!::PostMessage(
			hWnd,
			WM_APP + 101,
			0,
			0))
		{
			break;
		}
	}

	return 0;
}

BEGIN_MESSAGE_MAP(CCircleDrawerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()

	ON_BN_CLICKED(IDC_BUTTON_RESET, &CCircleDrawerDlg::OnBnClickedButtonReset)
	ON_BN_CLICKED(IDC_BUTTON_SAVE, &CCircleDrawerDlg::OnBnClickedButtonSave)

	ON_BN_CLICKED(
		IDC_BUTTON_RANDOM,
		&CCircleDrawerDlg::OnBnClickedButtonRandom
	)

	ON_MESSAGE(
		WM_APP + 101,
		&CCircleDrawerDlg::OnRandomStep
	)
	ON_BN_CLICKED(IDC_BUTTON_LOAD,
		&CCircleDrawerDlg::OnBnClickedButtonLoad)
END_MESSAGE_MAP()


// CCircleDrawerDlg 메시지 처리기

BOOL CCircleDrawerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	SetDlgItemInt(IDC_EDIT_RADIUS, 10);
	// Picture Control 가져오기
	CWnd* pCanvas = GetDlgItem(IDC_PICTURE_CANVAS);

	if (pCanvas != nullptr)
	{
		CRect rect;
		pCanvas->GetClientRect(&rect);

		m_canvasWidth = rect.Width();
		m_canvasHeight = rect.Height();

		// Picture Control과 호환되는 메모리 DC 생성
		CClientDC dc(pCanvas);

		m_memDC.CreateCompatibleDC(&dc);

		// 메모리 비트맵 생성
		m_bitmap.CreateCompatibleBitmap(
			&dc,
			m_canvasWidth,
			m_canvasHeight
		);

		m_oldBitmap = m_memDC.SelectObject(&m_bitmap);

		// 비트맵 배경을 흰색으로 초기화
		m_memDC.FillSolidRect(
			0,
			0,
			m_canvasWidth,
			m_canvasHeight,
			RGB(255, 255, 255)
		);
		m_canvas.SetMemoryDC(&m_memDC);
	}

	srand(static_cast<unsigned int>(::GetTickCount()));

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CCircleDrawerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CCircleDrawerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CCircleDrawerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCircleDrawerDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_randomRunning)
		return;

	CRect rectCanvas;
	m_canvas.GetWindowRect(&rectCanvas);
	ScreenToClient(&rectCanvas);

	if (!rectCanvas.PtInRect(point))
	{
		CDialogEx::OnLButtonDown(nFlags, point);
		return;
	}

	CPoint canvasPoint(
		point.x - rectCanvas.left,
		point.y - rectCanvas.top
	);

	// 기존 원을 클릭했는지 검사
	for (int i = m_clickCount - 1; i >= 0; i--)
	{
		int dx = canvasPoint.x - m_points[i].x;
		int dy = canvasPoint.y - m_points[i].y;

		if (dx * dx + dy * dy <= m_radius * m_radius)
		{
			m_isDragging = true;
			m_dragIndex = i;

			m_dragOffset = CPoint(dx, dy);

			SetCapture();
			return;
		}
	}

	// 원이 3개 미만이면 새 원 생성
	if (m_clickCount < 3)
	{
		CString strRadius;
		GetDlgItemText(IDC_EDIT_RADIUS, strRadius);

		int radius = _ttoi(strRadius);

		if (radius <= 0)
		{
			AfxMessageBox(_T("반지름은 1 이상의 정수로 입력하세요."));
			return;
		}

		m_radius = radius;
		m_points[m_clickCount] = canvasPoint;
		m_clickCount++;

		UpdateCanvas();
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CCircleDrawerDlg::DrawCircle(
	CDC* pDC,
	int centerX,
	int centerY,
	int radius,
	COLORREF color)
{
	int x = 0;
	int y = radius;
	int d = 1 - radius;

	while (x <= y)
	{
		pDC->SetPixel(centerX + x, centerY + y, color);
		pDC->SetPixel(centerX - x, centerY + y, color);
		pDC->SetPixel(centerX + x, centerY - y, color);
		pDC->SetPixel(centerX - x, centerY - y, color);

		pDC->SetPixel(centerX + y, centerY + x, color);
		pDC->SetPixel(centerX - y, centerY + x, color);
		pDC->SetPixel(centerX + y, centerY - x, color);
		pDC->SetPixel(centerX - y, centerY - x, color);

		if (d < 0)
			d += 2 * x + 3;
		else
		{
			d += 2 * (x - y) + 5;
			y--;
		}

		x++;
	}
}

void CCircleDrawerDlg::UpdateCanvas()
{
	CWnd* pCanvas = GetDlgItem(IDC_PICTURE_CANVAS);

	if (pCanvas == nullptr || m_memDC.GetSafeHdc() == nullptr)
		return;

	// 1. 메모리 비트맵 배경을 흰색으로 초기화
	m_memDC.FillSolidRect(
		0, 0,
		m_canvasWidth, m_canvasHeight,
		RGB(255, 255, 255)
	);

	// 2. 메모리 비트맵에 원 그리기
	for (int i = 0; i < m_clickCount; i++)
	{
		int x = m_points[i].x;
		int y = m_points[i].y;

		DrawCircle(
			&m_memDC,
			x,
			y,
			m_radius,
			RGB(0, 0, 255)
		);

		CString strPoint;
		strPoint.Format(_T("(%d, %d)"), x, y);

		m_memDC.SetBkMode(TRANSPARENT);
		m_memDC.SetTextColor(RGB(0, 0, 0));

		m_memDC.TextOut(
			x + m_radius + 5,
			y,
			strPoint
		);
	}

	// 원이 3개일 때 외접원 그리기
	if (m_clickCount == 3)
	{
		DrawLargeCircle(&m_memDC);
	}

	// Picture Control에 다시 그리기 요청
	m_canvas.Invalidate(FALSE);
	m_canvas.UpdateWindow();
}



bool CCircleDrawerDlg::CalculateCircumcircle(
	double& centerX,
	double& centerY,
	double& radius)
{
	if (m_clickCount < 3)
		return false;

	double x1 = m_points[0].x;
	double y1 = m_points[0].y;

	double x2 = m_points[1].x;
	double y2 = m_points[1].y;

	double x3 = m_points[2].x;
	double y3 = m_points[2].y;

	double d = 2.0 * (
		x1 * (y2 - y3) +
		x2 * (y3 - y1) +
		x3 * (y1 - y2)
		);

	// 세 점이 일직선인 경우
	if (std::abs(d) < 0.000001)
		return false;

	double s1 = x1 * x1 + y1 * y1;
	double s2 = x2 * x2 + y2 * y2;
	double s3 = x3 * x3 + y3 * y3;

	centerX = (
		s1 * (y2 - y3) +
		s2 * (y3 - y1) +
		s3 * (y1 - y2)
		) / d;

	centerY = (
		s1 * (x3 - x2) +
		s2 * (x1 - x3) +
		s3 * (x2 - x1)
		) / d;

	radius = std::sqrt(
		(centerX - x1) * (centerX - x1) +
		(centerY - y1) * (centerY - y1)
	);

	return true;
}

void CCircleDrawerDlg::DrawLargeCircle(CDC* pDC)
{
	double centerX, centerY, radius;

	if (!CalculateCircumcircle(centerX, centerY, radius))
		return;

	int cx = static_cast<int>(std::round(centerX));
	int cy = static_cast<int>(std::round(centerY));
	int r = static_cast<int>(std::round(radius));

	// 기존 DrawCircle 함수로 큰 원 그리기
	DrawCircle(pDC, cx, cy, r, RGB(255, 0, 0));
}
void CCircleDrawerDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_randomRunning)
		return;

	if (m_isDragging && m_dragIndex >= 0)
	{
		CRect rectCanvas;
		m_canvas.GetWindowRect(&rectCanvas);
		ScreenToClient(&rectCanvas);

		CPoint canvasPoint(
			point.x - rectCanvas.left,
			point.y - rectCanvas.top
		);

		int newX = canvasPoint.x - m_dragOffset.x;
		int newY = canvasPoint.y - m_dragOffset.y;

		// 원의 중심이 캔버스 밖으로 나가지 않도록 제한
		newX = max(0, min(newX, m_canvasWidth - 1));
		newY = max(0, min(newY, m_canvasHeight - 1));

		m_points[m_dragIndex] = CPoint(newX, newY);

		// 큰 원까지 다시 계산해서 그리기
		UpdateCanvas();
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CCircleDrawerDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_isDragging)
	{
		m_isDragging = false;
		m_dragIndex = -1;

		if (GetCapture() == this)
			ReleaseCapture();
	}

	CDialogEx::OnLButtonUp(nFlags, point);
}

void CCircleDrawerDlg::OnBnClickedButtonReset()
{
	// 드래그 상태 초기화
	m_isDragging = false;
	m_dragIndex = -1;

	if (GetCapture() == this)
		ReleaseCapture();

	// 원 좌표 및 개수 초기화
	m_clickCount = 0;

	for (int i = 0; i < 3; i++)
	{
		m_points[i] = CPoint(0, 0);
	}

	// 반지름 기본값 복원
	m_radius = 10;
	SetDlgItemInt(IDC_EDIT_RADIUS, 10);

	// 캔버스 다시 그리기
	UpdateCanvas();
}

bool CCircleDrawerDlg::SaveCanvasImage(const CString& filePath)
{
	// 1. 메모리 비트맵 존재 확인
	if (m_bitmap.GetSafeHandle() == nullptr)
		return false;

	// 2. 저장할 이미지 생성
	CImage image;

	if (!image.Create(m_canvasWidth, m_canvasHeight, 24))
		return false;

	// 3. 이미지 DC 가져오기
	HDC hImageDC = image.GetDC();

	// 4. 메모리 비트맵의 내용을 이미지에 복사
	BOOL copied = BitBlt(
		hImageDC,
		0, 0,
		m_canvasWidth,
		m_canvasHeight,
		m_memDC.GetSafeHdc(),
		0, 0,
		SRCCOPY
	);

	// 5. 이미지 DC 해제
	image.ReleaseDC();

	if (!copied)
		return false;

	// 6. BMP 파일로 저장
	HRESULT hr = image.Save(
		filePath,
		Gdiplus::ImageFormatBMP
	);

	return SUCCEEDED(hr);
}

void CCircleDrawerDlg::OnBnClickedButtonSave()
{
	// 현재 캔버스 내용을 솔루션 폴더에 저장
	if (SaveAutoImage(_T("Circle_Manual")))
	{
		AfxMessageBox(
			_T("솔루션 폴더에 이미지가 저장되었습니다.")
		);
	}
	else
	{
		AfxMessageBox(
			_T("이미지 저장 실패: 솔루션 폴더나 저장 상태를 확인하세요.")
		);
	}
}

bool CCircleDrawerDlg::SaveAutoImage(
	const CString& prefix)
{
	CString folder = FindSolutionFolder();

	if (folder.IsEmpty())
		return false;

	// 기존 파일이 있으면 번호를 증가시켜 덮어쓰기 방지
	for (int i = 1; i <= 9999; i++)
	{
		CString path;

		path.Format(
			_T("%s\\%s_%03d.bmp"),
			folder.GetString(),
			prefix.GetString(),
			i
		);

		if (::GetFileAttributes(path)
			== INVALID_FILE_ATTRIBUTES)
		{
			return SaveCanvasImage(path);
		}
	}

	return false;
}

void CCircleDrawerDlg::OnBnClickedButtonRandom()
{
	if (m_randomRunning)
		return;

	if (m_clickCount != 3)
	{
		AfxMessageBox(
			_T("원 3개를 먼저 생성하세요.")
		);
		return;
	}

	int margin = max(m_radius + 2, 10);

	if (m_canvasWidth <= margin * 2 ||
		m_canvasHeight <= margin * 2)
	{
		AfxMessageBox(
			_T("캔버스 크기에 비해 반지름이 너무 큽니다.")
		);
		return;
	}

	m_randomRunning = true;
	m_randomStep = 0;

	// 이전 드래그 상태 종료
	m_isDragging = false;
	m_dragIndex = -1;

	if (GetCapture() == this)
		ReleaseCapture();

	// 랜덤 이동 중 중복 실행 및 초기화 방지
	GetDlgItem(IDC_BUTTON_RANDOM)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_RESET)->EnableWindow(FALSE);

	CWinThread* pThread = AfxBeginThread(
		RandomWorkerProc,
		reinterpret_cast<LPVOID>(GetSafeHwnd())
	);

	if (pThread == nullptr)
	{
		m_randomRunning = false;

		GetDlgItem(IDC_BUTTON_RANDOM)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_RESET)->EnableWindow(TRUE);

		AfxMessageBox(_T("작업 스레드 생성 실패"));
	}
}

LRESULT CCircleDrawerDlg::OnRandomStep(
	WPARAM wParam, LPARAM lParam)
{
	if (!m_randomRunning)
		return 0;

	int margin = max(m_radius + 2, 10);

	CPoint oldPoints[3];

	for (int i = 0; i < 3; i++)
		oldPoints[i] = m_points[i];

	bool valid = false;

	// 유효한 외접원이 생성될 때까지 좌표 재시도
	for (int attempt = 0; attempt < 1000; attempt++)
	{
		for (int i = 0; i < 3; i++)
		{
			int x = margin + rand() %
				(m_canvasWidth - 2 * margin);

			int y = margin + rand() %
				(m_canvasHeight - 2 * margin);

			m_points[i] = CPoint(x, y);
		}

		double cx, cy, radius;

		if (CalculateCircumcircle(cx, cy, radius))
		{
			// 지나치게 큰 외접원은 제외
			if (std::isfinite(radius) &&
				radius <= max(m_canvasWidth, m_canvasHeight))
			{
				valid = true;
				break;
			}
		}
	}

	if (!valid)
	{
		for (int i = 0; i < 3; i++)
			m_points[i] = oldPoints[i];

		m_randomRunning = false;
		GetDlgItem(IDC_BUTTON_RANDOM)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_RESET)->EnableWindow(TRUE);

		AfxMessageBox(_T("유효한 랜덤 원 생성 실패"));
		return 0;
	}

	// 1. 새로운 원과 외접원을 메모리 비트맵에 그림
	UpdateCanvas();

	// 2. 현재 이미지를 BMP로 저장
	if (!SaveAutoImage(_T("Circle_Random")))
	{
		m_randomRunning = false;
		GetDlgItem(IDC_BUTTON_RANDOM)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_RESET)->EnableWindow(TRUE);

		AfxMessageBox(_T("랜덤 이미지 저장 실패"));
		return 0;
	}

	// 3. 저장 성공 후 반복 횟수 증가
	m_randomStep++;

	// 4. 10회 완료 시 종료
	if (m_randomStep >= 10)
	{
		m_randomRunning = false;

		GetDlgItem(IDC_BUTTON_RANDOM)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_RESET)->EnableWindow(TRUE);
	}

	return 0;
}

void CCircleDrawerDlg::OnBnClickedButtonLoad()
{
	CFileDialog dlg(
		TRUE,
		_T("bmp"),
		nullptr,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("BMP Files (*.bmp)|*.bmp||"),
		this
	);

	if (dlg.DoModal() != IDOK)
		return;

	CImage image;

	if (FAILED(image.Load(dlg.GetPathName())))
	{
		AfxMessageBox(_T("이미지를 불러올 수 없습니다."));
		return;
	}

	int width = image.GetWidth();
	int height = image.GetHeight();

	// 캔버스 크기와 동일한 이미지 사용
	if (width != m_canvasWidth ||
		height != m_canvasHeight)
	{
		AfxMessageBox(_T("캔버스와 이미지 크기가 다릅니다."));
		return;
	}

	// 빨간색 큰 원 픽셀 수집
	std::vector<CPoint> pixels;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			COLORREF color = image.GetPixel(x, y);

			if (GetRValue(color) >= 180 &&
				GetGValue(color) <= 100 &&
				GetBValue(color) <= 100)
			{
				pixels.emplace_back(x, y);
			}
		}
	}

	// 이미지 원본을 메모리 비트맵으로 복사
	image.Draw(
		m_memDC.GetSafeHdc(),
		0, 0, width, height
	);

	// 원호 픽셀이 충분하지 않은 경우
	if (pixels.size() < 20)
	{
		m_canvas.Invalidate(FALSE);
		m_canvas.UpdateWindow();

		AfxMessageBox(_T("큰 원을 검출할 수 없습니다."));
		return;
	}

	// 1. 픽셀 좌표 평균
	double meanX = 0.0;
	double meanY = 0.0;

	for (const auto& p : pixels)
	{
		meanX += p.x;
		meanY += p.y;
	}

	double n = static_cast<double>(pixels.size());

	meanX /= n;
	meanY /= n;

	// 2. 최소제곱 원 피팅을 위한 합계 계산
	double sxx = 0, syy = 0, sxy = 0;
	double sxz = 0, syz = 0, sz = 0;

	for (const auto& p : pixels)
	{
		double x = p.x - meanX;
		double y = p.y - meanY;
		double z = x * x + y * y;

		sxx += x * x;
		syy += y * y;
		sxy += x * y;
		sxz += x * z;
		syz += y * z;
		sz += z;
	}

	double det = sxx * syy - sxy * sxy;

	// 원호가 너무 짧거나 점들이 거의 직선인 경우
	if (det <= 1e-8 * sxx * syy)
	{
		m_canvas.Invalidate(FALSE);
		m_canvas.UpdateWindow();

		AfxMessageBox(_T("원의 중심을 안정적으로 계산할 수 없습니다."));
		return;
	}

	// 3. 원의 중심과 반지름 계산
	double dx = (sxz * syy - syz * sxy) / (2.0 * det);
	double dy = (syz * sxx - sxz * sxy) / (2.0 * det);

	double cx = meanX + dx;
	double cy = meanY + dy;
	double radius = std::sqrt(sz / n + dx * dx + dy * dy);

	if (!std::isfinite(cx) ||
		!std::isfinite(cy) ||
		!std::isfinite(radius) ||
		radius <= 0)
	{
		AfxMessageBox(_T("원 분석 결과가 유효하지 않습니다."));
		return;
	}

	// 4. 피팅 정확도 검증
	double error = 0.0;

	for (const auto& p : pixels)
	{
		double distance = std::hypot(p.x - cx, p.y - cy);
		double diff = distance - radius;
		error += diff * diff;
	}

	double rms = std::sqrt(error / n);

	if (rms > max(3.0, radius * 0.025))
	{
		m_canvas.Invalidate(FALSE);
		m_canvas.UpdateWindow();

		AfxMessageBox(_T("원 검출 정확도가 부족합니다."));
		return;
	}

	// 5. 중심이 캔버스 내부에 있는지 확인
	bool centerInside =
		cx >= 0 && cx < width &&
		cy >= 0 && cy < height;

	if (centerInside)
	{
		int centerX = static_cast<int>(std::round(cx));
		int centerY = static_cast<int>(std::round(cy));

		// 중심에 십자 표시
		for (int i = -5; i <= 5; i++)
		{
			m_memDC.SetPixel(
				centerX + i, centerY,
				RGB(0, 150, 0)
			);

			m_memDC.SetPixel(
				centerX, centerY + i,
				RGB(0, 150, 0)
			);
		}

		// 중심에서 원 둘레까지 반지름 선 표시
		CPen pen(PS_SOLID, 2, RGB(0, 150, 0));
		CPen* oldPen = m_memDC.SelectObject(&pen);

		m_memDC.MoveTo(centerX, centerY);
		m_memDC.LineTo(pixels[0].x, pixels[0].y);

		m_memDC.SelectObject(oldPen);
	}

	// 6. 오른쪽 하단에 분석 결과 표시
	CString result;

	result.Format(
		_T("Center: (%.1f, %.1f)\nRadius: %.1f px%s"),
		cx, cy, radius,
		centerInside ? _T("") : _T(" [Outside]")
	);

	CRect resultRect(
		max(0, width - 260),
		max(0, height - 58),
		width - 5,
		height - 5
	);

	m_memDC.FillSolidRect(
		&resultRect,
		RGB(255, 255, 255)
	);

	m_memDC.SetBkMode(TRANSPARENT);
	m_memDC.SetTextColor(RGB(0, 0, 0));

	m_memDC.DrawText(
		result,
		&resultRect,
		DT_LEFT | DT_VCENTER | DT_WORDBREAK
	);

	// 7. 분석된 이미지 출력
	m_canvas.Invalidate(FALSE);
	m_canvas.UpdateWindow();
}