// GraphicsMisc.h: interface for the GraphicsMisc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRAPHICSMISC_H__A3408501_A44D_407B_A8C3_B6AB31370CD2__INCLUDED_)
#define AFX_GRAPHICSMISC_H__A3408501_A44D_407B_A8C3_B6AB31370CD2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum 
{ 
	GMFS_BOLD		= 0x01, 
	GMFS_ITALIC		= 0x02, 
	GMFS_UNDERLINED	= 0x04, 
	GMFS_STRIKETHRU	= 0x08, 
	GMFS_SYMBOL		= 0x10,

	GMFS_ALL		= 0xff
};

enum 
{
	GMDR_LEFT		= 0x1, 		
	GMDR_TOP		= 0x2, 
	GMDR_RIGHT		= 0x4, 
	GMDR_BOTTOM		= 0x8, 

	GMDR_ALL		= 0xf
};

enum GM_GRADIENT
{
	GMG_NONE,
	GMG_GRADIENT,
	GMG_GLASS,
	GMG_GLASSWITHGRADIENT,
};

namespace GraphicsMisc  
{
	BOOL DrawGradient(GM_GRADIENT nType, CDC* pDC, LPCRECT pRect, COLORREF crFrom, COLORREF crTo, BOOL bHorz, int nBorder = -1);
	COLORREF GetGradientEdgeColor(GM_GRADIENT nType, COLORREF color, BOOL bFrom);
	GM_GRADIENT GetGradientType(BOOL bGlass, BOOL bGradient);

	void DrawHorzLine(CDC* pDC, int nXFrom, int nXTo, int nYPos, COLORREF crFrom, COLORREF crTo = CLR_NONE);
	void DrawVertLine(CDC* pDC, int nYFrom, int nYTo, int nXPos, COLORREF crFrom, COLORREF crTo = CLR_NONE);
		
	HFONT CreateFont(HFONT hFont, DWORD dwFlags = 0, DWORD dwMask = GMFS_ALL);
	HFONT CreateFont(LPCTSTR szFaceName, int nPoint = -1, DWORD dwFlags = 0);
	BOOL CreateFont(CFont& font, LPCTSTR szFaceName, int nPoint = -1, DWORD dwFlags = 0);
	BOOL CreateFont(CFont& fontOut, HFONT fontIn, DWORD dwFlags = 0, DWORD dwMask = GMFS_ALL);

	UINT GetRTLDrawTextFlags(HWND hwnd);
	UINT GetRTLDrawTextFlags(CDC* pDC);
	
	HCURSOR HandCursor();
	HICON LoadIcon(UINT nIDIcon, int nSize = 16);
	HBITMAP IconToPARGB32Bitmap(HICON hIcon);
	CSize GetIconSize(HICON hIcon);
	CSize GetBitmapSize(HBITMAP hBmp);

	int PointToPixel(int nPoints);
	int PixelToPoint(int nPixels);
	int PixelsPerInch();

	DWORD GetFontFlags(HFONT hFont);
	int GetFontNameAndPointSize(HFONT hFont, CString& sFaceName);
	int GetFontPointSize(HFONT hFont);
	int GetFontPixelSize(HFONT hFont);
	BOOL SameFont(HFONT hFont, LPCTSTR szFaceName, int nPoint);
	BOOL SameFontNameSize(HFONT hFont1, HFONT hFont2);
	CFont& WingDings();
	CFont& Marlett();
	int DrawSymbol(CDC* pDC, char cSymbol, const CRect& rText, UINT nFlags, CFont* pFont = NULL);
	CFont* PrepareDCFont(CDC* pDC, CWnd* pWndRef = NULL, CFont* pFont = NULL, int nStockFont = DEFAULT_GUI_FONT); // returns 'old' font
	
	int GetTextWidth(UINT nIDString, CWnd& wndRef, CFont* pRefFont = NULL);
	int GetTextWidth(const CString& sText, CWnd& wndRef, CFont* pRefFont = NULL);
	int AFX_CDECL GetTextWidth(CDC* pDC, LPCTSTR lpszFormat, ...);
	float GetAverageCharWidth(CDC* pDC);
	int GetAverageStringWidth(const CString& sText, CDC* pDC);
	int GetAverageMaxStringWidth(const CString& sText, CDC* pDC);

	inline BOOL VerifyDeleteObject(HGDIOBJ hObj)
	{
		if (hObj == NULL)
			return TRUE;

		// else
		if (::DeleteObject(hObj))
			return TRUE;

		// else
		ASSERT(0);
		return FALSE;
	}

	inline BOOL VerifyDeleteObject(CGdiObject& obj)
	{
		if (obj.m_hObject == NULL)
			return TRUE;

		// else
		if (obj.DeleteObject())
			return TRUE;

		// else
		ASSERT(0);
		return FALSE;
	}
	
	COLORREF Lighter(COLORREF color, double dAmount);
	COLORREF Darker(COLORREF color, double dAmount);
	COLORREF Blend(COLORREF color1, COLORREF color2, double dAmount);
	COLORREF GetBestTextColor(COLORREF crBack);

	CString GetWebColor(COLORREF color);

	void DrawRect(CDC* pDC, const CRect& rect, COLORREF crFill, COLORREF crBorder = CLR_NONE, 
					int nCornerRadius = 0, DWORD dwEdges = GMDR_ALL);
	
	BOOL ForceIconicRepresentation(HWND hWnd, BOOL bForce = TRUE);
	BOOL EnableAeroPeek(HWND hWnd, BOOL bEnable = TRUE);
	BOOL EnableFlip3D(HWND hWnd, BOOL bEnable = TRUE);
	BOOL DwmSetWindowAttribute(HWND hWnd, DWORD dwAttrib, LPCVOID pData, DWORD dwDataSize);
	BOOL GetScreenWorkArea(HWND hWnd, CRect& rWorkArea, UINT nMonitor = MONITOR_DEFAULTTONEAREST); 

	BOOL GetAvailableScreenSpace(const CRect& rWnd, CRect& rScreen);
	BOOL GetAvailableScreenSpace(HWND hWnd, CRect& rScreen);
	BOOL GetAvailableScreenSpace(CRect& rScreen);

	void DrawGradient(CDC* pDC, LPCRECT pRect, COLORREF crFrom, COLORREF crTo, BOOL bHorz, int nBorder);
	void DrawGlass(CDC* pDC, LPCRECT pRect, COLORREF crFrom, COLORREF crTo, BOOL bHorz, int nBorder);
	void DrawGlassWithGradient(CDC* pDC, LPCRECT pRect, COLORREF crFrom, COLORREF crTo, BOOL bHorz, int nBorder);
};

#endif // !defined(AFX_GRAPHICSMISC_H__A3408501_A44D_407B_A8C3_B6AB31370CD2__INCLUDED_)
