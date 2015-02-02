// EnBitmap.cpp: implementation of the CEnBitmap class (c) daniel godson 2002.
//
// credits: Peter Hendrix's CPicture implementation for the original IPicture code 
//          Yves Maurer's GDIRotate implementation for the idea of working directly on 32 bit representations of bitmaps 
//          Karl Lager's 'A Fast Algorithm for Rotating Bitmaps' 
// 
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EnBitmap.h"
#include "graphicsMisc.h"
#include "clipboard.h"

#ifdef PNG_SUPPORT
#	include "gdiplus.h"
#endif

#include <afxpriv.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

const int HIMETRIC_INCH	= 2540;

///////////////////////////////////////////////////////////////////////

C32BitImageProcessor::C32BitImageProcessor(BOOL bEnableWeighting) : m_bWeightingEnabled(bEnableWeighting)
{
}

C32BitImageProcessor::~C32BitImageProcessor()
{
}

CSize C32BitImageProcessor::CalcDestSize(CSize sizeSrc) 
{ 
	return sizeSrc; // default
}

BOOL C32BitImageProcessor::ProcessPixels(RGBX* pSrcPixels, CSize /*sizeSrc*/, RGBX* pDestPixels, 
										CSize sizeDest, COLORREF /*crMask*/)
{ 
	CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); // default
	return TRUE;
}
 
