#include "pch.h"
#include "CCanvasCtrl.h"

CCanvasCtrl::CCanvasCtrl()
{
}

CCanvasCtrl::~CCanvasCtrl()
{
}

BEGIN_MESSAGE_MAP(CCanvasCtrl, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()

void CCanvasCtrl::SetMemoryDC(CDC* pDC)
{
    m_pMemDC = pDC;

    if (GetSafeHwnd() != nullptr)
    {
        Invalidate(FALSE);
    }
}

void CCanvasCtrl::OnPaint()
{
    CPaintDC dc(this);

    CRect rect;
    GetClientRect(&rect);

    if (m_pMemDC != nullptr &&
        m_pMemDC->GetSafeHdc() != nullptr)
    {
        // 메모리 비트맵을 화면에 복사
        dc.BitBlt(
            0, 0,
            rect.Width(),
            rect.Height(),
            m_pMemDC,
            0, 0,
            SRCCOPY
        );
    }
    else
    {
        // 비트맵이 없다면 흰색 배경
        dc.FillSolidRect(
            &rect,
            RGB(255, 255, 255)
        );
    }
}