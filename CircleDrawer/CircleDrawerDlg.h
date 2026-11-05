
// CircleDrawerDlg.h: 헤더 파일
//

#pragma once

#include "CCanvasCtrl.h"

// CCircleDrawerDlg 대화 상자
class CCircleDrawerDlg : public CDialogEx
{
// 생성입니다.
public:
	CCircleDrawerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CIRCLEDRAWER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	afx_msg void OnBnClickedButtonReset();
	afx_msg void OnBnClickedButtonSave();

	afx_msg void OnBnClickedButtonRandom();
	afx_msg LRESULT OnRandomStep(WPARAM wParam, LPARAM lParam);

	afx_msg void OnBnClickedButtonLoad();

public:
	CPoint m_points[3];   // 클릭한 좌표 3개
	int m_clickCount = 0; // 현재 클릭 횟수
	int m_radius = 10;

	void DrawCircle(
		CDC* pDC,
		int centerX,
		int centerY,
		int radius,
		COLORREF color
	);
	void UpdateCanvas();

	bool CalculateCircumcircle(
		double& centerX,
		double& centerY,
		double& radius
	);

	void DrawLargeCircle(CDC* pDC);
	bool SaveCanvasImage(const CString& filePath);
	bool SaveAutoImage(const CString& prefix);

private:
	CCanvasCtrl m_canvas;

	CDC m_memDC;          // 메모리에서 그림을 그릴 DC
	CBitmap m_bitmap;     // 그림을 보관할 비트맵
	CBitmap* m_oldBitmap = nullptr;

	int m_canvasWidth = 0;
	int m_canvasHeight = 0;

	bool m_isDragging = false;
	int m_dragIndex = -1;
	CPoint m_dragOffset = CPoint(0, 0);

	bool m_randomRunning = false;
	int m_randomStep = 0;

};