void C32BitImageProcessor::CalcWeightedColor(RGBX* pPixels, CSize size, double dX, double dY, RGBX& rgbResult)
{
	ASSERT (m_bWeightingEnabled);

	// interpolate between the current pixel and its adjacent pixels to the right and down
	int nX = (int)dX;
	int nY = (int)dY;

	if (dX < 0 || dY < 0)
	{
		rgbResult = pPixels[max(0, nY) * size.cx + max(0, nX)]; // closest
		return;
	}

	double dXFraction = dX - nX;
	double dX1MinusFraction = 1 - dXFraction;
	
	double dYFraction = dY - nY;
	double dY1MinusFraction = 1 - dYFraction;

	int nXP1 = min(nX + 1, size.cx - 1);
	int nYP1 = min(nY + 1, size.cy - 1);
	
	RGBX* pRGB = &pPixels[nY * size.cx + nX]; // x, y
	RGBX* pRGBXP = &pPixels[nY * size.cx + nXP1]; // x + 1, y
	RGBX* pRGBYP = &pPixels[nYP1 * size.cx + nX]; // x, y + 1
	RGBX* pRGBXYP = &pPixels[nYP1 * size.cx + nXP1]; // x + 1, y + 1
	
	int nRed = (int)(dX1MinusFraction * dY1MinusFraction * pRGB->btRed +
					dXFraction * dY1MinusFraction * pRGBXP->btRed +
					dX1MinusFraction * dYFraction * pRGBYP->btRed +
					dXFraction * dYFraction * pRGBXYP->btRed);
	
	int nGreen = (int)(dX1MinusFraction * dY1MinusFraction * pRGB->btGreen +
					dXFraction * dY1MinusFraction * pRGBXP->btGreen +
					dX1MinusFraction * dYFraction * pRGBYP->btGreen +
					dXFraction * dYFraction * pRGBXYP->btGreen);
	
	int nBlue = (int)(dX1MinusFraction * dY1MinusFraction * pRGB->btBlue +
					dXFraction * dY1MinusFraction * pRGBXP->btBlue +
					dX1MinusFraction * dYFraction * pRGBYP->btBlue +
					dXFraction * dYFraction * pRGBXYP->btBlue);

	rgbResult.btRed = (BYTE)max(0, min(255, nRed));
	rgbResult.btGreen = (BYTE)max(0, min(255, nGreen));
	rgbResult.btBlue =(BYTE) max(0, min(255, nBlue));
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEnBitmap::CEnBitmap(COLORREF crBkgnd) : m_crBkgnd(crBkgnd), m_sizeDest(0, 0)
{

}

CEnBitmap::CEnBitmap(int cx, int cy, COLORREF crBkgnd) : m_crBkgnd(crBkgnd), m_sizeDest(cx, cy)
{
}

CEnBitmap::~CEnBitmap()
{

}

BOOL CEnBitmap::LoadImage(UINT uIDRes, LPCTSTR szResourceType, HMODULE hInst, COLORREF crBack)
{
	ASSERT(m_hObject == NULL);      // only attach once, detach on destroy

	if (m_hObject != NULL)
		return FALSE;

	HBITMAP hbm = LoadImageResource(uIDRes, szResourceType, hInst, 
									(crBack == -1 ? m_crBkgnd : crBack), 
									m_sizeDest.cx, m_sizeDest.cy);

	return Attach(hbm);
}

BOOL CEnBitmap::LoadImage(LPCTSTR szImagePath, COLORREF crBack)
{
	ASSERT(m_hObject == NULL);      // only attach once, detach on destroy

	if (m_hObject != NULL)
		return FALSE;

	if (crBack == -1)
		crBack = m_crBkgnd;

	HBITMAP hbm = LoadImageFile(szImagePath, crBack, m_sizeDest.cx, m_sizeDest.cy);

	return Attach(hbm);
}

HBITMAP CEnBitmap::LoadImageFile(LPCTSTR szImagePath, COLORREF crBack, int cx, int cy)
{
	int nType = GetFileType(szImagePath);
	HBITMAP hbm = NULL;

	switch (nType)
	{
	case FT_BMP:
		// the reason for this is that i suspect it is more efficient to load
		// bmps this way since it avoids creating device contexts etc that the 
		// IPicture methods requires. that method however is still valuable
		// since it handles other image types and transparency
		hbm = (HBITMAP)::LoadImage(NULL, szImagePath, IMAGE_BITMAP, cx, cy, LR_LOADFROMFILE | LR_LOADMAP3DCOLORS);
		break;
		
	case FT_UNKNOWN:
		break;
		
	case FT_ICO:
		{
			HICON hIcon = (HICON)::LoadImage(NULL, szImagePath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
			
			if (hIcon)
			{
				hbm = ExtractBitmap(hIcon, crBack, cx, cy);
			
				// cleanup
				::DestroyIcon(hIcon);
			}
		}
		break;
		
#ifdef PNG_SUPPORT
	case FT_PNG:
		{
			gdix_Bitmap* gdBitmap = NULL;
			HBITMAP hbmGdiP = NULL;

			if (CGdiPlus::CreateBitmapFromFile(szImagePath, &gdBitmap) &&
				CGdiPlus::CreateHBITMAPFromBitmap(gdBitmap, &hbmGdiP, crBack))
			{
				hbm = ExtractBitmap(hbmGdiP, crBack, cx, cy);

				// cleanup
				::DeleteObject(hbmGdiP);
			}
		}
		break;
#endif
		
	default: // all the rest
		{
			USES_CONVERSION;
			IPicture* pPicture = NULL;
			
			HRESULT hr = OleLoadPicturePath(T2OLE((LPTSTR)szImagePath), NULL, 0, crBack, IID_IPicture, (LPVOID*)&pPicture);
			
			if (SUCCEEDED(hr) && pPicture)
			{
				hbm = ExtractBitmap(pPicture, crBack, cx, cy);
				pPicture->Release();
			}
		}
		break;
	}
	
	return hbm;
}

HBITMAP CEnBitmap::LoadImageResource(UINT uIDRes, LPCTSTR szResourceType, HMODULE hInst, COLORREF crBack, int cx, int cy)
{
	BYTE* pBuff = NULL;
	int nSize = 0;
	HBITMAP hbm = NULL;

	// first call is to get buffer size
	if (GetResource(MAKEINTRESOURCE(uIDRes), szResourceType, hInst, 0, nSize))
	{
		if (nSize > 0)
		{
			pBuff = new BYTE[nSize];
			
			// this loads it
			if (GetResource(MAKEINTRESOURCE(uIDRes), szResourceType, hInst, pBuff, nSize))
			{
				IPicture* pPicture = LoadFromBuffer(pBuff, nSize);

				if (pPicture)
				{
					hbm = ExtractBitmap(pPicture, crBack, cx, cy);
					pPicture->Release();
				}
			}
			
			delete [] pBuff;
		}
	}

	return hbm;
}

IPicture* CEnBitmap::LoadFromBuffer(BYTE* pBuff, int nSize)
{
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, nSize);
	void* pData = GlobalLock(hGlobal);
	memcpy(pData, pBuff, nSize);
	GlobalUnlock(hGlobal);

	IStream* pStream = NULL;
	IPicture* pPicture = NULL;

	if (CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) == S_OK)
	{
		OleLoadPicture(pStream, nSize, FALSE, IID_IPicture, (LPVOID *)&pPicture);
		pStream->Release();
	}

	return pPicture; // caller releases
}

