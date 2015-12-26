#include "mupdf/fitz.h"
#include "mupdf/pdf.h"

#include <windows.h>
#include <windowsx.h>

#include <npapi.h>

#define PAD 5

typedef struct pdfmoz_s pdfmoz_t;
typedef struct page_s page_t;

struct page_s
{
	fz_page *page;
	fz_pixmap *image;
	fz_rect bbox;
	fz_display_list *contents;
	fz_display_list *annotations;
	fz_link *links;
	int errored;
	int w, h;	/* page size in units */
	int px; /* pixel height */
};

struct pdfmoz_s
{
	fz_context *ctx;
	NPP inst;
	HWND hwnd;
	HWND sbar;
	WNDPROC winproc;
	HCURSOR arrow, hand, wait;
	BITMAPINFO *dibinf;
	HBRUSH graybrush;

	int scrollpage;	/* scrollbar -> page (n) */
	int scrollyofs;	/* scrollbar -> page offset in pixels */

	int pagecount;
	page_t *pages;

	fz_document *doc;

	char error[1024];	/* empty if no error has occured */
};

static void pdfmoz_warn(pdfmoz_t *moz, const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	buf[sizeof(buf)-1] = 0;
	NPN_Status(moz->inst, buf);
}

static void pdfmoz_error(pdfmoz_t *moz, char *msg)
{
	strcpy(moz->error, msg);
	InvalidateRect(moz->hwnd, NULL, FALSE);
}

static void pdfmoz_open(pdfmoz_t *moz, char *filename)
{
	SCROLLINFO si;
	fz_page *page;
	int i;

	strcpy(moz->error, "");

	fz_try(moz->ctx)
	{
		fz_register_document_handlers(moz->ctx);

		moz->doc = fz_open_document(moz->ctx, filename);

		/*
		* Handle encrypted PDF files
		*/  
		if (fz_needs_password(moz->ctx, moz->doc))
			fz_throw(moz->ctx, FZ_ERROR_GENERIC, "Needs a password");

		moz->pagecount = fz_count_pages(moz->ctx, moz->doc);
		if (moz->pagecount <= 0)
			fz_throw(moz->ctx, FZ_ERROR_GENERIC, "No pages in document");
	}
	fz_catch(moz->ctx)
	{
		pdfmoz_error(moz, "Cannot open document");
		return;
	}


	/*
	* Load page tree
	*/

	moz->pages = fz_malloc(moz->ctx, sizeof(page_t) * moz->pagecount);

	for (i = 0; i < moz->pagecount; i++)
	{
		moz->pages[i].page = NULL;
		moz->pages[i].image = NULL;
		moz->pages[i].bbox.x0 = 0;
		moz->pages[i].bbox.y0 = 0;
		moz->pages[i].bbox.x1 = 100;
		moz->pages[i].bbox.y1 = 100;
		moz->pages[i].contents = NULL;
		moz->pages[i].annotations = NULL;
		moz->pages[i].links = NULL;

		page = fz_load_page(moz->ctx, moz->doc, i);
		fz_bound_page(moz->ctx, page, &moz->pages[i].bbox);
		moz->pages[i].w = moz->pages[i].bbox.x1 - moz->pages[i].bbox.x0;
		moz->pages[i].h = moz->pages[i].bbox.y1 - moz->pages[i].bbox.y0;
		fz_drop_page(moz->ctx, page);

		moz->pages[i].px = 1 + PAD;
	}

	/*
	* Start at first page
	*/

	si.cbSize = sizeof(si);
	si.fMask = SIF_POS | SIF_RANGE; // XXX | SIF_PAGE;
	si.nPos = 0;
	si.nMin = 0;
	si.nMax = 1;
	si.nPage = 1;
	SetScrollInfo(moz->hwnd, SB_VERT, &si, TRUE);

	moz->scrollpage = 0;
	moz->scrollyofs = 0;

	InvalidateRect(moz->hwnd, NULL, FALSE);
}

