#pragma once

#include <afxwin.h>

class CCanvasCtrl : public CStatic
{
public:
    CCanvasCtrl();
    virtual ~CCanvasCtrl();

    CDC* m_pMemDC = nullptr;

    void SetMemoryDC(CDC* pDC);

protected:
    afx_msg void OnPaint();

    DECLARE_MESSAGE_MAP()
};