BOOL CEnBitmap::GetResource(LPCTSTR lpName, LPCTSTR lpType, HMODULE hInst, void* pResource, int& nBufSize)
{ 
	HRSRC		hResInfo;
	HANDLE		hRes;
	LPSTR		lpRes	= NULL; 
	bool		bResult	= FALSE;

	// Find the resource
	hResInfo = FindResource(hInst, lpName, lpType);

	if (hResInfo == NULL) 
		return false;

	// Load the resource
	hRes = LoadResource(hInst, hResInfo);

	if (hRes == NULL) 
		return false;

	// Lock the resource
	lpRes = (LPSTR)(LPCTSTR)LockResource(hRes);

	if (lpRes != NULL)
	{ 
		if (pResource == NULL)
		{
			nBufSize = SizeofResource(hInst, hResInfo);
			bResult = true;
		}
		else
		{
			if (nBufSize >= (int)SizeofResource(hInst, hResInfo))
			{
				memcpy(pResource, lpRes, nBufSize);
				bResult = true;
			}
		} 

		UnlockResource(hRes);  
	}

	// Free the resource
	FreeResource(hRes);

	return bResult;
}

HBITMAP CEnBitmap::ExtractBitmap(IPicture* pPicture, COLORREF crBack, int cx, int cy)
{
	ASSERT(pPicture);

	if (!pPicture)
		return NULL;

	CBitmap bmMem;
	CDC dcMem;
	CDC* pDC = CWnd::GetDesktopWindow()->GetDC();

	if (dcMem.CreateCompatibleDC(pDC))
	{
		long hmWidth;
		long hmHeight;

		pPicture->get_Width(&hmWidth);
		pPicture->get_Height(&hmHeight);
		
		int nWidth	= cx;

		if (nWidth == 0)
			nWidth = MulDiv(hmWidth, pDC->GetDeviceCaps(LOGPIXELSX), HIMETRIC_INCH);

		int nHeight	= cy;

		if (nHeight == 0)
			nHeight = MulDiv(hmHeight, pDC->GetDeviceCaps(LOGPIXELSY), HIMETRIC_INCH);

		if (bmMem.CreateCompatibleBitmap(pDC, nWidth, nHeight))
		{
			CBitmap* pOldBM = dcMem.SelectObject(&bmMem);

			if (crBack != -1)
				dcMem.FillSolidRect(0, 0, nWidth, nHeight, crBack);
			
			HRESULT hr = pPicture->Render(dcMem, 0, 0, nWidth, nHeight, 0, hmHeight, hmWidth, -hmHeight, NULL);
			ASSERT (SUCCEEDED(hr));

			// cleanup
			dcMem.SelectObject(pOldBM);
		}
	}

	CWnd::GetDesktopWindow()->ReleaseDC(pDC);

	return (HBITMAP)bmMem.Detach();
}

HBITMAP CEnBitmap::ExtractBitmap(HICON hIcon, COLORREF crBack, int cx, int cy)
{
	// sanity checks
	ASSERT(hIcon);

	if (hIcon == NULL)
		return NULL;

	if (cx == 0 || cy == 0)
	{
		cx = cy = 32;
	}

	// when we load the icon we have no idea whether it 
	// contains an image of the required size, so we use an
	// imagelist to do the heavy lifting
	HBITMAP hbm = NULL;
	CImageList il;
			
	if (il.Create(cx, cy, ILC_COLOR32 | ILC_MASK, 1, 1))
	{
		if (il.Add(hIcon) == 0)
			hbm = ExtractBitmap(il, crBack, cx, cy);
	}

	return hbm;
}