static void decodescroll(pdfmoz_t *moz, int spos)
{
	int i, y = 0;
	moz->scrollpage = 0;
	moz->scrollyofs = 0;
	for (i = 0; i < moz->pagecount; i++)
	{
		if (spos >= y && spos < y + moz->pages[i].px)
		{
			moz->scrollpage = i;
			moz->scrollyofs = spos - y;
			return;
		}
		y += moz->pages[i].px;
	}
}

static void pdfmoz_pagectm(fz_matrix *mat, pdfmoz_t *moz, int pagenum)
{
	page_t *page = moz->pages + pagenum;
	float zoom;
	RECT rc;

	GetClientRect(moz->hwnd, &rc);

	zoom = (rc.right - rc.left) / (float) page->w;

	mat = fz_scale(mat, zoom, zoom);
}

void pdfmoz_loadpage(pdfmoz_t *moz, int pagenum)
{
	page_t *page = moz->pages + pagenum;
	fz_device *mdev = NULL;
	int errored = 0;
	fz_cookie cookie = { 0 };

	if (page->page)
		return;

	fz_var(mdev);

	fz_try(moz->ctx)
	{
		page->page = fz_load_page(moz->ctx, moz->doc, pagenum);
	}
	fz_catch(moz->ctx)
	{
		pdfmoz_warn(moz, "Cannot load page");
		return;
	}

	fz_try(moz->ctx)
	{
		fz_annot *annot;
		/* Create display lists */
		page->contents = fz_new_display_list(moz->ctx);
		mdev = fz_new_list_device(moz->ctx, page->contents);
		fz_run_page_contents(moz->ctx, page->page, mdev, &fz_identity, &cookie);
		fz_drop_device(moz->ctx, mdev);
		mdev = NULL;
		page->annotations = fz_new_display_list(moz->ctx);
		mdev = fz_new_list_device(moz->ctx, page->annotations);
		for (annot = fz_first_annot(moz->ctx, page->page); annot; annot = fz_next_annot(moz->ctx, page->page, annot))
			fz_run_annot(moz->ctx, page->page, annot, mdev, &fz_identity, &cookie);
		if (cookie.errors)
		{
			pdfmoz_warn(moz, "Errors found on page");
			errored = 1;
		}
	}
	fz_always(moz->ctx)
	{
		fz_drop_device(moz->ctx, mdev);
	}
	fz_catch(moz->ctx)
	{
		pdfmoz_warn(moz, "Cannot load page");
		errored = 1;
	}
	fz_try(moz->ctx)
	{
		page->links = fz_load_links(moz->ctx, page->page);
	}
	fz_catch(moz->ctx)
	{
		if (!errored)
			pdfmoz_warn(moz, "cannot load page");
	}

	page->errored = errored;
}

static void pdfmoz_runpage(fz_context *ctx, page_t *page, fz_device *dev, const fz_matrix *ctm, const fz_rect *rect, fz_cookie *cookie)
{
	fz_begin_page(ctx, dev, rect, ctm);
	if (page->contents)
		fz_run_display_list(ctx, page->contents, dev, ctm, rect, cookie);
	if (page->annotations)
		fz_run_display_list(ctx, page->annotations, dev, ctm, rect, cookie);
	fz_end_page(ctx, dev);
}

static void pdfmoz_drawpage(pdfmoz_t *moz, int pagenum)
{
	page_t *page = moz->pages + pagenum;
	fz_matrix ctm;
	fz_rect bounds;
	fz_irect ibounds;
	fz_colorspace *colorspace = fz_device_bgr(moz->ctx);// we're not imlementing grayscale toggle
	fz_device *idev;
	fz_cookie cookie = { 0 };

	if (page->image)
		return;

	pdfmoz_pagectm(&ctm, moz, pagenum);
	bounds = page->bbox;
	fz_round_rect(&ibounds, fz_transform_rect(&bounds, &ctm));
	fz_rect_from_irect(&bounds, &ibounds);
	page->image = fz_new_pixmap_with_bbox(moz->ctx, colorspace, &ibounds);
	fz_clear_pixmap_with_value(moz->ctx, page->image, 255);
	if (page->contents || page->annotations)
	{
		idev = fz_new_draw_device(moz->ctx, page->image);
		pdfmoz_runpage(moz->ctx, page, idev, &ctm, &bounds, &cookie);
		fz_drop_device(moz->ctx, idev);
	}

	if (cookie.errors && page->errored == 0)
	{
		page->errored = 1;
		pdfmoz_warn(moz, "Errors found on page. Page rendering may be incomplete.");
	}
}

