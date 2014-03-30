#include "StdAfx.h"
#include "MeterWebkit.h"
#include "Measure.h"
#include "Error.h"
#include "Rainmeter.h"
#include "System.h"
#include "../Common/PathUtil.h"
#include "../Common/Gfx/Canvas.h"

#include <locale>
#include <codecvt>
#include <string>
#include <fstream>
#include <cstdlib>

using namespace Gdiplus;
using namespace Awesomium;

size_t GetSizeOfFile(const std::wstring& path)
{
	struct _stat fileinfo;
	_wstat(path.c_str(), &fileinfo);
	return fileinfo.st_size;
}

std::wstring get_file_contents(const wchar_t *filename)
{
	std::wstring buffer;            // stores file contents
	FILE* f = _wfopen(filename, L"rtS, ccs=UTF-8");

	// Failed to open file
	if (f == NULL)
	{
		// ...handle some error...
		return buffer;
	}

	size_t filesize = GetSizeOfFile(filename);

	// Read entire file contents in to memory
	if (filesize > 0)
	{
		buffer.resize(filesize);
		size_t wchars_read = fread(&(buffer.front()), sizeof(wchar_t), filesize, f);
		buffer.resize(wchars_read);
		buffer.shrink_to_fit();
	}

	fclose(f);

	return buffer;
}


WebCore* m_webCore;

MeterWebkit::MeterWebkit(MeterWindow* meterWindow, const WCHAR* name) : Meter(meterWindow, name),
m_NeedsRedraw(true),
m_webView(nullptr),
m_Focus(false),
m_Window(meterWindow),
m_hasInjectedCss(false),
m_hasInjectedScripts(false),
m_AlwaysReinject(false)
{
}