#ifdef PNG_SUPPORT
HBITMAP CEnBitmap::ExtractBitmap(HBITMAP hbmGdip, COLORREF crBack, int cx, int cy)
{
	// sanity checks
	ASSERT(hbmGdip);

	if (hbmGdip == NULL)
		return NULL;

	HBITMAP hbm = NULL;

	if (cx == 0 || cy == 0)
	{
		BITMAP bm = { 0 };
		VERIFY (::GetObject(hbmGdip, sizeof(bm), &bm));

		cx = bm.bmWidth;
		cy = bm.bmHeight;
	}

	CImageList il;
	
	if (il.Create(cx, cy, ILC_COLOR32 | ILC_MASK, 1, 1))
	{
		if (il.Add(CBitmap::FromHandle(hbmGdip), crBack) == 0)
		{
			hbm = ExtractBitmap(il, crBack, cx, cy);
		}
	}

	return hbm;
}
#endif

HBITMAP CEnBitmap::ExtractBitmap(const CImageList& il, COLORREF crBack, int cx, int cy)
{
	HBITMAP hbm = NULL;
// 	IMAGEINFO ii = { 0 };
// 
// 	VERIFY (il.GetImageInfo(0, &ii));
// 
// 	int cx = (ii.rcImage.right - ii.rcImage.left);
// 	int cy = (ii.rcImage.bottom - ii.rcImage.top);
		
	HDC hDC = ::GetDC(::GetDesktopWindow());
	HDC hMemDC = ::CreateCompatibleDC(NULL);

	if (hMemDC)
	{
		hbm = ::CreateCompatibleBitmap(hDC, cx, cy);
		
		if (hbm)
		{
			HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hbm);
			
			::SetBkColor(hMemDC, crBack);
			::ExtTextOut(hMemDC, 0, 0, ETO_OPAQUE, CRect(0, 0, cx, cy), NULL, 0, NULL);
			
			ImageList_Draw(il, 0, hMemDC, 0, 0, ILD_TRANSPARENT);
			
			::SelectObject(hMemDC, hOldBitmap);
		}
		
		::DeleteDC(hMemDC);
	}
	
	// cleanup
	::ReleaseDC(::GetDesktopWindow(), hDC);

	return hbm;
}

BOOL CEnBitmap::CopyToClipboard(HWND hWnd) const
{
	if (!hWnd || !GetSafeHandle())
		return FALSE;

	return CClipboard(hWnd).SetData(*this);
}

BOOL CEnBitmap::CopyImageFileToClipboard(HWND hWnd, LPCTSTR szImagePath, COLORREF crBack)
{
	CEnBitmap bm;

	if (bm.LoadImage(szImagePath, crBack))
		return bm.CopyToClipboard(hWnd);

	return FALSE;
}

EB_IMAGETYPE CEnBitmap::GetFileType(LPCTSTR szImagePath)
{
	CString sPath(szImagePath);
	sPath.MakeUpper();

	if (sPath.Find(_T(".BMP")) > 0)
		return FT_BMP;

	else if (sPath.Find(_T(".ICO")) > 0)
		return FT_ICO;

	else if (sPath.Find(_T(".JPG")) > 0 || sPath.Find(_T(".JPEG")) > 0)
		return FT_JPG;

	else if (sPath.Find(_T(".GIF")) > 0)
		return FT_GIF;

#ifdef PNG_SUPPORT
	else if (sPath.Find(_T(".PNG")) > 0)
		return FT_PNG;
#endif

	// else
	return FT_UNKNOWN;
}

BOOL CEnBitmap::ProcessImage(C32BitImageProcessor* pProcessor, COLORREF crMask)
{
	C32BIPArray aProcessors;

	aProcessors.Add(pProcessor);

	return ProcessImage(aProcessors, crMask);
}