static void pdfmoz_gotouri(pdfmoz_t *moz, char *uri)
{
	NPN_GetURL(moz->inst, uri, "_blank");
}

static void pdfmoz_gotopage(pdfmoz_t *moz, int number)
{
	int i, y = 0;

	moz->scrollpage = number;
	for (i = 0; i < moz->pagecount; i++)
	{
		if (i == number)
		{
			SetScrollPos(moz->hwnd, SB_VERT, y, TRUE);
			InvalidateRect(moz->hwnd, NULL, FALSE);
			return;
		}
		y += moz->pages[i].px;
	}
}

static void pdfmoz_onmouse(pdfmoz_t *moz, int x, int y, int click)
{
	char buf[512];
	fz_irect irect;
	fz_link *link;
	fz_matrix ctm;
	fz_point p;
	int pi;
	int py;

	if (!moz->pages)
		return;

	pi = moz->scrollpage;
	py = -moz->scrollyofs;
	while (pi < moz->pagecount)
	{
		if (!moz->pages[pi].image)
			return;
		if (y > py && y < moz->pages[pi].px)
			break;
		py += moz->pages[pi].px;
		pi ++;
	}
	if (pi == moz->pagecount)
		return;

	fz_pixmap_bbox(moz->ctx, moz->pages[pi].image, &irect);
	p.x = x + irect.x0;
	p.y = y + irect.y0 - py;

	pdfmoz_pagectm(&ctm, moz, pi);
	fz_invert_matrix(&ctm, &ctm);

	fz_transform_point(&p, &ctm);

	for (link = moz->pages[pi].links; link; link = link->next)
	{
		if (p.x >= link->rect.x0 && p.x <= link->rect.x1)
			if (p.y >= link->rect.y0 && p.y <= link->rect.y1)
				break;
	}

	if (link)
	{
		SetCursor(moz->hand);
		if (click)
		{
			if (link->dest.kind == FZ_LINK_URI)
				pdfmoz_gotouri(moz, link->dest.ld.uri.uri);
			else if (link->dest.kind == FZ_LINK_GOTO)
				pdfmoz_gotopage(moz, link->dest.ld.gotor.page);
			return;
		}
		else
		{
			if (link->dest.kind == FZ_LINK_URI)
				NPN_Status(moz->inst, link->dest.ld.uri.uri);
			else if (link->dest.kind == FZ_LINK_GOTO)
			{
				sprintf(buf, "Go to page %d", link->dest.ld.gotor.page + 1);
				NPN_Status(moz->inst, buf);
			}
			else
				NPN_Status(moz->inst, "Say what?");
		}
	}
	else
	{
		sprintf(buf, "Page %d of %d", moz->scrollpage + 1, moz->pagecount);
		NPN_Status(moz->inst, buf);
		SetCursor(moz->arrow);
	}
}

static void drawimage(HDC hdc, pdfmoz_t *moz, fz_pixmap *image, int yofs)
{
	int image_w = fz_pixmap_width(moz->ctx, image);
	int image_h = fz_pixmap_height(moz->ctx, image);
	unsigned char *samples = fz_pixmap_samples(moz->ctx, image);

	if (!image)
		return;

	moz->dibinf->bmiHeader.biWidth = image_w;
	moz->dibinf->bmiHeader.biHeight = -image_h;
	moz->dibinf->bmiHeader.biSizeImage = image_h * 4;

	SetDIBitsToDevice(hdc,
		0, yofs, image_w, image_h,
		0, 0, 0, image_h, samples,
	moz->dibinf, DIB_RGB_COLORS);
}