MeterWebkit::~MeterWebkit(){
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
	m_Text = parser.ReadString(section, L"URL", L"http://www.clocklink.com/html5embed.php?clock=012&timezone=PST&color=black&size=170");
	m_injectJavascript = parser.ReadString(section, L"Javascript", L"");
	m_injectCSS = parser.ReadString(section, L"CSS", L"");

	m_AlwaysReinject = parser.ReadBool(section, L"AlwaysReinject", false);

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
	m_webView = m_webCore->CreateWebView(meterRect.Width, meterRect.Height, m_session);
	
	char *dest = new char[8024];
	const wchar_t *p = m_Text.c_str();
	wcstombs(dest, p, 8024);
	
	WebURL url(WebString::CreateFromUTF8(dest, m_Text.length()));
	m_webView->LoadURL(url);
	m_webView->SetTransparent(true);
	
//	Awesomium::WebScript

	delete dest;

	bih = { 0 };
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biBitCount = 32;
	bih.biCompression = BI_RGB;
	bih.biPlanes = 1;
	bih.biWidth = meterRect.Width;
	bih.biHeight = -(meterRect.Height);
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

//	m_hasInjectedCss = false; // always rerun until a proper handler to check if url changed is here

	if (m_AlwaysReinject == true) m_hasInjectedCss = false;
	if (m_webCore != nullptr && m_webView != nullptr)
	{
		if (m_webView->IsLoading() == false && !m_hasInjectedScripts)
		{
			if (m_injectJavascript != std::wstring(L""))
			{
				LogDebug(L"Injecting Javascript into page..");

				std::wstring contents = get_file_contents(const_cast<wchar_t*>(m_injectJavascript.c_str()));

				char *dest = new char[8024 * 4];
				const wchar_t *p = contents.c_str();
				wcstombs(dest, p, 8024 * 4);

				WebString code = WebString::CreateFromUTF8(dest, contents.length());
				m_webView->ExecuteJavascript(code, WebString(WSLit("")));
			}

			m_hasInjectedScripts = true;
		}

		m_hasInjectedCss = false;

		if (m_webView->IsLoading() == false && !m_hasInjectedCss)
		{
			if (m_injectCSS != std::wstring(L""))
			{
				LogDebug(L"Injecting CSS into page..");

				std::wstring contents = get_file_contents(const_cast<wchar_t*>(m_injectCSS.c_str()));

				char *dest = new char[8024 * 4];
				const wchar_t *p = contents.c_str();
				wcstombs(dest, p, 8024 * 4);

				WebString inject = WSLit("el = document.createElement('style'); el.id='custom_styles'; if (document.querySelectorAll('#custom_styles').length != 0) { el = document.querySelectorAll('#custom_styles')[0]; } el.type='text/css'; el.innerHTML='");
				WebString code = WebString::CreateFromUTF8(dest, contents.length());
				 
				inject.Append(code);
				inject.Append(WSLit("'; document.body.appendChild(el);"));

				LogDebugF(L"css injection script: %s", dest);

				m_webView->ExecuteJavascript(inject, WebString(WSLit("")));
			}

			m_hasInjectedCss = true;
		}

		Awesomium::BitmapSurface* bmp = (Awesomium::BitmapSurface*)m_webView->surface();
		
		if (bmp)
		{
			unsigned char* bitmap_buf = 0;
			HDC dc = CreateCompatibleDC(NULL);
			HBITMAP bitmap = CreateDIBSection(dc, (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&(bitmap_buf), 0, 0);

			bmp->CopyTo(bitmap_buf, meterRect.Width * 4, 4, false, false);
			Gdiplus::Bitmap *outputBmp;
			HBitmapToBitmap(bitmap, PixelFormat32bppARGB, (Gdiplus::Bitmap**)&outputBmp);
			canvas.DrawBitmap(outputBmp, meterRect, meterRect);

			delete outputBmp;
			DeleteObject(bitmap);
			bitmap = NULL;
			DeleteDC(dc);
			bitmapOutput = NULL;
			bitmap_buf = 0;
		}
	}

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

/*
** Checks if the given point is inside the web frame
**
*/
bool MeterWebkit::HitTest2(int px, int py)
{
	int x = GetX();
	int y = GetY();

	if (m_MouseOver &&
		px >= x && px < x + m_W &&
		py >= y && py < y + m_H)
	{
		if (m_SolidColor.GetA() != 0 || m_SolidColor2.GetA() != 0)
		{
			return true;
		}

		return true;
	}
	return false;
}

void MeterWebkit::SetWebFocus(bool focus)
{
	if (focus) m_webView->Focus();
	if (!focus) m_webView->Unfocus();
}

bool MeterWebkit::MouseUp(POINT pos, bool execute)
{
	if (m_State == BUTTON_STATE_DOWN)
	{
		// execute && m_Clicked && 
		LogNoticeF(L"Browser Mouseup: x=%i y=%i", pos.x, pos.y);
		if (m_Focus)
		{
			m_webView->InjectMouseUp(kMouseButton_Left);
		}
		m_State = BUTTON_STATE_NORMAL;
		m_Clicked = false;
		return true;
	}
	m_Clicked = false;

	return false;
}

bool MeterWebkit::MouseDown(POINT pos)
{
	if (m_Focus)
	{
		m_State = BUTTON_STATE_DOWN;
		m_Clicked = true;

		m_webView->InjectMouseDown(kMouseButton_Left);

		return true;
	}
	return false;
}

bool MeterWebkit::MouseMove(POINT pos)
{
//	RECT r = GetMeterRect();
	m_webView->InjectMouseMove(pos.x, pos.y);


	if (m_Clicked)
	{
		if (HitTest2(pos.x, pos.y))
		{
			if (m_State == BUTTON_STATE_NORMAL)
			{
				m_State = BUTTON_STATE_DOWN;
				return true;
			}
		}
		else
		{
			// If the left button is not down anymore the clicked state needs to be set false
			if (!IsLButtonDown())
			{
				m_Clicked = false;
			}

			if (m_State == BUTTON_STATE_DOWN)
			{
				m_State = BUTTON_STATE_NORMAL;
				return true;
			}
		}
	}
	else
	{
		if (HitTest2(pos.x, pos.y))
		{
			if (m_State == BUTTON_STATE_NORMAL)
			{
				m_State = BUTTON_STATE_HOVER;
				return true;
			}
		}
		else
		{
			if (m_State == BUTTON_STATE_HOVER)
			{
				m_State = BUTTON_STATE_NORMAL;
				return true;
			}
		}
	}
	return false;
}
