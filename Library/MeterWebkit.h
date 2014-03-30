/*
Copyright (C) 2014 gstach / modified from MeterImage

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __METERWEBKIT_H__
#define __METERWEBKIT_H__

#include "Meter.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
#include <Awesomium/BitmapSurface.h>
#include "Logger.h"
#include "MeterWindow.h"

#include <string>
#include <fstream>
#include <streambuf>

enum BUTTON_STATE
{
	BUTTON_STATE_NORMAL,
	BUTTON_STATE_DOWN,
	BUTTON_STATE_HOVER
};

class MeterWebkit : public Meter
{
public:
	MeterWebkit(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeterWebkit();

	MeterWebkit(const MeterWebkit& other) = delete;
	MeterWebkit& operator=(MeterWebkit other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterWebkit>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

	bool MouseMove(POINT pos);
	bool MouseUp(POINT pos, bool execute);
	bool MouseDown(POINT pos);
	
	bool HitTest2(int px, int py);

	void SetFocus(bool f) { m_Focus = f; SetWebFocus(f); }

	Awesomium::WebView* GetWebview() { return m_webView; };

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

	virtual bool IsFixedSize(bool overwrite = false) { return overwrite ? true : false; }

	virtual void InitWebview();
	
	std::wstring m_Text;
	Gfx::TextFormat* m_TextFormat;
	Gdiplus::Color m_Color;

	std::wstring m_injectJavascript;
	std::wstring m_injectCSS;
	
	Awesomium::WebConfig m_webConfig;
	Awesomium::WebView* m_webView;
	Awesomium::WebSession* m_session;

	bool m_Focus;
	bool m_Clicked;
	BUTTON_STATE m_State;

	bool m_hasInjectedScripts;
	bool m_hasInjectedCss;

	MeterWindow *m_Window;

private:
	/*enum DRAWMODE
	{
		DRAWMODE_NONE = 0,
		DRAWMODE_TILE,
		DRAWMODE_KEEPRATIO,
		DRAWMODE_KEEPRATIOANDCROP
	};
	*/

	//void LoadImage(const std::wstring& imageName, bool bLoadAlways);

	bool m_NeedsRedraw;
	bool m_AlwaysReinject;

	//DRAWMODE m_DrawMode;

	Gdiplus::Bitmap* bitmapOutput;
	BITMAPINFOHEADER bih;

	RECT m_ScaleMargins;
	void SetWebFocus(bool);
};

#endif