LRESULT CALLBACK
MozWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	pdfmoz_t *moz = (pdfmoz_t*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	char buf[256];

	int x = (signed short) LOWORD(lParam);
	int y = (signed short) HIWORD(lParam);
	int i, h;

	SCROLLINFO si;
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rc;
	RECT pad;
	WORD sendmsg;
	float zoom;

	GetClientRect(hwnd, &rc);
	h = rc.bottom - rc.top;

	if (strlen(moz->error))
	{
		if (msg == WM_PAINT)
		{
			hdc = BeginPaint(hwnd, &ps);
			FillRect(hdc, &rc, GetStockBrush(WHITE_BRUSH));
			rc.top += 10;
			rc.bottom -= 10;
			rc.left += 10;
			rc.right -= 10;
			DrawText(hdc, moz->error, strlen(moz->error), &rc, 0);
				// DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			EndPaint(hwnd, &ps);
		}
		if (msg == WM_MOUSEMOVE)
			SetCursor(moz->arrow);
		return 0;
	}

	switch (msg)
	{

		case WM_PAINT:
			GetClientRect(moz->hwnd, &rc);

			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hwnd, SB_VERT, &si);

			decodescroll(moz, si.nPos);

			/* evict out-of-range images and pages */
			for (i = 0; i < moz->pagecount; i++)
			{
				if (i < moz->scrollpage - 2 || i > moz->scrollpage + 6)
				{
					fz_drop_link(moz->ctx, moz->pages[i].links);
					moz->pages[i].links = NULL;

					fz_drop_display_list( moz->ctx, moz->pages[i].contents);
					moz->pages[i].contents =NULL;

					fz_drop_display_list( moz->ctx, moz->pages[i].annotations);
					moz->pages[i].annotations = NULL;

					fz_drop_page(moz->ctx, moz->pages[i].page);
					moz->pages[i].page = NULL;
				}
				if (i < moz->scrollpage - 1 || i > moz->scrollpage + 3)
				{
					fz_drop_pixmap(moz->ctx, moz->pages[i].image);
					moz->pages[i].image = NULL;
				}
			}

			i = moz->scrollpage;

			pdfmoz_loadpage(moz, i);
			if (moz->error[0])
				return 0;

			pdfmoz_drawpage(moz, i);
			if (moz->error[0])
				return 0;

			y = -moz->scrollyofs;
			while (y < h && i < moz->pagecount)
			{
				pdfmoz_loadpage(moz, i);
				if (moz->error[0])
					return 0;
				pdfmoz_drawpage(moz, i);
				if (moz->error[0])
					return 0;
				y += fz_pixmap_height(moz->ctx, moz->pages[i].image);
				i ++;
			}

			hdc = BeginPaint(hwnd, &ps);

			pad.left = rc.left;
			pad.right = rc.right;

			i = moz->scrollpage;
			y = -moz->scrollyofs;
			while (y < h && i < moz->pagecount)
			{
				drawimage(hdc, moz, moz->pages[i].image, y);
				y += fz_pixmap_height(moz->ctx, moz->pages[i].image);
				i ++;

				pad.top = y;
				pad.bottom = y + PAD;
				FillRect(hdc, &pad, moz->graybrush);
				y += PAD;
			}

			if (y < h)
			{
				pad.top = y;
				pad.bottom = h;
				FillRect(hdc, &pad, moz->graybrush);
			}

			EndPaint(hwnd, &ps);

			return 0;

		case WM_SIZE:
			ShowScrollBar(moz->hwnd, SB_VERT, TRUE);
			GetClientRect(moz->hwnd, &rc);

			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
			si.nPos = 0;
			si.nMin = 0;
			si.nMax = 0;
			// si.nPage = MAX(30, rc.bottom - rc.top - 30);
			si.nPage = rc.bottom - rc.top;

			for (i = 0; i < moz->pagecount; i++)
			{
				zoom = (rc.right - rc.left) / (float) moz->pages[i].w;
				moz->pages[i].px = zoom * moz->pages[i].h + PAD;

				if (moz->scrollpage == i)
				{
					si.nPos = si.nMax;
					if (moz->pages[i].image)
					{
						si.nPos +=
						moz->pages[i].px *
						moz->scrollyofs /
						fz_pixmap_height(moz->ctx, moz->pages[i].image) + 1;
					}
				}

				if (moz->pages[i].image)
					fz_drop_pixmap(moz->ctx, moz->pages[i].image);
				moz->pages[i].image = NULL;

				si.nMax += moz->pages[i].px;
			}

			si.nMax --;

			SetScrollInfo(moz->hwnd, SB_VERT, &si, TRUE);

			break;

		case WM_MOUSEMOVE:
			pdfmoz_onmouse(moz, x, y, 0);
			break;

		case WM_LBUTTONDOWN:
			SetFocus(hwnd);
			pdfmoz_onmouse(moz, x, y, 1);
			break;

		case WM_VSCROLL:
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hwnd, SB_VERT, &si);

			switch (LOWORD(wParam))
			{
				case SB_BOTTOM: si.nPos = si.nMax; break;
				case SB_TOP: si.nPos = 0; break;
				case SB_LINEUP: si.nPos -= 50; break;
				case SB_LINEDOWN: si.nPos += 50; break;
				case SB_PAGEUP: si.nPos -= si.nPage; break;
				case SB_PAGEDOWN: si.nPos += si.nPage; break;
				case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
				case SB_THUMBPOSITION: si.nPos = si.nTrackPos; break;
			}

			si.fMask = SIF_POS;
			si.nPos = fz_maxi(0, fz_mini(si.nPos, si.nMax));
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

			InvalidateRect(moz->hwnd, NULL, FALSE);

			decodescroll(moz, si.nPos);
			sprintf(buf, "Page %d of %d", moz->scrollpage + 1, moz->pagecount);
			NPN_Status(moz->inst, buf);

			return 0;

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif

		case WM_MOUSEWHEEL:
			if ((signed short)HIWORD(wParam) > 0)
				SendMessage(hwnd, WM_VSCROLL, MAKELONG(SB_LINEUP, 0), 0);
			else
				SendMessage(hwnd, WM_VSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
			break;

		case WM_KEYDOWN:
			sendmsg = 0xFFFF;

			switch (wParam)
			{
				case VK_UP: sendmsg = SB_LINEUP; break;
				case VK_PRIOR: sendmsg = SB_PAGEUP; break;
				case ' ':
				case VK_NEXT: sendmsg = SB_PAGEDOWN; break;
				case '\r':
				case VK_DOWN: sendmsg = SB_LINEDOWN; break;
				case VK_HOME: sendmsg = SB_TOP; break;
				case VK_END: sendmsg = SB_BOTTOM; break;
			}

			if (sendmsg != 0xFFFF)
				SendMessage(hwnd, WM_VSCROLL, MAKELONG(sendmsg, 0), 0);

			/* ick! someone eats events instead of bubbling... not my fault! */

			break;

		default:
			break;

	}

	return moz->winproc(hwnd, msg, wParam, lParam);
}