BOOL CEnBitmap::ProcessImage(C32BIPArray& aProcessors, COLORREF crMask)
{
	ASSERT (GetSafeHandle());

	if (!GetSafeHandle())
		return FALSE;

	if (!aProcessors.GetSize())
		return TRUE;

	int nProcessor, nCount = aProcessors.GetSize();

	// retrieve src and final dest sizes
	BITMAP BM;

	if (!GetBitmap(&BM))
		return FALSE;

	CSize sizeSrc(BM.bmWidth, BM.bmHeight);
	CSize sizeDest(sizeSrc), sizeMax(sizeSrc);

	for (nProcessor = 0; nProcessor < nCount; nProcessor++)
	{
		sizeDest = aProcessors[nProcessor]->CalcDestSize(sizeDest);
		sizeMax = CSize(max(sizeMax.cx, sizeDest.cx), max(sizeMax.cy, sizeDest.cy));
	}

	// prepare src and dest bits
	RGBX* pSrcPixels = GetDIBits32();

	if (!pSrcPixels)
		return FALSE;

	RGBX* pDestPixels = new RGBX[sizeMax.cx * sizeMax.cy];

	if (!pDestPixels)
		return FALSE;

	Fill(pDestPixels, sizeMax, m_crBkgnd);

	BOOL bRes = TRUE;
	sizeDest = sizeSrc;

	// do the processing
	for (nProcessor = 0; bRes && nProcessor < nCount; nProcessor++)
	{
		// if its the second processor or later then we need to copy
		// the previous dest bits back into source.
		// we also need to check that sizeSrc is big enough
		if (nProcessor > 0)
		{
			if (sizeSrc.cx < sizeDest.cx || sizeSrc.cy < sizeDest.cy)
			{
				delete [] pSrcPixels;
				pSrcPixels = new RGBX[sizeDest.cx * sizeDest.cy];
			}

			CopyMemory(pSrcPixels, pDestPixels, sizeDest.cx * 4 * sizeDest.cy); // default
			Fill(pDestPixels, sizeDest, m_crBkgnd);
		}

		sizeSrc = sizeDest;
		sizeDest = aProcessors[nProcessor]->CalcDestSize(sizeSrc);
		
		bRes = aProcessors[nProcessor]->ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest, crMask);
	}

	// update the bitmap
	if (bRes)
	{
		// set the bits
		HDC hdc = GetDC(NULL);
		HBITMAP hbmSrc = ::CreateCompatibleBitmap(hdc, sizeDest.cx, sizeDest.cy);

		if (hbmSrc)
		{
			BITMAPINFO bi;

			if (PrepareBitmapInfo32(bi, hbmSrc))
			{
				if (SetDIBits(hdc, hbmSrc, 0, sizeDest.cy, pDestPixels, &bi, DIB_RGB_COLORS))
				{
					// delete the bitmap and attach new
					GraphicsMisc::VerifyDeleteObject(*this);
					bRes = Attach(hbmSrc);
				}
			}

			::ReleaseDC(NULL, hdc);

			if (!bRes)
				GraphicsMisc::VerifyDeleteObject(hbmSrc);
		}
	}

	delete [] pSrcPixels;
	delete [] pDestPixels;

	return bRes;
}

RGBX* CEnBitmap::GetDIBits32()
{
	BITMAPINFO bi;

	int nHeight = PrepareBitmapInfo32(bi);
	
	if (!nHeight)
		return FALSE;

	BYTE* pBits = (BYTE*)new BYTE[bi.bmiHeader.biSizeImage];
	HDC hdc = GetDC(NULL);

	if (!GetDIBits(hdc, (HBITMAP)GetSafeHandle(), 0, nHeight, pBits, &bi, DIB_RGB_COLORS))
	{
		delete [] pBits;
		pBits = NULL;
	}

	::ReleaseDC(NULL, hdc);

	return (RGBX*)pBits;
}

