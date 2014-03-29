#include "StdAfx.h"
#include "MeterWebkit.h"
#include "Measure.h"
#include "Error.h"
#include "Rainmeter.h"
#include "System.h"
#include "../Common/PathUtil.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

WebCore* m_webCore;

enum BUTTON_STATE
{
	BUTTON_STATE_NORMAL,
	BUTTON_STATE_DOWN,
	BUTTON_STATE_HOVER
};

MeterWebkit::MeterWebkit(MeterWindow* meterWindow, const WCHAR* name) : Meter(meterWindow, name),
m_NeedsRedraw(true),
m_webView(nullptr),
m_TextFormat(meterWindow->GetCanvas().CreateTextFormat())
{
	m_Text = L"testing testing 1234-5 sixty..";
}

MeterWebkit::~MeterWebkit(){
	delete m_TextFormat;
	m_TextFormat = nullptr;

	m_webView->Destroy();
	//m_webCore->Shutdown();
	//WebCore::Shutdown();
}

void MeterWebkit::Initialize()
{
	Meter::Initialize();
}

void MeterWebkit::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Meter::ReadOptions(parser, section);
	//m_Color = parser.ReadColor(section, L"FontColor", Color::Black);

	InitWebview();
}

void MeterWebkit::InitWebview()
{
	m_webConfig.log_path = WSLit("C:\\rainmeter\\awesomium.txt");
	m_webConfig.log_level = kLogLevel_Verbose;

	if (!m_webCore) m_webCore = WebCore::Initialize(m_webConfig);

	m_session = m_webCore->CreateWebSession(
		WSLit("C:\\rainmeter\\sessions\\"), WebPreferences());

	Gdiplus::Rect meterRect = GetMeterRectPadding();
	m_webView = m_webCore->CreateWebView(500, 500, m_session);

	WebURL url(WSLit("http://www.clocklink.com/html5embed.php?clock=012&timezone=PST&color=black&size=170"));
	m_webView->LoadURL(url);
	m_webView->SetTransparent(true);

	BITMAPINFOHEADER bih = { 0 };
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biBitCount = 32;
	bih.biCompression = BI_RGB;
	bih.biPlanes = 1;
	bih.biWidth = 512;
	bih.biHeight = -(512);

	HDC dc = CreateCompatibleDC(NULL);
	bitmap = CreateDIBSection(dc, (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&(bitmap_buf), 0, 0);
}

bool MeterWebkit::Update()
{
	bool update = Meter::Update();
	if (m_webCore != nullptr) m_webCore->Update();
	return update;
}

Gdiplus::Status HBitmapToBitmap(HBITMAP source, Gdiplus::PixelFormat pixel_format, Gdiplus::Bitmap** result_out)
{
	BITMAP source_info = { 0 };
	if (!::GetObject(source, sizeof(source_info), &source_info))
		return Gdiplus::GenericError;

	Gdiplus::Status s;

	std::auto_ptr< Gdiplus::Bitmap > target(new Gdiplus::Bitmap(source_info.bmWidth, source_info.bmHeight, pixel_format));
	if (!target.get())
		return Gdiplus::OutOfMemory;
	if ((s = target->GetLastStatus()) != Gdiplus::Ok)
		return s;

	Gdiplus::BitmapData target_info;
	Gdiplus::Rect rect(0, 0, source_info.bmWidth, source_info.bmHeight);

	s = target->LockBits(&rect, Gdiplus::ImageLockModeWrite, pixel_format, &target_info);
	if (s != Gdiplus::Ok)
		return s;

	if (target_info.Stride != source_info.bmWidthBytes)
		return Gdiplus::InvalidParameter; // pixel_format is wrong!

	CopyMemory(target_info.Scan0, source_info.bmBits, source_info.bmWidthBytes * source_info.bmHeight);

	s = target->UnlockBits(&target_info);
	if (s != Gdiplus::Ok)
		return s;

	*result_out = target.release();

	return Gdiplus::Ok;
}

bool MeterWebkit::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	Gdiplus::Graphics& graphics = canvas.BeginGdiplusContext();
	Gdiplus::Rect meterRect = GetMeterRectPadding();

	RectF rcDest((REAL)meterRect.X, (REAL)meterRect.Y, (REAL)meterRect.Width, (REAL)meterRect.Height);

	//if (!m_TextFormat->IsInitialized()) return false;
	if (m_webCore != nullptr) m_webCore->Update();

	if (m_webCore != nullptr && m_webView != nullptr && !m_webView->IsLoading())
	{
		Awesomium::BitmapSurface* bmp = (Awesomium::BitmapSurface*)m_webView->surface();
		
		if (bmp)
		{
		//	bmp->SaveToPNG(WSLit("C:\\rainmeter\\test.png"));
			Gdiplus::Bitmap* bitmapOutput = new Bitmap(meterRect.Width, meterRect.Height, &graphics);

			bmp->CopyTo(bitmap_buf, 512 * 4, 4, false, false);
			bmp->set_is_dirty(false);
		//	
			HBitmapToBitmap(bitmap, PixelFormat32bppARGB, &bitmapOutput);
			canvas.DrawBitmap(bitmapOutput, meterRect, meterRect);

			delete bitmapOutput;
		}
	}

	/**
		LPCWSTR string = m_Text.c_str();
		UINT stringLen = (UINT)m_Text.length();

		canvas.SetTextAntiAliasing(m_AntiAlias);

		SolidBrush solidBrush(m_Color);
		canvas.DrawTextW(string, (UINT)stringLen, *m_TextFormat, rcDest, solidBrush);
	*/
	canvas.EndGdiplusContext();

	return true;
}

/*
** Overridden method. The meters need not to be bound on anything
**
*/
void MeterWebkit::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	BindPrimaryMeasure(parser, section, true);
}