NPError
NPP_New(NPMIMEType mime, NPP inst, uint16_t mode,
	int16_t argc, char *argn[], char *argv[], NPSavedData *saved)
{
	pdfmoz_t *moz;
	fz_context *ctx;

	ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
	if (!ctx)
		return NPERR_OUT_OF_MEMORY_ERROR;

	moz = fz_malloc(ctx, sizeof(pdfmoz_t));
	if (!moz)
		return NPERR_OUT_OF_MEMORY_ERROR;

	memset(moz, 0, sizeof(pdfmoz_t));

	moz->ctx = ctx;

	sprintf(moz->error, "MuPDF is loading the file...");

	moz->inst = inst;

	moz->arrow = LoadCursor(NULL, IDC_ARROW);
	moz->hand = LoadCursor(NULL, IDC_HAND);
	moz->wait = LoadCursor(NULL, IDC_WAIT);

	moz->dibinf = fz_malloc( moz->ctx, sizeof(BITMAPINFO) + 12);
	if (!moz->dibinf)
		return NPERR_OUT_OF_MEMORY_ERROR;

	moz->dibinf->bmiHeader.biSize = sizeof(moz->dibinf->bmiHeader);
	moz->dibinf->bmiHeader.biPlanes = 1;
	moz->dibinf->bmiHeader.biBitCount = 32;
	moz->dibinf->bmiHeader.biCompression = BI_RGB;
	moz->dibinf->bmiHeader.biXPelsPerMeter = 2834;
	moz->dibinf->bmiHeader.biYPelsPerMeter = 2834;
	moz->dibinf->bmiHeader.biClrUsed = 0;
	moz->dibinf->bmiHeader.biClrImportant = 0;
	moz->dibinf->bmiHeader.biClrUsed = 0;

	moz->graybrush = CreateSolidBrush(RGB(0x70,0x70,0x70));

	inst->pdata = moz;

	return NPERR_NO_ERROR;
}