BOOL CEnBitmap::PrepareBitmapInfo32(BITMAPINFO& bi, HBITMAP hBitmap)
{
	if (!hBitmap)
		hBitmap = (HBITMAP)GetSafeHandle();

	BITMAP BM;

	if (!::GetObject(hBitmap, sizeof(BM), &BM))
		return FALSE;

	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = BM.bmWidth;
	bi.bmiHeader.biHeight = -BM.bmHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32; // 32 bit
	bi.bmiHeader.biCompression = BI_RGB; // 32 bit
	bi.bmiHeader.biSizeImage = BM.bmWidth * 4 * BM.bmHeight; // 32 bit
	bi.bmiHeader.biClrUsed = 0;
	bi.bmiHeader.biClrImportant = 0;

	return BM.bmHeight;
}

BOOL CEnBitmap::CopyImage(HBITMAP hBitmap)
{
	ASSERT (hBitmap);
	
	if (!hBitmap)
		return FALSE;
	
	BITMAPINFO bi;
	int nHeight = PrepareBitmapInfo32(bi, hBitmap);

	if (!nHeight)
		return FALSE;

	BYTE* pBits = (BYTE*)new BYTE[bi.bmiHeader.biSizeImage];
	HDC hdc = GetDC(NULL);
	BOOL bRes = FALSE;

	if (GetDIBits(hdc, hBitmap, 0, nHeight, pBits, &bi, DIB_RGB_COLORS))
	{
		int nWidth = bi.bmiHeader.biSizeImage / (nHeight * 4);

		HBITMAP hbmDest = ::CreateCompatibleBitmap(hdc, nWidth, nHeight);

		if (hbmDest)
		{
			if (SetDIBits(hdc, hbmDest, 0, nHeight, pBits, &bi, DIB_RGB_COLORS))
			{
				GraphicsMisc::VerifyDeleteObject(*this);
				bRes = Attach(hbmDest);
			}
		}
	}

	::ReleaseDC(NULL, hdc);
	delete [] pBits;

	return bRes;
}

BOOL CEnBitmap::CopyImage(CBitmap* pBitmap)
{
	if (!pBitmap)
		return FALSE;

	return CopyImage((HBITMAP)pBitmap->GetSafeHandle());
}

BOOL CEnBitmap::Fill(RGBX* pPixels, CSize size, COLORREF color)
{
	if (!pPixels)
		return FALSE;

	if (color == -1 || color == RGB(255, 255, 255))
		FillMemory(pPixels, size.cx * 4 * size.cy, 255); // white

	else if (color == 0)
		FillMemory(pPixels, size.cx * 4 * size.cy, 0); // black

	else
	{
		// fill the first line with the color
		RGBX* pLine = &pPixels[0];
		int nSize = 1;

		pLine[0] = RGBX(color);

		while (1)
		{
			if (nSize > size.cx)
				break;

			// else
			int nAmount = min(size.cx - nSize, nSize) * 4;

			CopyMemory(&pLine[nSize], pLine, nAmount);
			nSize *= 2;
		}

		// use that line to fill the rest of the block
		int nRow = 1;

		while (1)
		{
			if (nRow > size.cy)
				break;

			// else
			int nAmount = min(size.cy - nRow, nRow) * size.cx * 4;

			CopyMemory(&pPixels[nRow * size.cx], pPixels, nAmount);
			nRow *= 2;
		}
	}

	return TRUE;
}

BOOL CEnBitmap::Copy(HIMAGELIST hImageList)
{
	GraphicsMisc::VerifyDeleteObject(*this);
	
	int nWidth = 0, nHeight = 0;
	int nCount = ImageList_GetImageCount(hImageList);
	
	ImageList_GetIconSize(hImageList, &nWidth, &nHeight);
	
	HDC hdc = GetDC(NULL);
	HBITMAP hbmDest = ::CreateCompatibleBitmap(hdc, nWidth * nCount, nHeight);
	HDC hdcMem = CreateCompatibleDC(hdc);
	
	HBITMAP hBMOld = (HBITMAP)::SelectObject(hdcMem, hbmDest);
	
	for (int nIcon = 0; nIcon < nCount; nIcon++)
	{
		VERIFY (ImageList_Draw(hImageList, nIcon, hdcMem, nIcon * nWidth, 0, ILD_NORMAL));
	}
	
	// cleanup
	::SelectObject(hdcMem, hBMOld);
	DeleteDC(hdcMem);
	
	return Attach(hbmDest);
}