NPError
NPP_Destroy(NPP inst, NPSavedData **saved)
{
	pdfmoz_t *moz = inst->pdata;
	int i;

	inst->pdata = NULL;

	SetWindowLongPtr(moz->hwnd, GWLP_WNDPROC, (LONG_PTR)moz->winproc);

	DeleteObject(moz->graybrush);

	DestroyCursor(moz->arrow);
	DestroyCursor(moz->hand);
	DestroyCursor(moz->wait);

	fz_free(moz->ctx, moz->dibinf);
	moz->dibinf = NULL;

	for (i = 0; i < moz->pagecount; i++)
	{
		fz_drop_link(moz->ctx, moz->pages[i].links);
		moz->pages[i].links = NULL;

		fz_drop_display_list( moz->ctx, moz->pages[i].contents);
		moz->pages[i].contents = NULL;

		fz_drop_display_list( moz->ctx, moz->pages[i].annotations);
		moz->pages[i].annotations = NULL;

		fz_drop_page(moz->ctx, moz->pages[i].page);
		moz->pages[i].page = NULL;

		fz_drop_pixmap(moz->ctx, moz->pages[i].image);
		moz->pages[i].image = NULL;
	}

	fz_free(moz->ctx, moz->pages);
	moz->pages = NULL;

	fz_drop_document(moz->ctx, moz->doc);
	moz->doc = NULL;

	fz_context *ctx = moz->ctx;
	fz_free(ctx, moz);
	moz = NULL;
	fz_drop_context(ctx);
	ctx = NULL;

	return NPERR_NO_ERROR;
}

NPError
NPP_SetWindow(NPP inst, NPWindow *npwin)
{
	pdfmoz_t *moz = inst->pdata;

	if (moz->hwnd != npwin->window)
	{
		moz->hwnd = npwin->window;
		SetWindowLongPtr(moz->hwnd, GWLP_USERDATA, (LONG_PTR)moz);
		moz->winproc = (WNDPROC)
		SetWindowLongPtr(moz->hwnd, GWLP_WNDPROC, (LONG_PTR)MozWinProc);
	}

	SetFocus(moz->hwnd);

	return NPERR_NO_ERROR;
}

NPError
NPP_NewStream(NPP inst, NPMIMEType type,
	NPStream* stream, NPBool seekable,
	uint16_t* stype)
{
	*stype = NP_ASFILE;
	return NPERR_NO_ERROR;
}

NPError
NPP_DestroyStream(NPP inst, NPStream* stream, NPReason reason)
{
	return NPERR_NO_ERROR;
}

int32_t
NPP_WriteReady(NPP inst, NPStream* stream)
{
	return 2147483647;
}

int32_t
NPP_Write(NPP inst, NPStream* stream, int32_t offset, int32_t len, void* buffer)
{
	return len;
}

void
NPP_StreamAsFile(NPP inst, NPStream* stream, const char* fname)
{
	pdfmoz_t *moz = inst->pdata;

	pdfmoz_open(moz, (char*)fname);
}

void
NPP_Print(NPP inst, NPPrint* platformPrint)
{
}

int16_t
NPP_HandleEvent(NPP inst, void* event)
{
	return 0;
}

void
NPP_URLNotify(NPP inst, const char* url,
	NPReason reason, void* notifyData)
{
}

NPError
NPP_GetValue(NPP inst, NPPVariable variable, void *value)
{
	return NPERR_NO_ERROR;
}

NPError
NPP_SetValue(NPP inst, NPNVariable variable, void *value)
{
	return NPERR_NO_ERROR;
}
