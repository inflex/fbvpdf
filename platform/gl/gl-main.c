#include "gl-app.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "mupdf/keyboard.h"
#include "mupdf/ddi.h"
#include "mupdf/pdf.h" /* for pdf specifics and forms */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FL __FILE__, __LINE__
#ifndef _WIN32
#include <unistd.h> // for fork and exec
#else
#include <tlhelp32.h> // for getppid()
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h> // for NSGetExecutablePath
#endif

#define PDFK_HELP 0
#define PDFK_SEARCH 1
#define PDFK_SEARCH_NEXT 2
#define PDFK_SEARCH_NEXT_PAGE 102
#define PDFK_SEARCH_PREV 3
#define PDFK_SEARCH_PREV_PAGE 103
#define PDFK_PAN_UP 4
#define PDFK_PAN_DOWN 5
#define PDFK_PAN_LEFT 6
#define PDFK_PAN_RIGHT 7
#define PDFK_PGUP 8
#define PDFK_PGUP_10 20
#define PDFK_PGDN 9
#define PDFK_PGDN_10 21
#define PDFK_ROTATE_CW 10
#define PDFK_ROTATE_CCW 11
#define PDFK_ZOOMIN 12
#define PDFK_ZOOMOUT 13
#define PDFK_FITWIDTH 14
#define PDFK_FITHEIGHT 15
#define PDFK_FITWINDOW 16
#define PDFK_GOPAGE 17
#define PDFK_GOENDPAGE 18
#define PDFK_PASTE 19


#define PDFK_QUIT 100

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Surface *sdlSurface;
SDL_Texture *sdlTexture;
SDL_Event sdlEvent;
SDL_GLContext glcontext;

enum {
	/* Screen furniture: aggregate size of unusable space from title bars, task bars, window borders, etc */
	SCREEN_FURNITURE_W = 20,
	SCREEN_FURNITURE_H = 40,

	/* Default EPUB/HTML layout dimensions */
	DEFAULT_LAYOUT_W  = 450,
	DEFAULT_LAYOUT_H  = 600,
	DEFAULT_LAYOUT_EM = 12,

	/* Default UI sizes */
	DEFAULT_UI_FONTSIZE   = 18,
	DEFAULT_UI_BASELINE   = 14,
	DEFAULT_UI_LINEHEIGHT = 22,
};

struct ui ui;
fz_context *ctx = NULL;
struct ddi_s ddi;

/* OpenGL capabilities */
static int has_ARB_texture_non_power_of_two = 1;
//static GLint max_texture_size               = 8192;
static GLint max_texture_size               = 65536;

#include <stdio.h>

#ifdef __WIN32

#include <windows.h>
#else

#endif
void getexepath( char *fullpath, size_t bs ) {
#ifdef __WIN32
	char s[4096];

     // When NULL is passed to GetModuleHandle, the handle of the exe itself is returned
     HMODULE hModule = GetModuleHandle(NULL);
     if (hModule != NULL) {
		  char *p;
         // Use GetModuleFileName() with module handle to get the path
         GetModuleFileNameA(hModule, s, sizeof(s));
//		  s[length] = '\0';
			 p = strrchr(s, '\\');
			 if (p) *(p+1) = '\0';
			 snprintf(fullpath, bs, "%s", s);
     } else {
		  //flog("exe path is empty");
     }
#endif

#ifdef __linux__
    ssize_t length;
	 char s[4096];
	 char *p;

    /*
     * /proc/self is a symbolic link to the process-ID subdir of /proc, e.g.
     * /proc/4323 when the pid of the process of this program is 4323.
     * Inside /proc/<pid> there is a symbolic link to the executable that is
     * running as this <pid>.  This symbolic link is called "exe". So if we
     * read the path where the symlink /proc/self/exe points to we have the
     * full path of the executable. 
     */

    length = readlink("/proc/self/exe", s, sizeof(s));

    /*
     * Catch some errors: 
     */
    if (length < 0) {
        //flog("Error resolving symlink /proc/self/exe trying to find full binary path.");
		  return;
    }
    if (length >= (ssize_t)sizeof(s)) {
        //flog("Exe Path too long.\n");
		  return;
    }

    s[length] = '\0';
	 p = strrchr(s, '/');
	 if (p) *(p+1) = '\0';
	 snprintf(fullpath, bs, "%s", s);
#endif

#ifdef __APPLE__
    int ret;
    pid_t pid; 
    char pathbuf[4096], *p;
	 uint32_t pbs = sizeof(pathbuf);

	_NSGetExecutablePath(pathbuf, &pbs);
	p = strcasestr(pathbuf, "fbvpdf.app");
	if (p) *p = '\0';
	snprintf(fullpath, bs, "%s", pathbuf);
	return;
#endif

}


// Menu handling function declaration
void menucb(int mitem) {}

static void ui_begin(void) {
	ui.hot = NULL;
}

static void ui_end(void) {
	if (!ui.down && !ui.middle && !ui.right) ui.active = NULL;
}

static void open_browser(const char *uri) {
#ifdef _WIN32
	ShellExecuteA(NULL, "open", uri, 0, 0, SW_SHOWNORMAL);
#else
	const char *browser = getenv("BROWSER");
	if (!browser) {
#ifdef __APPLE__
		browser = "open";
#else
		browser = "xdg-open";
#endif
	}
	if (fork() == 0) {
		execlp(browser, browser, uri, (char *)0);
		fprintf(stderr, "cannot exec '%s'\n", browser);
		exit(0);
	}
#endif
}

const char *ogl_error_string(GLenum code) {
#define CASE(E) \
	case E: return #E; break
			  switch (code) {
				  /* glGetError */
				  CASE(GL_NO_ERROR);
				  CASE(GL_INVALID_ENUM);
				  CASE(GL_INVALID_VALUE);
				  CASE(GL_INVALID_OPERATION);
				  CASE(GL_OUT_OF_MEMORY);
				  CASE(GL_STACK_UNDERFLOW);
				  CASE(GL_STACK_OVERFLOW);
				  default: return "(unknown)";
			  }
#undef CASE
}

void ogl_assert(fz_context *ctx, const char *msg) {
	int code = glGetError();
	if (code != GL_NO_ERROR) {
		fz_warn(ctx, "glGetError(%s): %s", msg, ogl_error_string(code));
	}
}

void ui_draw_image(struct texture *tex, float x, float y) {
//	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_ONE, GL_DST_COLOR);
//	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, tex->id);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_TRIANGLE_STRIP);
	{
		glColor4f(1, 1, 1, 1);
		glTexCoord2f(0, tex->t);
		glVertex2f(x + tex->x, y + tex->y + tex->h);
		glTexCoord2f(0, 0);
		glVertex2f(x + tex->x, y + tex->y);
		glTexCoord2f(tex->s, tex->t);
		glVertex2f(x + tex->x + tex->w, y + tex->y + tex->h);
		glTexCoord2f(tex->s, 0);
		glVertex2f(x + tex->x + tex->w, y + tex->y);
	}
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

static const int zoom_list[] = {18, 24, 36, 54, 72, 96, 120, 144, 180, 216, 288, 350, 450, 600};

#define MINRES (zoom_list[0]*retina_factor *0.75)
#define MAXRES (zoom_list[nelem(zoom_list) - 1]*retina_factor*0.75)
#define DEFRES 96

#define SEARCH_STATUS_NONE 0
#define SEARCH_STATUS_INPAGE 1
#define SEARCH_STATUS_SEEKING 2

#define DDI_SIMULATE_OPTION_NONE 0
#define DDI_SIMULATE_OPTION_SEARCH_NEXT 1
#define DDI_SIMULATE_OPTION_SEARCH_PREV 2
#define DDI_SIMULATE_OPTION_PREPROCESSED_SEARCH 3

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define SEARCH_TYPE_NONE 0
#define SEARCH_TYPE_DDI_SEQUENCE 1
#define SEARCH_TYPE_TEXT_ONLY 2
#define SEARCH_TYPE_PEDANTIC_WORD 4

#define SEARCH_MODE_NONE 0
#define SEARCH_MODE_NORMAL 1
#define SEARCH_MODE_INPAGE 2
#define SEARCH_MODE_COMPOUND 4

#define SEARCH_FLAG_NONE 0
#define SEARCH_FLAG_STANDARD 1
#define SEARCH_FLAG_STRICT 2
#define SEARCH_FLAG_NOTFOUND 4
#define SEARCH_FLAG_HEURISTICS 8

#define FLOG_FILENAME "fbvpdf.log"

static char flog_filename[4096];

struct search_s {
	char search_raw[1024];
	char alt[1024];
	char a[1024]; // cooked results from search_raw
	char b[1024];
	char c[1024];
	char prev[1024];
	int mode;
	int flags;
	int hit_count_a, hit_count_b, hit_count_c; // hit_count_a is also current page max index
	fz_rect hit_bbox_a[500];
	fz_rect hit_bbox_b[500];
	fz_rect hit_bbox_c[500];

	int page;
	int inpage_index;
	int direction;
	int not_found; // we set this to 1 when we load a new search in, set to 0 if any hit is found
	int has_hits;
	int active;
};

static struct search_s this_search;
static struct search_s prior_search;

static int drawable_x, drawable_y;
static int retina_factor = 1;
static char filename[PATH_MAX];
static char *password          = "";
static int raise_on_search     = 0;
static int detached            = 0; // if we talk back via DDI or not
static int search_heuristics   = 0;
static int scroll_wheel_swap   = 0;
static char *ddiprefix         = "fbvpdf";
static char *ddiloadstr        = NULL;
static int ddi_simulate_option = DDI_SIMULATE_OPTION_NONE;
//static int document_has_hits   = 0;
static int am_dragging         = 0;
static fz_point dragging_start;

static char *anchor           = NULL;
static float layout_w         = DEFAULT_LAYOUT_W;
static float layout_h         = DEFAULT_LAYOUT_H;
static float layout_em        = DEFAULT_LAYOUT_EM;
static char *layout_css       = NULL;
static int layout_use_doc_css = 1;

static const char *title    = "FlexBV Schematic Viewer";

static fz_document *doc     = NULL;
static fz_page *page        = NULL;
static fz_stext_page *text  = NULL;
static pdf_document *pdf    = NULL;
static fz_outline *outline  = NULL;
static fz_link *links       = NULL;

static int number = 0;
// static int show_help = 0;

static struct texture page_tex = {0};
static int scroll_x = 0, scroll_y = 0;
static int canvas_x = 0, canvas_w = 100;
static int canvas_y = 0, canvas_h = 100;

// static struct texture annot_tex[256];
// static int annot_count = 0;

static double compsearch_radius = 500.0f;
static int compsearch_highlight = 8; // 0b0111 (highlight all 3 elements)
static char *headless_data;
static int window_w = 1, window_h = 1;
static int origin_x, origin_y;

#define RUNMODE_NORMAL 0
#define RUNMODE_HEADLESS 1

static int reload_required = 0;
static int runmode         = RUNMODE_NORMAL; // 0 == standard
static int debug           = 0;
static time_t process_start_time;
static int oldinvert = 0, currentinvert = 0;
static int oldpage               = 0; //, this_search.page = 0;
static int currently_viewed_page = 0; // which page is on screen right now
static float oldzoom = DEFRES, currentzoom = DEFRES;
static float oldrotate = 0, currentrotate = 0;
static fz_matrix page_ctm, page_inv_ctm;
static int loaded = 0;

static int isfullscreen = 0;
static int showoutline  = 0;
static int showlinks    = 0;
static int showsearch_input   = 0;
static int showinfo     = 0;
static int showhelp     = 0;
static int doquit       = 0;

struct mark {
	int page;
	fz_point scroll;
};

static int history_count = 0;
static struct mark history[256];
static int future_count = 0;
static struct mark future[256];
static struct mark marks[10];

static struct input search_input = {{0}, 0};
static unsigned int next_power_of_two(unsigned int n) {
	--n;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return ++n;
}

char exepath[4096];

int flog_init(char *filename) {
	FILE *f;
	snprintf(flog_filename, sizeof(flog_filename), "%s", filename);
	f = fopen(flog_filename, "w");
	if (f) {
		fclose(f);
	}
	return 0;
}

int flog(const char *format, ...) {
	if (debug) {
		FILE *f;
		va_list args;
		va_start(args, format);

		f = fopen(flog_filename, "a");
		if (f) {
			fprintf(f, "%ld ", time(NULL));
			vfprintf(f, format, args);
			va_end(args);
			fclose(f);
		}
	}
	return 0;
}

static void update_title(void) {
	static char buf[256];
	size_t n = strlen(title);

	if (n > 50) {
		sprintf(buf, "...%s - %d / %d (R%d)", title + n - 50, currently_viewed_page + 1, fz_count_pages(ctx, doc), GIT_BUILD);
	} else {
		sprintf(buf, "%s - %d / %d (R%d)", title, currently_viewed_page + 1, fz_count_pages(ctx, doc), GIT_BUILD);
	}
	flog("%s:%d: Setting window title '%s'\r\n", FL, buf);
	SDL_SetWindowTitle(sdlWindow, buf);
}

void texture_from_pixmap(struct texture *tex, fz_pixmap *pix) {
	if (!tex->id) glGenTextures(1, &tex->id);

	glBindTexture(GL_TEXTURE_2D, tex->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	tex->x = pix->x;
	tex->y = pix->y;
	tex->w = pix->w;
	tex->h = pix->h;

	if (has_ARB_texture_non_power_of_two) {
		if (tex->w > max_texture_size || tex->h > max_texture_size)
			fz_warn(ctx, "texture size (%d x %d) exceeds implementation limit (%d)", tex->w, tex->h, max_texture_size);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, pix->n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pix->samples);
		tex->s = 1;
		tex->t = 1;
	} else {
		int w2 = next_power_of_two(tex->w);
		int h2 = next_power_of_two(tex->h);
		if (w2 > max_texture_size || h2 > max_texture_size)
			fz_warn(ctx, "texture size (%d x %d) exceeds implementation limit (%d)", w2, h2, max_texture_size);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w2, h2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, pix->n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pix->samples);
		tex->s = (float)tex->w / w2;
		tex->t = (float)tex->h / h2;
	}
}

void load_page(void) {
	fz_rect rect;
	fz_irect irect;

	fz_scale(&page_ctm, currentzoom / 72, currentzoom / 72);

	fz_pre_rotate(&page_ctm, -currentrotate);
	fz_invert_matrix(&page_inv_ctm, &page_ctm);

	fz_drop_stext_page(ctx, text);
	text = NULL;
	fz_drop_link(ctx, links);
	links = NULL;
	fz_drop_page(ctx, page);
	page = NULL;

	currently_viewed_page = fz_clampi(currently_viewed_page, 0, fz_count_pages(ctx, doc) - 1);
	page                  = fz_load_page(ctx, doc, currently_viewed_page);
	links                 = fz_load_links(ctx, page);
	text                  = fz_new_stext_page_from_page(ctx, page, NULL);

	/* compute bounds here for initial window size */
	fz_bound_page(ctx, page, &rect);
	fz_transform_rect(&rect, &page_ctm);
	fz_round_rect(&irect, &rect);
	page_tex.w = irect.x1 - irect.x0;
	page_tex.h = irect.y1 - irect.y0;

	loaded = 1;
}

void render_page(void) {
	// fz_annot *annot;
	fz_pixmap *pix;

	if (!loaded) load_page();

	pix = fz_new_pixmap_from_page_contents(ctx, page, &page_ctm, fz_device_rgb(ctx), 0);
	// pix = fz_new_pixmap_from_page_contents(ctx, currently_viewed_page, &page_ctm, fz_device_rgb(ctx), 0);
	if (currentinvert) {
		fz_invert_pixmap(ctx, pix);
		fz_gamma_pixmap(ctx, pix, 1 / 1.4f);
	}

	texture_from_pixmap(&page_tex, pix);
	fz_drop_pixmap(ctx, pix);

	/*
		annot_count = 0;
		for (annot = fz_first_annot(ctx, page); annot; annot = fz_next_annot(ctx, annot))
		{
		pix = fz_new_pixmap_from_annot(ctx, annot, &page_ctm, fz_device_rgb(ctx), 1);
		texture_from_pixmap(&annot_tex[annot_count++], pix);
		fz_drop_pixmap(ctx, pix);
		if (annot_count >= nelem(annot_tex))
		{
		fz_warn(ctx, "too many annotations to display!");
		break;
		}
		}
		*/

	loaded = 0;
}

static struct mark save_mark() {
	struct mark mark;
	mark.page     = currently_viewed_page;
	mark.scroll.x = scroll_x;
	mark.scroll.y = scroll_y;
	fz_transform_point(&mark.scroll, &page_inv_ctm);
	return mark;
}

static void restore_mark(struct mark mark) {
	currently_viewed_page = mark.page;
	fz_transform_point(&mark.scroll, &page_ctm);
	scroll_x = mark.scroll.x;
	scroll_y = mark.scroll.y;
}

static void push_history(void) {
	if (history_count + 1 >= nelem(history)) {
		memmove(history, history + 1, sizeof *history * (nelem(history) - 1));
		history[history_count] = save_mark();
	} else {
		history[history_count++] = save_mark();
	}
}

static void push_future(void) {
	if (future_count + 1 >= nelem(future)) {
		memmove(future, future + 1, sizeof *future * (nelem(future) - 1));
		future[future_count] = save_mark();
	} else {
		future[future_count++] = save_mark();
	}
}

static void clear_future(void) {
	future_count = 0;
}

static void jump_to_page(int newpage) {
	newpage = fz_clampi(newpage, 0, fz_count_pages(ctx, doc) - 1);
	clear_future();
	push_history();
	this_search.page      = newpage;
	currently_viewed_page = newpage;
	push_history();
}

static void jump_to_page_xy(int newpage, float x, float y) {
	flog("%s:%d: Jumping to %d[%f %f]\r\n", FL, newpage, x, y);
	fz_point p = {x, y};
	newpage    = fz_clampi(newpage, 0, fz_count_pages(ctx, doc) - 1);
	fz_transform_point(&p, &page_ctm);
	clear_future();
	push_history();
	currently_viewed_page = newpage;
	scroll_x              = p.x;
	scroll_y              = p.y;
	push_history();
}

static void pop_history(void) {
	int here = currently_viewed_page;
	push_future();
	while (history_count > 0 && currently_viewed_page == here) restore_mark(history[--history_count]);
}

static void pop_future(void) {
	int here = currently_viewed_page;
	push_history();
	while (future_count > 0 && currently_viewed_page == here) restore_mark(future[--future_count]);
	push_history();
}

static void ui_label_draw(int x0, int y0, int x1, int y1, const char *text) {
	glColor4f(1, 1, 1, 1);
	glRectf(x0, y0, x1, y1);
	glColor4f(0, 0, 0, 1);
	ui_draw_string(ctx, x0 + 2, y0 + 2 + ui.baseline, text);
}

static void ui_scrollbar(int x0, int y0, int x1, int y1, int *value, int page_size, int max) {
	static float saved_top = 0;
	static int saved_ui_y  = 0;
	float top;

	int total_h = y1 - y0;
	int thumb_h = fz_maxi(x1 - x0, total_h * page_size / max);
	int avail_h = total_h - thumb_h;

	max -= page_size;

	if (max <= 0) {
		*value = 0;
		glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
		glRectf(x0, y0, x1, y1);
		return;
	}

	top = (float)*value * avail_h / max;

	if (ui.down && !ui.active) {
		if (ui.x >= x0 && ui.x < x1 && ui.y >= y0 && ui.y < y1) {
			if (ui.y < top) {
				ui.active = "pgdn";
				*value -= page_size;
			} else if (ui.y >= top + thumb_h) {
				ui.active = "pgup";
				*value += page_size;
			} else {
				ui.hot     = value;
				ui.active  = value;
				saved_top  = top;
				saved_ui_y = ui.y;
			}
		}
	}

	if (ui.active == value) {
		*value = (saved_top + ui.y - saved_ui_y) * max / avail_h;
	}

	if (*value < 0)
		*value = 0;
	else if (*value > max)
		*value = max;

	top = (float)*value * avail_h / max;

	glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
	glRectf(x0, y0, x1, y1);
	glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
	glRectf(x0, top, x1, top + thumb_h);
}

static int measure_outline_height(fz_outline *node) {
	int h = 0;
	while (node) {
		h += ui.lineheight;
		if (node->down) h += measure_outline_height(node->down);
		node = node->next;
	}
	return h;
}

static int do_outline_imp(fz_outline *node, int end, int x0, int x1, int x, int y) {
	int h = 0;
	int p = currently_viewed_page;
	int n = end;

	while (node) {
		p = node->page;
		if (p >= 0) {
			if (ui.x >= x0 && ui.x < x1 && ui.y >= y + h && ui.y < y + h + ui.lineheight) {
				ui.hot = node;
				if (!ui.active && ui.down) {
					ui.active = node;
					jump_to_page_xy(p, node->x, node->y);
				}
			}

			n = end;
			if (node->next && node->next->page >= 0) {
				n = node->next->page;
			}
			if (currently_viewed_page == p || (currently_viewed_page > p && currently_viewed_page < n)) {
				glColor4f(0.9f, 0.9f, 0.9f, 1.0f);
				glRectf(x0, y + h, x1, y + h + ui.lineheight);
			}
		}

		glColor4f(0, 0, 0, 1);
		ui_draw_string(ctx, x, y + h + ui.baseline, node->title);
		h += ui.lineheight;
		if (node->down) h += do_outline_imp(node->down, n, x0, x1, x + ui.lineheight, y + h);

		node = node->next;
	}
	return h;
}

static void do_outline(fz_outline *node, int outline_w) {
	static char *id                   = "outline";
	static int outline_scroll_y       = 0;
	static int saved_outline_scroll_y = 0;
	static int saved_ui_y             = 0;

	int outline_h;
	int total_h;

	outline_w -= ui.lineheight;
	outline_h = window_h;
	total_h   = measure_outline_height(outline);

	if (ui.x >= 0 && ui.x < outline_w && ui.y >= 0 && ui.y < outline_h) {
		ui.hot = id;
		if (!ui.active && ui.middle) {
			ui.active              = id;
			saved_ui_y             = ui.y;
			saved_outline_scroll_y = outline_scroll_y;
		}
	}

	if (ui.active == id) outline_scroll_y = saved_outline_scroll_y + (saved_ui_y - ui.y) * 5;

	if (ui.hot == id) outline_scroll_y -= ui.scroll_y * ui.lineheight * 3;

	ui_scrollbar(outline_w, 0, outline_w + ui.lineheight, outline_h, &outline_scroll_y, outline_h, total_h);

	glScissor(0, 0, outline_w, outline_h);
	glEnable(GL_SCISSOR_TEST);

	glColor4f(1, 1, 1, 1);
	glRectf(0, 0, outline_w, outline_h);

	do_outline_imp(outline, fz_count_pages(ctx, doc), 0, outline_w, 10, -outline_scroll_y);

	glDisable(GL_SCISSOR_TEST);
}

static void do_links(fz_link *link, int xofs, int yofs) {
	fz_rect r;
	float x, y;
	float link_x, link_y;

	x = ui.x;
	y = ui.y;

	xofs -= page_tex.x;
	yofs -= page_tex.y;

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	while (link) {

		if (fz_is_external_link(ctx, link->uri)) {
			link = link->next;
			continue;
		}

		r = link->rect;
		fz_transform_rect(&r, &page_ctm);

		if (x >= xofs + r.x0 && x < xofs + r.x1 && y >= yofs + r.y0 && y < yofs + r.y1) {
			ui.hot = link;
			if (!ui.active && ui.down) ui.active = link;
		}

		if (ui.hot == link || showlinks) {
			if (ui.active == link && ui.hot == link)
				glColor4f(0, 0, 1, 0.4f);
			else if (ui.hot == link)
				glColor4f(0, 0, 1, 0.2f);
			else
				glColor4f(0, 0, 1, 0.1f);
			glRectf(xofs + r.x0, yofs + r.y0, xofs + r.x1, yofs + r.y1);
		}

		if (ui.active == link && !ui.down) {
			if (ui.hot == link) {
				if (fz_is_external_link(ctx, link->uri))
					open_browser(link->uri);
				else {
					int p = fz_resolve_link(ctx, doc, link->uri, &link_x, &link_y);
					if (p >= 0)
						jump_to_page_xy(p, link_x, link_y);
					else
						fz_warn(ctx, "cannot find link destination '%s'", link->uri);
				}
			}
		}

		link = link->next;
	}

	glDisable(GL_BLEND);
}

static void do_page_selection(int x0, int y0, int x1, int y1) {
	static fz_point pt = {0, 0};
	fz_rect hits[1000];
	int i, n;

	if (ui.x >= x0 && ui.x < x1 && ui.y >= y0 && ui.y < y1) {
		ui.hot = &pt;
		if (!ui.active && ui.right) {
			ui.active = &pt;
			pt.x      = ui.x;
			pt.y      = ui.y;
		}
	}

	if (ui.active == &pt) {
		int xofs = x0 - page_tex.x;
		int yofs = y0 - page_tex.y;

		fz_point page_a = {pt.x - xofs, pt.y - yofs};
		fz_point page_b = {ui.x - xofs, ui.y - yofs};

		fz_transform_point(&page_a, &page_inv_ctm);
		fz_transform_point(&page_b, &page_inv_ctm);

		n = fz_highlight_selection(ctx, text, page_a, page_b, hits, nelem(hits));

		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO); /* invert destination color */
		glEnable(GL_BLEND);

		glColor4f(1, 1, 1, 1);
		for (i = 0; i < n; ++i) {
			fz_transform_rect(&hits[i], &page_ctm);
			glRectf(hits[i].x0 + xofs, hits[i].y0 + yofs, hits[i].x1 + 1 + xofs, hits[i].y1 + 1 + yofs);
		}

		glDisable(GL_BLEND);

		if (!ui.right) {
			char *s;
#ifdef _WIN32
			s = fz_copy_selection(ctx, text, page_a, page_b, 1);
#else
			s = fz_copy_selection(ctx, text, page_a, page_b, 0);
#endif
			ui_set_clipboard(s);

			if (ui.lctrl||ui.rctrl) {
				flog("%s:%d: Searching internally for '%s'\r\n", FL, s);
				snprintf(this_search.a, sizeof(this_search.a), "%s", s);
				this_search.direction = 1;
				this_search.page = 0;
				this_search.has_hits = 0;
				this_search.not_found = 0;
				this_search.active = 1;

			} else {
				flog("%s:%d: Dispatching request '%s'\n", FL, s);
				if (!detached) DDI_dispatch(&ddi, s);
			}
			fz_free(ctx, s);
		}
	}
}


/*
 * This is where we colour the search hits on a page
 *
 */
static void do_search_hits(int xofs, int yofs) {
	fz_rect r;
	int i;

	xofs -= page_tex.x;
	yofs -= page_tex.y;


	for (i = 0; i < this_search.hit_count_a; ++i) {
		r = this_search.hit_bbox_a[i];

		fz_transform_rect(&r, &page_ctm);

		if (i == this_search.inpage_index) {
			float offset = 5.0f;
			float loffset = 6.0f;
			/*
			 * highlight the current hit
			 *
			 */


//			glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO); /* invert destination color */
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glColor4f(1, 0.0, 0.2, 0.5f);
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			glRectf(xofs + r.x0 -offset, yofs + r.y0 -offset, xofs + r.x1 +offset, yofs + r.y1 +offset);
			glDisable(GL_BLEND);
	/*
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glLineWidth(3.0f);
			glColor4f(0, 0, 0, 0.20f);
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
			glRectf(xofs + r.x0 -loffset, yofs + r.y0 -loffset, xofs + r.x1 +loffset, yofs + r.y1 +loffset);
			glLineWidth(1.0f);
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			glDisable(GL_BLEND);
			*/

		} else {
			/*
			 * standard faded for other hits
			 *
			 */
		//	glColor4f(1, 0, 0, 0.1f);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glColor4f(1, 0.2, 0.2, 0.3f);
			glRectf(xofs + r.x0, yofs + r.y0, xofs + r.x1, yofs + r.y1);
			glDisable(GL_BLEND);
		}
//		glRect(xofs + r.x0 -2, yofs + r.y0 -2, xofs + r.x1 +2, yofs + r.y1 +2);
	}

}

static void toggle_fullscreen(void) {
	static int win_x = 0, win_y = 0;
	static int win_w = 100, win_h = 100;
	if (!isfullscreen) {
		SDL_GetWindowSize(sdlWindow, &win_w, &win_h);
		SDL_GetWindowPosition(sdlWindow, &win_x, &win_y);
		SDL_SetWindowFullscreen(sdlWindow, 0);
		isfullscreen = 1;
	} else {
		SDL_SetWindowPosition(sdlWindow, win_x, win_y);
		SDL_SetWindowSize(sdlWindow, win_w, win_h);
		isfullscreen = 0;
	}
}

static void shrinkwrap(void) {
	int x, y;
	int screen_w, screen_h;
	int w, h;

	SDL_GetWindowSize(sdlWindow, &x, &y);
	screen_w = x - SCREEN_FURNITURE_W;
	screen_h = y - SCREEN_FURNITURE_H;
	w        = page_tex.w + canvas_x;
	h        = page_tex.h + canvas_y;

	if (screen_w > 0 && w > screen_w) w = screen_w;
	if (screen_h > 0 && h > screen_h) h = screen_h;
	if (isfullscreen) toggle_fullscreen();
	// glutReshapeWindow(w, h);
	SDL_SetWindowSize(sdlWindow, w, h);
}

static void load_document(void) {
	fz_drop_outline(ctx, outline);
	fz_drop_document(ctx, doc);

	doc = fz_open_document(ctx, filename);
	if (fz_needs_password(ctx, doc)) {
		if (!fz_authenticate_password(ctx, doc, password)) {
			fprintf(stderr, "Invalid password.\n");
			exit(1);
		}
	}

	fz_layout_document(ctx, doc, layout_w, layout_h, layout_em);

	fz_try(ctx) outline   = fz_load_outline(ctx, doc);
	fz_catch(ctx) outline = NULL;

	pdf = pdf_specifics(ctx, doc);
	if (pdf) {
		pdf_enable_js(ctx, pdf);
		if (anchor) currently_viewed_page = pdf_lookup_anchor(ctx, pdf, anchor, NULL, NULL);
	} else {
		if (anchor) currently_viewed_page = fz_atoi(anchor) - 1;
	}
	anchor = NULL;

	currently_viewed_page = fz_clampi(currently_viewed_page, 0, fz_count_pages(ctx, doc) - 1);
}

static void reload(void) {
	load_document();
	if (runmode != RUNMODE_HEADLESS) render_page();
	update_title();
}

static void toggle_outline(void) {
	if (outline) {
		showoutline = !showoutline;
		if (showoutline)
			canvas_x = ui.lineheight * 16;
		else
			canvas_x = 0;
		if (canvas_w == page_tex.w && canvas_h == page_tex.h) shrinkwrap();
	}
}

static void auto_zoom_w(void) {
	scroll_x = scroll_y = 0;
	currentzoom         = fz_clamp(currentzoom * canvas_w / page_tex.w, MINRES, MAXRES);
}

static void auto_zoom_h(void) {
	scroll_x = scroll_y = 0;
	currentzoom         = fz_clamp(currentzoom * canvas_h / page_tex.h, MINRES, MAXRES);
}

static void auto_zoom(void) {
	float page_a   = (float)page_tex.w / page_tex.h;
	float screen_a = (float)canvas_w / canvas_h;
	if (page_a > screen_a)
		auto_zoom_w();
	else
		auto_zoom_h();
}

static void smart_move_backward(void) {
	if (scroll_y <= 0) {
		if (scroll_x <= 0) {
			if (currently_viewed_page - 1 >= 0) {
				scroll_x = page_tex.w;
				scroll_y = page_tex.h;
				currently_viewed_page -= 1;
			}
		} else {
			scroll_y = page_tex.h;
			scroll_x -= canvas_w * 9 / 10;
		}
	} else {
		scroll_y -= canvas_h * 9 / 10;
	}
}

static void smart_move_forward(void) {
	if (scroll_y + canvas_h >= page_tex.h) {
		if (scroll_x + canvas_w >= page_tex.w) {
			if (currently_viewed_page + 1 < fz_count_pages(ctx, doc)) {
				scroll_x = 0;
				scroll_y = 0;
				currently_viewed_page += 1;
			}
		} else {
			scroll_y = 0;
			scroll_x += canvas_w * 9 / 10;
		}
	} else {
		scroll_y += canvas_h * 9 / 10;
	}
}

static void quit(void) {
	doquit = 1;
}

static void clear_search(void) {
	memset(&this_search, 0, sizeof(this_search));
	this_search.not_found = 0;
	this_search.mode = SEARCH_MODE_NONE;
}

void ui_set_keypress( int idx ) {

	ui.key = 0;
	ui.mod = 0;
	if (keyboard_map[idx].mods & KEYB_MOD_SHIFT) ui.mod |= KMOD_SHIFT;
	if (keyboard_map[idx].mods & KEYB_MOD_CTRL) ui.mod |= KMOD_CTRL;
	if (keyboard_map[idx].mods & KEYB_MOD_ALT) ui.mod |= KMOD_ALT;
	if (keyboard_map[idx].mods & KEYB_MOD_OS) ui.mod |= KMOD_GUI;

	ui.scancode = keyboard_map[idx].key;
}

int IsKeyPressed ( int index ) {

	if (ui.scancode == keyboard_map[index].key ) {
		uint8_t modmap = 0;

		if (ui.mod & KMOD_SHIFT) modmap |= KEYB_MOD_SHIFT;
		if (ui.mod & KMOD_CTRL) modmap |= KEYB_MOD_CTRL;
		if (ui.mod & KMOD_ALT) modmap |= KEYB_MOD_ALT;
		if (ui.mod & KMOD_GUI) modmap |= KEYB_MOD_OS;

		//flog("%s:%d: IsKeyPressed(): looking for %x, currently %x\n", FL, keyboard_map[index].mods, modmap);
		if (keyboard_map[index].mods == modmap) {
			ui.mod = 0;
			ui.scancode = 0;
			ui.key = 0;
			return 1;
		}
	}
	return 0;
}

static void do_keypress(void) {

	ui.plain = 1;
	//	ui.mod   = SDL_GetModState();


	/*
	 * close the help/info if we've pressed something else
	 *
	 */
	if (ui.down || ui.middle || ui.right || ui.key) showinfo = showhelp = 0;


	if (!showsearch_input) {

		if (IsKeyPressed(PDFK_HELP)) {
			showhelp = 1;
			return;
		}

		if (IsKeyPressed(PDFK_SEARCH)) {
			clear_search();
			this_search.direction = 1;
			showsearch_input            = 1;
			this_search.not_found = 0;
			this_search.has_hits = 0;
			search_input.text[0] = 0;
			search_input.end     = search_input.text;
			search_input.p       = search_input.text;
			search_input.q       = search_input.end;
			update_title();
			return;
		}


		if (IsKeyPressed(PDFK_SEARCH_NEXT)) {
			if (strlen(this_search.a)) {
				this_search.active         = 1;
				this_search.direction = 1;
				this_search.inpage_index++;
				flog("%s:%d: search_next ->  inpage_index=%d, hit count=%d\n", FL, this_search.inpage_index, this_search.hit_count_a);
				if (this_search.inpage_index >= this_search.hit_count_a) {
					flog("%s:%d: Resetting to next page\n", FL);
					this_search.inpage_index = -1;
					this_search.page++;
					if (this_search.page > fz_count_pages(ctx, doc)) {
						this_search.page = 0;
					}
				}
			}
			return;
		}

		if (IsKeyPressed(PDFK_SEARCH_PREV)) {
			if (strlen(this_search.a)) {
				this_search.direction = -1;
				this_search.active         = 1;
				flog("%s:%d: search_prev ->  page=%d inpage_index=%d, hit count=%d\n", FL, this_search.page+1, this_search.inpage_index, this_search.hit_count_a);
				if (this_search.inpage_index < 1) {
					flog("%s:%d: Resetting to prev page\n", FL);
					this_search.inpage_index = -1;
					this_search.page--;
					flog("%s:%d: New page=%d index=%d\n", FL, this_search.page+1, this_search.inpage_index);
					if (this_search.page < 0) {
						this_search.page = fz_count_pages(ctx,doc) -1;
					}
				} else {
					this_search.inpage_index--;
				}
			}
			return;
		}

		if (IsKeyPressed(PDFK_SEARCH_NEXT_PAGE)) {
			if (strlen(this_search.a)) {
				this_search.active         = 1;
				this_search.direction = 1;
				this_search.inpage_index = -1;
				this_search.page++;
				if (this_search.page > fz_count_pages(ctx, doc)) {
					this_search.page = 0;
				}
			}
			return;
		}

		if (IsKeyPressed(PDFK_SEARCH_PREV_PAGE)) {
			if (strlen(this_search.a)) {
				this_search.direction = -1;
				this_search.active         = 1;
				this_search.inpage_index = 0;
				this_search.page--;
				if (this_search.page < 0) {
					this_search.page = fz_count_pages(ctx,doc) -1;
				}
			}
			return;
		}

		if (IsKeyPressed(PDFK_ZOOMIN)) { currentzoom *= 1.25; }
		if (IsKeyPressed(PDFK_ZOOMOUT)) { currentzoom /= 1.25; }

		if (IsKeyPressed(PDFK_ROTATE_CW)) { currentrotate += 90; }
		if (IsKeyPressed(PDFK_ROTATE_CCW)) { currentrotate -= 90; }

		if (IsKeyPressed(PDFK_PGUP)) { currently_viewed_page -= fz_maxi(number, 1); }
		if (IsKeyPressed(PDFK_PGDN)) { currently_viewed_page += fz_maxi(number, 1); }
		if (IsKeyPressed(PDFK_PGUP_10)) { currently_viewed_page -= fz_maxi(number, 10); }
		if (IsKeyPressed(PDFK_PGDN_10)) { currently_viewed_page += fz_maxi(number, 10); }

		if (IsKeyPressed(PDFK_FITWIDTH)) { auto_zoom_w(); }
		if (IsKeyPressed(PDFK_FITWINDOW)) { auto_zoom(); }
		if (IsKeyPressed(PDFK_FITHEIGHT)) { auto_zoom_h(); }

		if (IsKeyPressed(PDFK_GOENDPAGE)) {
			flog("%s:%d: Jump to page '%d'\r\n", FL, fz_count_pages(ctx, doc) - 1);
			jump_to_page(fz_count_pages(ctx, doc) - 1);
			return;
		}
		if (IsKeyPressed(PDFK_GOPAGE)) {
			flog("%s:%d: Jump to page '%d'\r\n", FL, number - 1);
			jump_to_page(number - 1);
			return;
		}


		if (IsKeyPressed(PDFK_QUIT)) { quit(); }
	} // if we're not in the UI search field input mode.

	if (IsKeyPressed(PDFK_PASTE)) {
		if (showsearch_input) {
			snprintf(search_input.text, sizeof(search_input.text), "%s", SDL_GetClipboardText());
			search_input.end = search_input.q = search_input.text +strlen(search_input.text);
			search_input.p = search_input.q;
		}
		return;
	}

	if (!ui.focus && ui.key && ui.plain) {
		flog("%s:%d: Acting on key '0x%08x' '%c' (mod='%0x')\r\n", FL, ui.key, ui.key, ui.mod);
		switch (ui.key) {
			case SDLK_ESCAPE: clear_search(); break;

		}

		if (ui.key >= '0' && ui.key <= '9') {
			number = number * 10 + ui.key - '0';
			flog("%s:%d: number = %d\r\n", FL, number);
		} else {
			number = 0;
		}

		currently_viewed_page = fz_clampi(currently_viewed_page, 0, fz_count_pages(ctx, doc) - 1);
		currentzoom           = fz_clamp(currentzoom, MINRES, MAXRES);
		while (currentrotate < 0) currentrotate += 360;
		while (currentrotate >= 360) currentrotate -= 360;

		ui.key = 0; /* we ate the key event, so zap it */
	}
}

static int do_info_line(int x, int y, char *label, char *text) {
	char buf[512];
	fz_snprintf(buf, sizeof buf, "%s: %s", label, text);
	ui_draw_string(ctx, x, y, buf);
	return y + ui.lineheight;
}

static void do_info(void) {
	char buf[256];

	int x = canvas_x + 4 * ui.lineheight;
	int y = canvas_y + 4 * ui.lineheight;
	int w = canvas_w - 8 * ui.lineheight;
	int h = 9 * ui.lineheight;

	glBegin(GL_TRIANGLE_STRIP);
	{
		glColor4f(0.9f, 0.9f, 0.9f, 1.0f);
		glVertex2f(x, y);
		glVertex2f(x, y + h);
		glVertex2f(x + w, y);
		glVertex2f(x + w, y + h);
	}
	glEnd();

	x += ui.lineheight;
	y += ui.lineheight + ui.baseline;

	glColor4f(0, 0, 0, 1);
	if (fz_lookup_metadata(ctx, doc, FZ_META_INFO_TITLE, buf, sizeof buf) > 0) y = do_info_line(x, y, "Title", buf);
	if (fz_lookup_metadata(ctx, doc, FZ_META_INFO_AUTHOR, buf, sizeof buf) > 0) y = do_info_line(x, y, "Author", buf);
	if (fz_lookup_metadata(ctx, doc, FZ_META_FORMAT, buf, sizeof buf) > 0) y = do_info_line(x, y, "Format", buf);
	if (fz_lookup_metadata(ctx, doc, FZ_META_ENCRYPTION, buf, sizeof buf) > 0) y = do_info_line(x, y, "Encryption", buf);
	if (pdf_specifics(ctx, doc)) {
		if (fz_lookup_metadata(ctx, doc, "info:Creator", buf, sizeof buf) > 0) y = do_info_line(x, y, "PDF Creator", buf);
		if (fz_lookup_metadata(ctx, doc, "info:Producer", buf, sizeof buf) > 0) y = do_info_line(x, y, "PDF Producer", buf);
		buf[0] = 0;
		if (fz_has_permission(ctx, doc, FZ_PERMISSION_PRINT)) fz_strlcat(buf, "print, ", sizeof buf);
		if (fz_has_permission(ctx, doc, FZ_PERMISSION_COPY)) fz_strlcat(buf, "copy, ", sizeof buf);
		if (fz_has_permission(ctx, doc, FZ_PERMISSION_EDIT)) fz_strlcat(buf, "edit, ", sizeof buf);
		if (fz_has_permission(ctx, doc, FZ_PERMISSION_ANNOTATE)) fz_strlcat(buf, "annotate, ", sizeof buf);
		if (strlen(buf) > 2)
			buf[strlen(buf) - 2] = 0;
		else
			fz_strlcat(buf, "none", sizeof buf);
		y = do_info_line(x, y, "Permissions", buf);
	}
}

static int do_status_footer(void) {
	char s[1024];
	char ss[1024];

	int x = canvas_x; // + 1 * ui.lineheight;
	int w = canvas_w;
	int h = 1.125 * ui.lineheight;
	int y = canvas_h - h; // + 1 * ui.lineheight;

	ss[0] = 0;
	s[0]  = 0;

	if ((this_search.hit_count_a) > 0) { //&& (last_search_string[0]))
		char a[20],b[20],c[20], d[20];
		snprintf(ss,
				sizeof(ss),
				"Searching '%s'. %d of %d hit(s) on current page [ %s/%s = Next item (page), %s/%s = Prev item (page), ESC = Clear ]"
				, this_search.a
				, this_search.inpage_index+1
				, this_search.hit_count_a
				, KEYB_combo_to_string(a, sizeof(a), keyboard_map[PDFK_SEARCH_NEXT])
				, KEYB_combo_to_string(b, sizeof(b), keyboard_map[PDFK_SEARCH_NEXT_PAGE])
				, KEYB_combo_to_string(c, sizeof(c), keyboard_map[PDFK_SEARCH_PREV])
				, KEYB_combo_to_string(d, sizeof(d), keyboard_map[PDFK_SEARCH_PREV_PAGE])
				);
	} else if (this_search.not_found) {
		snprintf(ss, sizeof(ss), "Search not found '%s' [ Press ESC to clear ]", this_search.a);

	} else {
		char a[20],b[20],c[20];
		char d[20],e[20],f[20],g[20];
		snprintf(ss, sizeof(ss), "[ F1 = HELP | Search=%s, Next=%s, Prev=%s | PgUp=%s, PgDn=%s | Rotate CW=%s, CCW=%s ]"
				, KEYB_combo_to_string(a, sizeof(a), keyboard_map[PDFK_SEARCH])
				, KEYB_combo_to_string(d, sizeof(d), keyboard_map[PDFK_SEARCH_NEXT])
				, KEYB_combo_to_string(e, sizeof(e), keyboard_map[PDFK_SEARCH_PREV])
				, KEYB_combo_to_string(b, sizeof(b), keyboard_map[PDFK_PGUP])
				, KEYB_combo_to_string(c, sizeof(c), keyboard_map[PDFK_PGDN])
				, KEYB_combo_to_string(f, sizeof(f), keyboard_map[PDFK_ROTATE_CW])
				, KEYB_combo_to_string(g, sizeof(g), keyboard_map[PDFK_ROTATE_CCW])
				);
	}

	snprintf(s, sizeof(s), "V.%d | Page %d of %d. %s", GIT_BUILD, currently_viewed_page + 1, fz_count_pages(ctx, doc), ss);

	glBegin(GL_TRIANGLE_STRIP);
	{
		if ((this_search.not_found == 1) && (this_search.hit_count_a == 0)) {
			glColor4f(1.0f, 0.2f, 0.2f, 1.0f);
		} else if (this_search.hit_count_a) { //&&(last_search_string[0]))
			glColor4f(0.7f, 1.0f, 0.7f, 1.0f);
		} else {
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		}
		glVertex2f(x, y);
		glVertex2f(x, y + h);
		glVertex2f(x + w, y);
		glVertex2f(x + w, y + h);
	}
	glEnd();

	glLineWidth(1.0);
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glBegin(GL_LINES);
	{
		glVertex2f(x, y);
		glVertex2f(x + w, y);
	}
	glEnd();

	{
		float gs = ui_get_font_size(ctx);

		ui_set_font_size(ctx, gs * 0.75);
		glColor4f(0, 0, 0, 1);
		ui_draw_string(ctx, 1, canvas_h - 5, s);
		ui_set_font_size(ctx, gs);
	}

	return 0;
}
char *stolower( char *convertme )
{

	/*
		char *c = convertme;

		while ( c && (*c != '\0')) {
		char t = tolower(*c); 
	 *c = t;  
	 c++;
	 }
	 */

	return convertme;
}

static int do_help_line(int x, int y, char *label, char *text) {
	stolower(label);
	ui_draw_string(ctx, x, y, label);
	ui_draw_string(ctx, x + 200, y, text);
	return y + (ui.lineheight *1.1);
}

static void do_help(void) {
	float x = canvas_x + 4 * ui.lineheight;
	float y = canvas_y + 4 * ui.lineheight;
	float w = canvas_w - 8 * ui.lineheight;
	float h = 38 * ui.lineheight;
	char ks[50];

	glBegin(GL_TRIANGLE_STRIP);
	{
		glColor4f(0.9f, 0.9f, 0.9f, 1.0f);
		glVertex2f(x, y);
		glVertex2f(x, y + h);
		glVertex2f(x + w, y);
		glVertex2f(x + w, y + h);
	}
	glEnd();

	x += ui.lineheight;
	y += ui.lineheight + ui.baseline;

	glColor4f(0, 0, 0, 1);
	y = do_help_line(x, y, "FlexBV PDF Viewer", "");

	y = do_help_line(x, y, "App path", exepath);
	y = do_help_line(x, y, "Log file", flog_filename);
	snprintf(ks, sizeof(ks), "%d x %d", drawable_x, drawable_y);
	y = do_help_line(x, y, "Window size", ks);

	y += ui.lineheight;
	y = do_help_line(x, y, "F1", "show this message");
	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_QUIT]);
	y = do_help_line(x, y, ks, "Quit");
	y += ui.lineheight;

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_FITWINDOW]);
	y = do_help_line(x, y, ks, "Fit to window");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_FITHEIGHT]);
	y = do_help_line(x, y, ks, "Fit to height");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_FITWIDTH]);
	y = do_help_line(x, y, ks, "Fit to width");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_ZOOMIN]);
	y = do_help_line(x, y, ks, "Zoom IN");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_ZOOMOUT]);
	y = do_help_line(x, y, ks, "Zoom OUT");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_ROTATE_CW]);
	y = do_help_line(x, y, ks, "Rotate Clockwise");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_ROTATE_CCW]);
	y = do_help_line(x, y, ks, "Rotate Counter-Clockwise");

	y += ui.lineheight;

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_PGUP]);
	y = do_help_line(x, y, ks, "Previous Page (+SHIFT = 10 pages)");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_PGDN]);
	y = do_help_line(x, y, ks, "Next Page (+SHIFT = 10 pages)");

	y += ui.lineheight;

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_SEARCH]);
	y = do_help_line(x, y, ks, "Search");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_SEARCH_NEXT]);
	y = do_help_line(x, y, ks, "Next search hit (in page, or next)");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_SEARCH_NEXT_PAGE]);
	y = do_help_line(x, y, ks, "Next search hit on subsequent page");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_SEARCH_PREV]);
	y = do_help_line(x, y, ks, "Previous search hit (in page, or next)");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_SEARCH_PREV_PAGE]);
	y = do_help_line(x, y, ks, "Previous search hit on earlier page");

	y += ui.lineheight;


	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_GOPAGE]);
	y = do_help_line(x, y, ks, "Go to page 'N', ie, type 22g for page 22.");

	KEYB_combo_to_string(ks, sizeof(ks),keyboard_map[PDFK_GOENDPAGE]);
	y = do_help_line(x, y, ks, "Go to end of document");


	y += ui.lineheight;
}

static void do_canvas(void) {
	float x, y;

	if (oldpage != currently_viewed_page || oldzoom != currentzoom || oldrotate != currentrotate || oldinvert != currentinvert) {
		render_page();
		update_title();
		oldpage   = currently_viewed_page;
		oldzoom   = currentzoom;
		oldrotate = currentrotate;
		oldinvert = currentinvert;
	}

	x = canvas_x - scroll_x;
	y = canvas_y - scroll_y;


	ui_draw_image(&page_tex, x - page_tex.x, y - page_tex.y);

	if (!this_search.active) {
		do_links(links, x, y);
		do_page_selection(x, y, x + page_tex.w, y + page_tex.h);
		if (currently_viewed_page == this_search.page && this_search.hit_count_a > 0) do_search_hits(x, y);
	}

}

int ddi_process_keymap( char *ddi_data, char *keystr, int index ) {
	//	flog("%s:%d: processing keymap, %s [ %d ]\n", FL, keystr, index);
	if (strstr(ddi_data, keystr)) {
		char *p = strstr(ddi_data, keystr);
		if (p) {
			p += strlen(keystr);
			sscanf(p,"%d %x", &keyboard_map[index].key, &keyboard_map[index].mods);
			//			flog("%s:%d: imported %d & %x\n", FL, keyboard_map[index].key, keyboard_map[index].mods);
		}
	}
	return 0;
}


int ddi_process(char *ddi_data) {
	char *cmd, *p, *q;
	int result = 0;

	compsearch_radius    = 500.0f;
	compsearch_highlight = 8; // 0b0111

	if (strstr(ddi_data, "!debug:")) {
		fprintf(stderr, "%s:%d: DEBUG mode ACTIVE\r\n", FL);
		fprintf(stderr, "%s:%d: DDI Data---------\r\n%s\r\n-------------\r\n", FL, ddi_data);
		debug = 1;
	}

	if (strstr(ddi_data, "!headless:")) {
		headless_data = strdup(ddi_data);
		runmode       = RUNMODE_HEADLESS;
	}

	if ((cmd = strstr(ddi_data, "!csradius:"))) {
		double t = atof(cmd + strlen("!csradius:"));
		if ((t >= 1.0) || (t <= 50000.0)) compsearch_radius = t; // default fall back
		flog("%s:%d: compsearch radius set to %f\r\n", FL, t);
	}

	if ((cmd = strstr(ddi_data, "!cshighlight:"))) {
		int t = atoi(cmd + strlen("!cshighlight:"));
		if (t > 0) compsearch_highlight = t; // default fall back
		flog("%s:%d: compsearch highlight set to %d\r\n", FL, compsearch_highlight);
	}

	if ((p = strstr(ddi_data, "!setwindowsize:"))) {
		q = strchr(p, '\n');
		if (q) *q = '\0';
		sscanf(p + strlen("!setwindowsize:"), "%d %d", &window_w, &window_h);
		flog("%s:%d: Set window size: %d %d\r\n", FL, window_w, window_h);
		if (q) *q = '\n';
	}

	if ((p = strstr(ddi_data, "!setwindowsizepos:"))) {
		q = strchr(p, '\n');
		if (q) *q = '\0';
		sscanf(p + strlen("!setwindowsizepos:"), "%d %d %d %d", &window_w, &window_h, &origin_x, &origin_y);
		flog("%s:%d: Set window size pos: %d %d @ %d %d\r\n", FL, window_w, window_h, origin_x, origin_y);
		if (q) *q = '\n';
	}

	if ((p = strstr(ddi_data, "!getwindowsizepos:"))) {
		char tmp[1024];
		int x, y, ox, oy;
		SDL_GetWindowPosition(sdlWindow, &ox, &oy);
		SDL_GetWindowSize(sdlWindow, &x, &y);
		snprintf(tmp, sizeof(tmp), "!pdfwininfo=%d %d %d %d\r\n", ox, oy, x, y);
		flog("%s:%d: Dispatching '%s'\r\n", FL, tmp);
		DDI_dispatch(&ddi, tmp);

	}


	if (strstr(ddi_data, "!search_next:")) {
		flog("%s:%d: Search NEXT hit, simulate keypress in viewer.\n",FL);
		ui_set_keypress(PDFK_SEARCH_NEXT);
		do_keypress();
		return result;
	}

	if (strstr(ddi_data, "!search_prev:")) {
		flog("%s:%d: Search PREV hit, simulate keypress in viewer.\n",FL);
		ui_set_keypress( PDFK_SEARCH_PREV );
		do_keypress();
		return result;
	}

	if (strstr(ddi_data, "!search_page_next:")) {
		ui_set_keypress(PDFK_SEARCH_NEXT_PAGE);
		do_keypress();
		return result;
	}

	if (strstr(ddi_data, "!search_page_prev:")) {
		ui_set_keypress(PDFK_SEARCH_PREV_PAGE);
		do_keypress();
		return result;
	}

	if (strstr(ddi_data, "!gotopg:")) {
		char *p = strstr(ddi_data, "!gotopg:");
		if (p) {
			clear_search();
			flog("%s:%d: decoding %s\r\n", FL, p + strlen("!gotopg:"));
			currently_viewed_page = strtol(p + strlen("!gotopg:"), NULL, 10);
			flog("%s:%d: page set to %d\r\n", FL, currently_viewed_page);
			if (currently_viewed_page > fz_count_pages(ctx, doc)) currently_viewed_page = fz_count_pages(ctx, doc);
			currently_viewed_page--;
			this_search.mode = SEARCH_MODE_NONE;
		}
	}

	if (strstr(ddi_data, "!getstats:")) {
		char tmp[1024];
		clear_search();
		snprintf(tmp, sizeof(tmp), "!pdfstats:page=%d\r\n", currently_viewed_page + 1);
		flog("%s:%d: Dispatching '%s'\r\n", FL, tmp);
		DDI_dispatch(&ddi, tmp);
	}

	if (strstr(ddi_data, "!quit:")) {
		if (time(NULL) - process_start_time > 2) quit();
	}

	if (strstr(ddi_data, "!cinvert:")) {
		currentinvert = !currentinvert;
	}

	if (strstr(ddi_data, "!ss:")) {
		scroll_wheel_swap = 1;
	}

	if (strstr(ddi_data, "!raise:")) {
		raise_on_search = 1;
	}

	if (strstr(ddi_data, "!noraise:")) {
		raise_on_search = 0;
	}

	if (strstr(ddi_data, "!detached:")) {
		detached = 1;
	}

	if (strstr(ddi_data, "!load:")) {

		char *fnp;

		/*
		 * load a file, not searching.
		 */

		clear_search();
		fnp = strstr(ddi_data, "!load:");
		flog("%s:%d: fnp = '%s'\r\n", FL, fnp);
		snprintf(filename, sizeof(filename), "%s", fnp + strlen("!load:"));
		fnp = strpbrk(filename, "\n\r");
		if (fnp) *fnp = '\0';
		flog("%s:%d: filename = '%s'\r\n", FL, filename);

		title = strrchr(filename, '/');
		if (!title) title = strrchr(filename, '\\');
		if (title)
			++title;
		else
			title = filename;

		ctx = fz_new_context(NULL, NULL, 0);
		fz_register_document_handlers(ctx);

		if (layout_css) {
			fz_buffer *buf = fz_read_file(ctx, layout_css);
			fz_set_user_css(ctx, fz_string_from_buffer(ctx, buf));
			fz_drop_buffer(ctx, buf);
		}

		fz_set_use_document_css(ctx, layout_use_doc_css);

		flog("%s:%d: about to reload (%s)\r\n", FL, filename);
		reload_required = 1;
		flog("%s:%d: reload done (%s)\r\n", FL, filename);
	}

	ddi_process_keymap(ddi_data, "!keysearch=", PDFK_SEARCH);
	ddi_process_keymap(ddi_data, "!keynext=", PDFK_SEARCH_NEXT);
	ddi_process_keymap(ddi_data, "!keyprev=", PDFK_SEARCH_PREV);

	keyboard_map[PDFK_SEARCH_PREV_PAGE].key = keyboard_map[PDFK_SEARCH_PREV].key;
	keyboard_map[PDFK_SEARCH_PREV_PAGE].mods = keyboard_map[PDFK_SEARCH_PREV].mods | KEYB_MOD_SHIFT;
	keyboard_map[PDFK_SEARCH_NEXT_PAGE].key = keyboard_map[PDFK_SEARCH_NEXT].key;
	keyboard_map[PDFK_SEARCH_NEXT_PAGE].mods = keyboard_map[PDFK_SEARCH_NEXT].mods | KEYB_MOD_SHIFT;

	ddi_process_keymap(ddi_data, "!keypgup=", PDFK_PGUP);
	ddi_process_keymap(ddi_data, "!keypgdn=", PDFK_PGDN);

	keyboard_map[PDFK_PGUP_10].key = keyboard_map[PDFK_PGUP].key;
	keyboard_map[PDFK_PGUP_10].mods = keyboard_map[PDFK_PGUP].mods | KEYB_MOD_SHIFT;
	keyboard_map[PDFK_PGDN_10].key = keyboard_map[PDFK_PGDN].key;
	keyboard_map[PDFK_PGDN_10].mods = keyboard_map[PDFK_PGDN].mods | KEYB_MOD_SHIFT;

	ddi_process_keymap(ddi_data, "!keyzoomin=", PDFK_ZOOMIN);
	ddi_process_keymap(ddi_data, "!keyzoomout=", PDFK_ZOOMOUT);
	ddi_process_keymap(ddi_data, "!keyrotatecw=", PDFK_ROTATE_CW);
	ddi_process_keymap(ddi_data, "!keyrotateccw=", PDFK_ROTATE_CCW);
	ddi_process_keymap(ddi_data, "!keyup=", PDFK_PAN_UP);
	ddi_process_keymap(ddi_data, "!keydown=", PDFK_PAN_DOWN);
	ddi_process_keymap(ddi_data, "!keyleft=", PDFK_PAN_LEFT);
	ddi_process_keymap(ddi_data, "!keyright=", PDFK_PAN_RIGHT);

	ddi_process_keymap(ddi_data, "!keyfitwindow=", PDFK_FITWINDOW);
	ddi_process_keymap(ddi_data, "!keyfitwidth=", PDFK_FITWIDTH);
	ddi_process_keymap(ddi_data, "!keyfitheight=", PDFK_FITHEIGHT);
	ddi_process_keymap(ddi_data, "!keygopage=", PDFK_GOPAGE);
	ddi_process_keymap(ddi_data, "!keygoendpage=", PDFK_GOENDPAGE);

	if (strstr(ddi_data, "!noheuristics:")) {
		flog("%s:%d: No heuristics", FL);
		search_heuristics = 0;
	}

	if (strstr(ddi_data, "!heuristics:")) {
		search_heuristics = 1;
		flog("%s:%d: Heuristics", FL);
	}

	if ((cmd = strstr(ddi_data, "!strictmatch:"))) {
		flog("%s:%d: Strict match\r\n", FL);
		ctx->flags |= FZ_CTX_FLAGS_STRICT_MATCH;
	}

	if ((cmd = strstr(ddi_data, "!stdmatch:"))) {
		flog("%s:%d: Standard match (default)\r\n", FL);
		ctx->flags &= ~FZ_CTX_FLAGS_STRICT_MATCH;
	}

	if ((cmd = strstr(ddi_data, "!pagesearch:")) != NULL) {
		this_search.active = 1;
		this_search.mode = SEARCH_MODE_INPAGE;
		this_search.direction = 1;
		this_search.has_hits = 0;
		snprintf(this_search.search_raw, sizeof(this_search.search_raw), "%s", cmd);
		snprintf(this_search.a, sizeof(this_search.a), "%s", cmd + strlen("!pagesearch:"));
	}

	if ((cmd = strstr(ddi_data, "!search:")) != NULL) {
		this_search.mode = SEARCH_MODE_NORMAL;
		if (strcmp(this_search.search_raw, cmd)==0) {
			flog("%s:%d: Same DDI search as before, simulate 'next' keypress.\n",FL);
			ui_set_keypress(PDFK_SEARCH_NEXT);
			do_keypress();
		} else {
			snprintf(this_search.search_raw, sizeof(this_search.search_raw), "%s", cmd);
			snprintf(this_search.a, sizeof(this_search.a), "%s", cmd + strlen("!search:"));
			this_search.active = 1;
			this_search.direction = 1;
			this_search.not_found = 0;
			this_search.has_hits = 0;
			this_search.page = 0;
		}
	}

	if ((cmd = strstr(ddi_data, "!compsearch:"))) {
		/*
		 * compound search requested.  First we find the page with the
		 * first part, then we find the second part.
		 *
		 */
		char tmp[1024];
		char *p;

		this_search.a[0] = this_search.b[0] = this_search.c[0] = '\0';
		this_search.mode                                       = SEARCH_MODE_COMPOUND;
		this_search.page                                       = 0;
		this_search.direction = 1;
		this_search.active = 1;

		//FIXME - should this be 1 or 0?		this_search.page                                       = 1;
		snprintf(this_search.search_raw, sizeof(this_search.search_raw), "%s", cmd);
		snprintf(tmp, sizeof(tmp), "%s", cmd + strlen("!compsearch:"));

		snprintf(this_search.a, sizeof(this_search.a), "%s", tmp);
		this_search.not_found = 0;
		this_search.has_hits = 0;
		p = strpbrk(this_search.a, "\r\n");
		if (p) *p = '\0';

		p = strchr(this_search.a, ':');
		if (p) {
			*p = '\0';
			snprintf(this_search.b, sizeof(this_search.b), "%s", p + 1);
			p = strchr(this_search.b, ':');
			if (p) {
				*p = '\0';
				snprintf(this_search.c, sizeof(this_search.c), "%s", p + 1);
			}
		}
		flog("%s:%d: elements to comp-search: %s %s %s\r\n", FL, this_search.a, this_search.b, this_search.c);
	} // compsearch

	return 0;
} // ddi_process





int sort_hits( fz_rect bb[], int count ) {
	int i, j;
	for (i = 0; i < count -1; i++) {
		double a1 = (bb[i].x1 -bb[i].x0)*(bb[i].y1 -bb[i].y0); // area for bb1
		for (j = i+1; j < count; j++) {
			double a2 = (bb[j].x1 -bb[j].x0)*(bb[j].y1 -bb[j].y0); // area for bb1
			if (a2 > a1) {
				fz_rect tmp;
				tmp = bb[i];
				bb[i] = bb[j];
				bb[j] = tmp;
			} 
		} // for j
	} // for i

	return 0;
}

#ifdef UNUSED_CODE
int do_search_2( void ) {
	if (this_search.active) {

		if (ui.key == KEY_ESCAPE) {
			this_search.active    = 0;
			memset(&this_search, 0, sizeof(this_search));
			this_search.not_found = 0;
		}

		/* ignore events during search */
		ui.key = ui.mod = ui.plain = 0;
		ui.down = ui.middle = ui.right = 0;

		if (this_search.page > fz_count_pages(ctx,doc)) {
			this_search.page = 0;
		} else if (this_search.page < 0) {
			this_search.page = fz_count_pages(ctx,doc) -1;
		}

		while (1) {
			/*
			 * Because of the prevelance of space vs underscore strings in PDF schematics
			 * we try search for both variants
			 *
			 */


			this_search.page = fz_clampi(this_search.page, 0, fz_count_pages(ctx, doc) - 1);


			if (this_search.mode != SEARCH_MODE_COMPOUND) {

				/*
				 * STANDARD SEARCH mode
				 * STANDARD SEARCH mode
				 * STANDARD SEARCH mode
				 *
				 * This is for requests from FlexBV and internal requests
				 * within the PDF Viewer itself
				 *
				 */
				this_search.hit_count_a = fz_search_page_number(
						ctx, doc, this_search.page, this_search.a, this_search.hit_bbox_a, nelem(this_search.hit_bbox_a));
				flog("%s:%d: Searching for '%s', %d hits on page %d\n",
						FL,
						this_search.a,
						this_search.hit_count_a,
						this_search.page + 1);
				if (this_search.hit_count_a) this_search.has_hits = 1;
				if ((this_search.hit_count_a == 0) && (strlen(this_search.alt))) {
					this_search.hit_count_a = fz_search_page_number(
							ctx, doc, this_search.page, this_search.alt, this_search.hit_bbox_a, nelem(this_search.hit_bbox_a));
					flog("%s:%d: Searching for '%s', %d hits on page %d\n",
							FL,
							this_search.alt,
							this_search.hit_count_a,
							this_search.page + 1);
					if (this_search.hit_count_a) this_search.has_hits = 1;
				} // if no hits on standard, try alternative


			}	else if (this_search.mode == SEARCH_MODE_COMPOUND) {
				/*
				 * COMPOUND SEARCH MODE
				 * COMPOUND SEARCH MODE
				 * COMPOUND SEARCH MODE
				 *
				 * With compound searching, we're using using the initial part just to locate our page
				 *
				 * This is usually used when we're in EDIT mode on the FlexBV side.  
				 *
				 * Basically we can ignore this under most circumstances
				 *
				 */

				this_search.page = fz_clampi(this_search.page, 0, fz_count_pages(ctx, doc) - 1);
				this_search.hit_count_a = fz_search_page_number(
						ctx, doc, this_search.page, this_search.a, this_search.hit_bbox_a, nelem(this_search.hit_bbox_a));
				flog("%s:%d: '%s' matched %d time(s)\r\n", FL, this_search.a, this_search.hit_count_a);

				if (this_search.hit_count_a) {
					int new_hit_count = 0;
					int i;

					this_search.has_hits = 1;

					if (strlen(this_search.b)) {
						this_search.hit_count_b = fz_search_page_number(
								ctx, doc, this_search.page, this_search.b, this_search.hit_bbox_b, nelem(this_search.hit_bbox_b));
						flog("%s:%d: '%s' matched %d time(s)\r\n", FL, this_search.b, this_search.hit_count_b);
					}

					if (strlen(this_search.c)) {
						this_search.hit_count_c = fz_search_page_number(
								ctx, doc, this_search.page, this_search.c, this_search.hit_bbox_c, nelem(this_search.hit_bbox_c));
						flog("%s:%d: '%s' matched %d time(s)\r\n", FL, this_search.c, this_search.hit_count_c);
					}

					/*
					 * Now... let's try merge these boxes :-o
					 *
					 */
					for (i = 0; i < this_search.hit_count_a; i++) {
						int j;
						double dist_b, dist_c;
						int closest_b = -1;
						int closest_c = -1;

						if (this_search.hit_count_b) {
							dist_b = 100000000.0f;
							for (j = 0; j < this_search.hit_count_b; j++) {
								double dx = this_search.hit_bbox_a[i].x0 - this_search.hit_bbox_b[j].x0;
								double dy = this_search.hit_bbox_a[i].y0 - this_search.hit_bbox_b[j].y0;
								double td = dx * dx + dy * dy;
								if (td < dist_b) {
									dist_b    = td;
									closest_b = j;
								}
							}
						} else
							dist_b = 0;

						if (this_search.hit_count_c) {
							dist_c = 100000000.0f;
							for (j = 0; j < this_search.hit_count_c; j++) {
								double dx = this_search.hit_bbox_a[i].x0 - this_search.hit_bbox_c[j].x0;
								double dy = this_search.hit_bbox_a[i].y0 - this_search.hit_bbox_c[j].y0;
								double td = dx * dx + dy * dy;
								if (td < dist_c) {
									dist_c    = td;
									closest_c = j;
								}
							}
						} else
							dist_c = 0;

						if (strlen(this_search.b) && (this_search.hit_count_b == 0)) this_search.hit_count_a = 0;
						if (strlen(this_search.c) && (this_search.hit_count_c == 0)) this_search.hit_count_a = 0;

						if (this_search.hit_count_a) {
							if ((dist_b < compsearch_radius) && (dist_c < compsearch_radius)) {
								static fz_rect a, b, c;

								if (compsearch_highlight == 8) {
									a = this_search.hit_bbox_a[i];

									if (this_search.hit_count_b) {
										b = this_search.hit_bbox_b[closest_b];
										if (b.x1 > a.x1) a.x1 = b.x1;
										if (b.y1 > a.y1) a.y1 = b.y1;
										if (b.x0 < a.x0) a.x0 = b.x0;
										if (b.y0 < a.y0) a.y0 = b.y0;
									}

									if (this_search.hit_count_c) {
										c = this_search.hit_bbox_c[closest_c];
										if (c.x1 > a.x1) a.x1 = c.x1;
										if (c.y1 > a.y1) a.y1 = c.y1;
										if (c.x0 < a.x0) a.x0 = c.x0;
										if (c.y0 < a.y0) a.y0 = c.y0;
									}
								} else if (compsearch_highlight == 4) {
									a = this_search.hit_bbox_a[i];
								} else if (compsearch_highlight == 2) {
									if (this_search.hit_count_b) a = this_search.hit_bbox_b[closest_b];
								} else if (compsearch_highlight == 1) {
									if (this_search.hit_count_c) a = this_search.hit_bbox_c[closest_c];
								}

								this_search.hit_bbox_a[new_hit_count] = a;
								new_hit_count++;
							} // if distances are within 500
						} // if there was a hit on the first search parameter
					} // for each main search hit
					this_search.hit_count_a = new_hit_count;
				} // if search hit count
			} // if search compound


			/*
			 * Evalute our search results
			 *
			 */

			if (this_search.hit_count_a > 0) {

				/*
				 * If we did get a search hit, then we can proceed
				 * with marking it out on the canvas view
				 *
				 */
				fz_point p;
				fz_rect *bb;
				this_search.active = 0;

				if ((this_search.direction == 1) && (this_search.inpage_index == -1)) {
					this_search.inpage_index = 0;
				}

				if ((this_search.direction == -1) && (this_search.inpage_index == -1)) {
					this_search.inpage_index = this_search.hit_count_a - 1;
				}

				p.x = (canvas_w / 2) * 72 / (currentzoom);
				p.y = (canvas_h / 2) * 72 / (currentzoom);

				sort_hits(this_search.hit_bbox_a, this_search.hit_count_a);
				bb  = &this_search.hit_bbox_a[this_search.inpage_index];

				flog("%s:%d: Jumping to %d[%f %f]\r\n", FL, this_search.page, bb->x0, bb->y0);
				jump_to_page_xy(this_search.page, bb->x0 - p.x, bb->y0 - p.y);

				break;

			} else {

				/*
				 * NO SEARCH HITS
				 * NO SEARCH HITS
				 * NO SEARCH HITS
				 *
				 */
				flog("%s:%d: No hits on page %d, trying next...\r\n", FL, this_search.page);

				this_search.page += this_search.direction; // try the next page.

				if (this_search.direction > 0) {
					if (this_search.page >= fz_count_pages(ctx, doc) - 1) {
						flog("%s:%d: end of the road for '%s'\r\n", FL, this_search.a);
						this_search.page = fz_count_pages(ctx, doc) - 1;
						memcpy(&prior_search, &this_search, sizeof(this_search));
						this_search.not_found = 1;
						update_title();
						this_search.active = 0;
						break;
					} // if we ran out of pages in the positive direction

				} else if (this_search.direction < 0) {
					if (this_search.page < 0) {
						flog("%s:%d: end of the road for '%s'\r\n", FL, this_search.a);
						this_search.page = 0;
						memcpy(&prior_search, &this_search, sizeof(this_search));
						this_search.not_found = 1;
						update_title();
						this_search.active = 0;
						break;
					} // if we ran out of pages in the negative direction

				} // if search-direction

			} // while loop

			/* keep searching later */
			if (this_search.active) {
			}
		} // if search-active

	} // while (1) (break out when hits found or end of document

}
#endif //UNUSED_CODE




int do_search_compound( void ) {
	this_search.page = fz_clampi(this_search.page, 0, fz_count_pages(ctx, doc) - 1);
	this_search.hit_count_a = fz_search_page_number(
			ctx, doc, this_search.page, this_search.a, this_search.hit_bbox_a, nelem(this_search.hit_bbox_a));
	flog("%s:%d: '%s' matched %d time(s)\r\n", FL, this_search.a, this_search.hit_count_a);

	if (this_search.hit_count_a) {
		int new_hit_count = 0;
		int i;

		this_search.has_hits = 1;
		if (strlen(this_search.b)) {
			this_search.hit_count_b = fz_search_page_number(ctx,
					doc,
					this_search.page,
					this_search.b,
					this_search.hit_bbox_b,
					nelem(this_search.hit_bbox_b));
			flog("%s:%d: '%s' matched %d time(s)\r\n", FL, this_search.b, this_search.hit_count_b);
		}

		if (strlen(this_search.c)) {
			this_search.hit_count_c = fz_search_page_number(ctx,
					doc,
					this_search.page,
					this_search.c,
					this_search.hit_bbox_c,
					nelem(this_search.hit_bbox_c));
			flog("%s:%d: '%s' matched %d time(s)\r\n", FL, this_search.c, this_search.hit_count_c);
		}

		/*
		 * Now... let's try merge these boxes :-o
		 *
		 */
		for (i = 0; i < this_search.hit_count_a; i++) {
			int j;
			double dist_b, dist_c;
			int closest_b = -1;
			int closest_c = -1;

			if (this_search.hit_count_b) {
				dist_b = 100000000.0f;
				for (j = 0; j < this_search.hit_count_b; j++) {
					double dx = this_search.hit_bbox_a[i].x0 - this_search.hit_bbox_b[j].x0;
					double dy = this_search.hit_bbox_a[i].y0 - this_search.hit_bbox_b[j].y0;
					double td = dx * dx + dy * dy;
					if (td < dist_b) {
						dist_b    = td;
						closest_b = j;
					}
				}
			} else
				dist_b = 0;

			if (this_search.hit_count_c) {
				dist_c = 100000000.0f;
				for (j = 0; j < this_search.hit_count_c; j++) {
					double dx = this_search.hit_bbox_a[i].x0 - this_search.hit_bbox_c[j].x0;
					double dy = this_search.hit_bbox_a[i].y0 - this_search.hit_bbox_c[j].y0;
					double td = dx * dx + dy * dy;
					if (td < dist_c) {
						dist_c    = td;
						closest_c = j;
					}
				}
			} else
				dist_c = 0;

			if (strlen(this_search.b) && (this_search.hit_count_b == 0)) this_search.hit_count_a = 0;
			if (strlen(this_search.c) && (this_search.hit_count_c == 0)) this_search.hit_count_a = 0;

			if ((this_search.hit_count_a) && (dist_b < compsearch_radius) && (dist_c < compsearch_radius)) {
				static fz_rect a, b, c;
				a = this_search.hit_bbox_a[i];

				flog("%s:%d: limit = %f highlight mode = %d  distb = %f distc = %f",
						FL,
						compsearch_radius,
						compsearch_highlight,
						dist_b,
						dist_c);

				if (compsearch_highlight == 8) {
					a = this_search.hit_bbox_a[i];

					if (this_search.hit_count_b) {
						b = this_search.hit_bbox_b[closest_b];
						if (b.x1 > a.x1) a.x1 = b.x1;
						if (b.y1 > a.y1) a.y1 = b.y1;
						if (b.x0 < a.x0) a.x0 = b.x0;
						if (b.y0 < a.y0) a.y0 = b.y0;
					}

					if (this_search.hit_count_c) {
						c = this_search.hit_bbox_c[closest_c];
						if (c.x1 > a.x1) a.x1 = c.x1;
						if (c.y1 > a.y1) a.y1 = c.y1;
						if (c.x0 < a.x0) a.x0 = c.x0;
						if (c.y0 < a.y0) a.y0 = c.y0;
					}
				} else if (compsearch_highlight == 4) {
					a = this_search.hit_bbox_a[i];
				} else if (compsearch_highlight == 2) {
					if (this_search.hit_count_b) a = this_search.hit_bbox_b[closest_b];
				} else if (compsearch_highlight == 1) {
					if (this_search.hit_count_c) a = this_search.hit_bbox_c[closest_c];
				}

				this_search.hit_bbox_a[new_hit_count] = a;
				new_hit_count++;
			} // if distances are within 500

		} // for each main search hit
		this_search.hit_count_a = new_hit_count;
	} // if search hit count

	return 0;
}

/*
 *
 * DO_SEARCH
 * DO_SEARCH
 * DO_SEARCH
 *
 *    This one has IN_PAGE search as well... might have to use this one?
 *
 */
int do_search( void ) {

	if (strlen(this_search.a) == 0) return 0;
	if (!this_search.active) return 0;
	if (this_search.mode == SEARCH_MODE_NONE) return 0;
	if (this_search.a[0] == '\0') return 0;

	/*
	 * searching
	 *
	 * Sending the same search string as before
	 * will result in us moving to the next index.
	 *
	 * sn contains the search string.
	 *
	 */

	flog("%s:%d: SEARCHING mode=%d '%s'\r\n", FL, this_search.mode, this_search.a);

	this_search.alt[0] = '\0';
	if (search_heuristics == 1) {
		if (strchr(this_search.a, '_')) {
			int i, l;
			snprintf(this_search.alt, sizeof(this_search.alt), "%s", this_search.a);
			l = strlen(this_search.alt);
			for (i = 0; i < l - 1; i++) {
				if (this_search.alt[i] == '_') this_search.alt[i] = ' ';
			}
			if (debug) fprintf(stderr, "%s:%d: Alternative search: '%s'\r\n", FL, this_search.alt);
		}
	} else {
		flog("%s:%d: Search heuristics disabled\r\n", FL);
	}

	/*
	 * Step 1: check our input
	 *
	 */
	if (this_search.mode == SEARCH_MODE_INPAGE) {
		this_search.inpage_index = 0;
	}

	ui_begin();


	while (this_search.active == 1) {

		/*
		 * Keep going around until our search stops being
		 * active
		 *
		 */
		flog("\n\n%s:%d: Current search page = %d ( %d page(s) in document )\n", FL, this_search.page +1, fz_count_pages(ctx,doc));

		if (this_search.page > fz_count_pages(ctx, doc) - 1) {
			flog("%s:%d: End of pages hit, resetting search data and stopping search\n", FL);
			memset(&prior_search, 0, sizeof(prior_search));
			this_search.page         = -1;
			this_search.inpage_index = -1;
			this_search.active            = 0;
			break;
		}

		/*
		 * Because of the prevelance of space vs underscore strings in PDF schematics
		 * we try search for both variants
		 *
		 */
		if (this_search.mode != SEARCH_MODE_COMPOUND) {
			this_search.page = fz_clampi(this_search.page, 0, fz_count_pages(ctx, doc) - 1);
			this_search.hit_count_a = fz_search_page_number(
					ctx, doc, this_search.page, this_search.a, this_search.hit_bbox_a, nelem(this_search.hit_bbox_a));
			flog("%s:%d: Searching for '%s', %d hits on page %d\n",
					FL,
					this_search.a,
					this_search.hit_count_a,
					this_search.page + 1);

			if ((this_search.hit_count_a == 0) && (strlen(this_search.alt))) {
				this_search.hit_count_a = fz_search_page_number(
						ctx, doc, this_search.page, this_search.alt, this_search.hit_bbox_a, nelem(this_search.hit_bbox_a));
				flog("%s:%d: Searching for alternative - '%s', %d hits on page %d\n",
						FL,
						this_search.alt,
						this_search.hit_count_a,
						this_search.page + 1);
			}
		}

		if (this_search.hit_count_a) this_search.has_hits = 1;

		/*
		 * With compound searching, we're using using the initial part just to locate our page
		 *
		 */
		if (this_search.mode == SEARCH_MODE_COMPOUND) {

			do_search_compound();

		}     // if search compound

		/*
		 * If we've used up all our hits in this page
		 *
		 * OR if there were no hits on this page
		 *
		 */
		if (this_search.mode != SEARCH_MODE_INPAGE) {
			flog("%s:%d: Normal page search: %d hits, inpage_index=%d, page=%d, direction=%d\r\n", FL, this_search.hit_count_a, this_search.inpage_index, this_search.page, this_search.direction);

			if (this_search.direction == 1) {

				//if ((this_search.hit_count_a == 0) || (this_search.inpage_index > this_search.hit_count_a - 2)) 
				if (this_search.hit_count_a == 0)  {
					this_search.inpage_index = -1;
					flog("%s:%d: Search direction = %d\n", FL, this_search.direction);
					this_search.page += this_search.direction; //20190415

					/*
					 * If we've used up all the pages in the document
					 *
					 */
					if (this_search.page >= fz_count_pages(ctx, doc) - 1) {

						/*
						 * If the document does have hits
						 *
						 */
						if (this_search.has_hits) {
							flog("%s:%d: End of document reached, but resetting back to start\r\n", FL);
							this_search.page  = 0;
							this_search.active     = 0;
							//									continue;
						} else {
							flog("%s:%d: End of document reached, no hits found at all\r\n", FL);
							memcpy(&prior_search, &this_search, sizeof(prior_search));

							this_search.not_found = 1;
							update_title();
							this_search.active = 0;
							break; // no hits in document
						} // if document has hits
					} // if search page is out of bounds

				} else {
					if (this_search.inpage_index == -1) this_search.inpage_index = 0;

				} // if search hit count is zero for this page


			} else if (this_search.direction == -1) {

				if (this_search.hit_count_a == 0)  {
					this_search.inpage_index = -1;
					this_search.page += this_search.direction; //20190415
					flog("%s:%d: Search direction = %d, page = %d\n", FL, this_search.direction, this_search.page);

					/*
					 * If we've used up all the pages in the document
					 *
					 */
					if (this_search.page <= 0) {

						/*
						 * If the document does have hits
						 *
						 */
						if (this_search.has_hits) {
							flog("%s:%d: End of document reached, but resetting back to start\r\n", FL);
							this_search.page  = fz_count_pages(ctx,doc) -1;
							this_search.active     = 0;

						} else {
							flog("%s:%d: End of document reached, no hits found at all\r\n", FL);
							memcpy(&prior_search, &this_search, sizeof(prior_search));

							this_search.not_found = 1;
							update_title();
							this_search.active = 0;
							break; // no hits in document
						} // if document has hits
					} // if search page is out of bounds

				} else {
					if (this_search.inpage_index == -1) this_search.inpage_index = this_search.hit_count_a -1;

				}// if search hit count is zero for this page

			} else { // if search direction is positive
				flog("%s:%d: Direction is zero ---- what's going on?", FL);
				this_search.active = 0;
			}

		} // checking results if not an INPAGE search

		/*
		 * If we have search hits...
		 *
		 */
		if ((this_search.hit_count_a) && (this_search.mode != SEARCH_MODE_INPAGE)) {
			fz_point p;
			fz_rect *bb;
			this_search.has_hits = 1;

			/*
			 * If the search_current_page hasn't yet been initialised
			 * then set it to this page that we've got hits on.
			 *
			 */
			if (this_search.inpage_index < 0) this_search.inpage_index = 0;

			this_search.active = 0;

			p.x = (canvas_w / 2) * 72 / (currentzoom);
			p.y = (canvas_h / 2) * 72 / (currentzoom);

			sort_hits(this_search.hit_bbox_a, this_search.hit_count_a);
			bb  = &this_search.hit_bbox_a[this_search.inpage_index];
			flog("%s:%d: Jumping to %d[%d][%f %f]\r\n", FL, this_search.page+1, this_search.inpage_index, bb->x0, bb->y0);
			jump_to_page_xy(this_search.page, bb->x0 - p.x, bb->y0 - p.y);
			currently_viewed_page = this_search.page;
			this_search.active = 0;
		} // if search hit count > 0

		if (this_search.mode == SEARCH_MODE_INPAGE) {
			this_search.active = 0;
		} // search in page only

	} // while search is active

	return 0;
}




int ddi_get(char *buf, size_t size) {
	int result = 0;

	if (!ddiprefix) return 0;

	if (DDI_pickup(&ddi, buf, size) == 0) {
		flog("%s:%d: Received '%s'\r\n", FL, buf);
		result = 1;
	} // if file opened

	return result;
}




static void run_processing_loop(void) {

	ui_begin();

	do_search();

	canvas_w = window_w - canvas_x;
	canvas_h = window_h - canvas_y;

	do_canvas();

	if (showinfo)
		do_info();
	else if (showhelp)
		do_help();
	if (showoutline) do_outline(outline, canvas_x);

	if (showsearch_input) {
		/*
		 * This is handling the user input, not the search results
		 *
		 */
		int state =
			ui_input(canvas_x + ui.lineheight, ui.lineheight, canvas_x + canvas_w - 10, ui.lineheight * 2 + 2, &search_input);
		if (state == -1) {
			// User pressed ESCAPE
			ui.focus   = NULL;
			showsearch_input = 0;

		} else if (state == 1) {
			// User pressed RETURN
			flog("%s:%d: User search input completed, ENTER pressed. search='%s'\n",FL, search_input.text);
			ui.focus         = NULL;
			showsearch_input       = 0;
			this_search.page = 0; //FIXME 20190415

			if (search_input.end > search_input.text) {
				snprintf(this_search.a, sizeof(this_search.a), "%s", search_input.text);
				this_search.mode = SEARCH_MODE_NORMAL;
				this_search.not_found = 0;
				this_search.has_hits = 0;
				this_search.active = 1;
				this_search.direction = 1;
				//this_search.page = currently_viewed_page;
				this_search.page = 0;
				this_search.inpage_index = -1;
			}
		}
	}

	if (this_search.active) {
		char buf[256];
		int x = canvas_x; // + 1 * ui.lineheight;
		int y = canvas_y; // + 1 * ui.lineheight;
		int w = canvas_w;
		int h = 1.25 * ui.lineheight;

		glBegin(GL_TRIANGLE_STRIP);
		{
			glColor4f(0.9f, 0.9f, 0.1f, 1.0f);
			glVertex2f(x, y);
			glVertex2f(x, y + h);
			glVertex2f(x + w, y);
			glVertex2f(x + w, y + h);
		}
		glEnd();

		sprintf(buf, "%d of %d.", this_search.page + 1, fz_count_pages(ctx, doc));
		glColor4f(0, 0, 0, 1);
		do_info_line(x, y + (1.1 * ui.lineheight), "Searching: ", buf);
	}

	do_status_footer();

	ui_end();

	ogl_assert(ctx, "swap buffers");
}

static void on_wheel(int direction, int x, int y) {

	/*
	 * NOTE: We don't set the glutOnMouse() callback to be this
	 * function because it's not dependable in X.  Rather instead
	 * we use on_mouse() to determine all the button states and
	 * call on_wheel() in a more predictable manner.
	 *
	 */

	if (!(SDL_GetModState() & KMOD_CTRL) != (!scroll_wheel_swap)) {

		/*
		 * ZOOM modfication
		 *
		 */
		double oz;
		double tx, ty, desx, desy;
		double pct;
		double tsx, tsy;

		oz = currentzoom;

		tsx = scroll_x;
		tsy = scroll_y;

		pct = 1.2;

		tx = (tsx + x);
		ty = (tsy + y);

		if (direction > 0) {
			currentzoom *= pct;
			if (currentzoom > MAXRES) {
				currentzoom = MAXRES;
//				currentzoom = oz;
				return;
			}
			desx = tx * pct;
			desy = ty * pct;
			tsx += desx - tx;
			tsy += desy - ty;

		} else {
			currentzoom /= pct;
			if (currentzoom < MINRES) {
				currentzoom = MINRES;
//				currentzoom = oz;
				return;
			}
			desx = tx / pct;
			desy = ty / pct;
			tsx += desx - tx;
			tsy += desy - ty;
		}

		scroll_x = floor(tsx);
		scroll_y = floor(tsy);

	} else {

		/*
		 * PAGE jump
		 *
		 */
		int jump = 1;
		if (direction < 0)
			currently_viewed_page += jump;
		else
			currently_viewed_page -= jump;
		if (currently_viewed_page < 0) currently_viewed_page = 0;
		if (currently_viewed_page >= fz_count_pages(ctx, doc)) {
			currently_viewed_page = fz_count_pages(ctx, doc) - 1;
		}
	} // page jump & zoom modification
} // on_wheel()

static void on_mouse(int button, int action, int x, int y, int clicks) {
	ui.x = x;
	ui.y = y;

	//	flog("%s:%d: button: %d %d\r\n", FL, button, action);
	switch (button) {
		case SDL_BUTTON_LEFT: ui.down = (action == SDL_MOUSEBUTTONDOWN); break;
		case SDL_BUTTON_MIDDLE: ui.middle = (action == SDL_MOUSEBUTTONDOWN); break;
		case SDL_BUTTON_RIGHT: ui.right = (action == SDL_MOUSEBUTTONDOWN); break;
	}

} // on_mouse()

static void on_motion(int x, int y) {
	ui.x = x;
	ui.y = y;

	if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		if (!am_dragging) {
			am_dragging      = 1;
			dragging_start.x = x;
			dragging_start.y = y;
		} else {
			scroll_x += dragging_start.x - x;
			scroll_y += dragging_start.y - y;
			dragging_start.x = x;
			dragging_start.y = y;
		}
	} else {
		am_dragging = 0;
	}
}

/*
 * Headless DDI check, with single shot run and results
 *
 */
static int ddi_check_headless(char *sn_a) {
	int new_hit_count = 0;
	int pages;

	pages = fz_count_pages(ctx, doc);
	if (this_search.mode != SEARCH_MODE_COMPOUND) return 0;

	this_search.page      = 0; //FIXME 20190415
	this_search.direction = 1;

	while (this_search.page < pages) {

		/*
		 * Because of the prevelance of space vs underscore strings in PDF schematics
		 * we try search for both variants
		 *
		 */
		this_search.page = fz_clampi(this_search.page, 0, fz_count_pages(ctx, doc) - 1);
		this_search.hit_count_a =
			fz_search_page_number(ctx, doc, this_search.page, this_search.a, this_search.hit_bbox_a, nelem(this_search.hit_bbox_a));

		/*
		 * With compound searching, we're using using the initial part just to locate our page
		 *
		 */
		if (this_search.hit_count_a > 0) {

			flog("%s:%d: Searching for '%s', %d hits on page %d\n",
					FL,
					this_search.a,
					this_search.hit_count_a,
					this_search.page + 1);

			flog("%s:%d: page:%d, compound searching, now check for '%s' and '%s'\r\n",
					FL,
					this_search.page + 1,
					this_search.b,
					this_search.c);

			/*
			 * short circuit the search process here, if we've been sent only a single parameter
			 * then it sometimes is a specific part code, so we can just jump out right here.
			 *
			 */
			if (strlen(this_search.b) == 0) {
				return this_search.hit_count_a;
			}

			this_search.page = fz_clampi(this_search.page, 0, fz_count_pages(ctx, doc) - 1);
			this_search.hit_count_b = fz_search_page_number(
					ctx, doc, this_search.page, this_search.b, this_search.hit_bbox_b, nelem(this_search.hit_bbox_b));
			if (this_search.hit_count_b == 0) return 0;

			if (this_search.hit_count_b > 0) {

				if (strlen(this_search.c) > 0) {
					this_search.hit_count_c = fz_search_page_number(
							ctx, doc, this_search.page, this_search.c, this_search.hit_bbox_c, nelem(this_search.hit_bbox_c));
					if (this_search.hit_count_c == 0) return 0;
				} else
					this_search.hit_count_c = 0;

				{
					int i;
					/*
					 * Now... let's try merge these boxes :-o
					 *
					 */

					for (i = 0; i < this_search.hit_count_a; i++) {
						int j;
						double dist_b, dist_c;

						if (this_search.hit_count_b) {
							dist_b = 100000000.0f;
							for (j = 0; j < this_search.hit_count_b; j++) {
								double dx = this_search.hit_bbox_a[i].x0 - this_search.hit_bbox_b[j].x0;
								double dy = this_search.hit_bbox_a[i].y0 - this_search.hit_bbox_b[j].y0;
								double td = dx * dx + dy * dy;
								if (td < dist_b) {
									dist_b = td;
								}
							}
						} else
							dist_b = 0;

						if (this_search.hit_count_c) {
							dist_c = 100000000.0f;
							for (j = 0; j < this_search.hit_count_c; j++) {
								double dx = this_search.hit_bbox_a[i].x0 - this_search.hit_bbox_c[j].x0;
								double dy = this_search.hit_bbox_a[i].y0 - this_search.hit_bbox_c[j].y0;
								double td = dx * dx + dy * dy;
								if (td < dist_c) {
									dist_c = td;
								}
							}
						} else
							dist_c = 0;

						if ((dist_b < compsearch_radius) && (dist_c < compsearch_radius)) {
							flog("%s:%d: triple hit ( %f %f )\r\n", FL, dist_b, dist_c);
							new_hit_count++;
						} // test to see if we have a triple hit in the same vacinity
						else {
							//								fprintf(stderr,"%s:%d: NO hit :( ( %f %f )\r\n", FL, dist_b, dist_c );
						}
					} // for each of our original parameter hits
				}     // if this page has hits for the third parameter
			}         // if this page has hits for the second parameter
		}             // if this page has hits for first parameter
		if (new_hit_count) break;

		this_search.page++;
	} // while search is active

	flog("%s:%d: %d matches for %s, %s, %s in %s\r\n", FL, new_hit_count, this_search.a, this_search.b, this_search.c, filename);
	return new_hit_count;
}

/*
 * Standard DDI check with normal GUI searching processing
 *
 */
static void ddi_check(void) {
	char ddi_data[10240];

	/*
	 *
	 * NOTE: We're only expecting single-line DDI requests here.
	 *
	 * The only compound DDI request we get with muPDF is right at
	 * the start when we load up.
	 *
	 */

	if ((ddi_simulate_option) || (ddi_get(ddi_data, sizeof(ddi_data)))) {

		if (ddi_simulate_option == DDI_SIMULATE_OPTION_PREPROCESSED_SEARCH) {
			ddi_simulate_option = DDI_SIMULATE_OPTION_NONE;

		} else {
			if (strlen(ddi_data) < 2) return;
			flog("%s:%d: DDI DATA: '%s'\r\n", FL, ddi_data);
			ddi_process(ddi_data);
			flog("%s:%d: After DDI Processing Searching: '%s'\r\n", FL, this_search.a);
		}
	}
}

void ui_set_clipboard(const char *buf) {
	SDL_SetClipboardText(buf);
}

const char *ui_get_clipboard(void) {
	return SDL_GetClipboardText();
}

static void usage(const char *argv0) {
	fprintf(stderr, "fbvpdf version %s\n", FZ_VERSION);
	fprintf(stderr, "usage: %s [options] document [page]\n", argv0);
	fprintf(stderr, "\t-p -\tpassword\n");
	fprintf(stderr, "\t-r -\tresolution\n");
	fprintf(stderr, "\t-I\tinvert colors\n");
	fprintf(stderr, "\t-D\t<ddi prefix>\n");
	fprintf(stderr, "\t-W -\tpage width for EPUB layout\n");
	fprintf(stderr, "\t-H -\tpage height for EPUB layout\n");
	fprintf(stderr, "\t-S -\tfont size for EPUB layout\n");
	fprintf(stderr, "\t-U -\tuser style sheet for EPUB layout\n");
	fprintf(stderr, "\t-X\tdisable document styles for EPUB layout\n");
	exit(1);
}

int initGL(void) {
	int success  = 1;
	GLenum error = GL_NO_ERROR;

	// Initialize Projection Matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Check for error
	error = glGetError();
	if (error != GL_NO_ERROR) {
		success = 1;
	}

	// Initialize Modelview Matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Check for error
	error = glGetError();
	if (error != GL_NO_ERROR) {
		success = 1;
	}

	// Initialize clear color
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Check for error
	error = glGetError();
	if (error != GL_NO_ERROR) {
		success = 1;
	}

	return success;
}

int init(void) {
	// Initialization flag
	int success = 1;

	// Initialize SDL
	//	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = 0;
	} else {
		// Use OpenGL 2.1
//
		// Create window
#ifdef __APPLE__
		SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED,"0");
		//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
		//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		//SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		//SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		//SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
//		SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
		//SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		sdlWindow = SDL_CreateWindow(
				"FlexBV Schematic Viewer", origin_x, origin_y, window_w, window_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI );
#else
//		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
		SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		sdlWindow = SDL_CreateWindow(
				"FlexBV Schematic Viewer", origin_x, origin_y, window_w, window_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE );
#endif



		if (sdlWindow == NULL) {
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = 0;

		} else {
			// Create context

			SDL_GLContext glcontext = SDL_GL_CreateContext(sdlWindow);
			if (glcontext == NULL) {
				printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
				success = 0;
			} else {

				// Initialize OpenGL
				if (!initGL()) {
					printf("Unable to initialize OpenGL!\n");
					success = 0;
				}
			}
		}
	}

	if (success) {
		SDL_GL_GetDrawableSize(sdlWindow, &drawable_x, &drawable_y);
		if (drawable_y/window_h > 1.5) {
			retina_factor = 2;
			window_w *= retina_factor;
			window_h *= retina_factor;
		}
	}

	return success;
}





#ifdef __WIN32
int main_utf8(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	int c;
	int check_again  = 0;
	int wait_for_ddi = 10;
	int sleepout     = 100;
	char flogpath[4096];
	char s[10240];

	//debug = 1;
	getexepath(exepath, sizeof(exepath));
	snprintf(flogpath,sizeof(flogpath),"%s/fbvpdf.log", exepath);
	flog_init(flogpath);
	flog("Start.\r\n");


	window_w = 1280;
	window_h = 720;
	origin_x = SDL_WINDOWPOS_CENTERED;
	origin_y = SDL_WINDOWPOS_CENTERED;

	KEYB_init();

	keyboard_map[PDFK_HELP].key = SDL_SCANCODE_F1;

	keyboard_map[PDFK_SEARCH].key = SDL_SCANCODE_F;
	keyboard_map[PDFK_SEARCH].mods = KEYB_MOD_CTRL;

	keyboard_map[PDFK_SEARCH_NEXT].key = SDL_SCANCODE_N;
	keyboard_map[PDFK_SEARCH_PREV].key = SDL_SCANCODE_P;

	keyboard_map[PDFK_SEARCH_NEXT_PAGE].key = SDL_SCANCODE_N;
	keyboard_map[PDFK_SEARCH_NEXT_PAGE].mods = KEYB_MOD_SHIFT;

	keyboard_map[PDFK_SEARCH_PREV_PAGE].key = SDL_SCANCODE_P;
	keyboard_map[PDFK_SEARCH_PREV_PAGE].mods = KEYB_MOD_SHIFT;

	keyboard_map[PDFK_PAN_UP].key = SDL_SCANCODE_UP;
	keyboard_map[PDFK_PAN_DOWN].key = SDL_SCANCODE_DOWN;
	keyboard_map[PDFK_PAN_LEFT].key = SDL_SCANCODE_LEFT;
	keyboard_map[PDFK_PAN_RIGHT].key = SDL_SCANCODE_RIGHT;

	keyboard_map[PDFK_PGUP].key = SDL_SCANCODE_PAGEUP;
	keyboard_map[PDFK_PGDN].key = SDL_SCANCODE_PAGEDOWN;

	keyboard_map[PDFK_PGUP_10].key = SDL_SCANCODE_PAGEUP;
	keyboard_map[PDFK_PGUP_10].mods = KEYB_MOD_SHIFT;
	keyboard_map[PDFK_PGDN_10].key = SDL_SCANCODE_PAGEDOWN;
	keyboard_map[PDFK_PGDN_10].mods = KEYB_MOD_SHIFT;

	keyboard_map[PDFK_ZOOMIN].key = SDL_SCANCODE_EQUALS;
	keyboard_map[PDFK_ZOOMOUT].key = SDL_SCANCODE_MINUS;

	keyboard_map[PDFK_ROTATE_CW].key = SDL_SCANCODE_PERIOD;
	keyboard_map[PDFK_ROTATE_CCW].key = SDL_SCANCODE_COMMA;

	keyboard_map[PDFK_QUIT].key = SDL_SCANCODE_Q;
	keyboard_map[PDFK_QUIT].mods = KEYB_MOD_CTRL;

	keyboard_map[PDFK_GOPAGE].key = SDL_SCANCODE_G;
	keyboard_map[PDFK_GOENDPAGE].key = SDL_SCANCODE_G;
	keyboard_map[PDFK_GOENDPAGE].mods = KEYB_MOD_SHIFT;

	keyboard_map[PDFK_PASTE].key = SDL_SCANCODE_V;
	keyboard_map[PDFK_PASTE].mods = KEYB_MOD_CTRL;



	process_start_time = time(NULL); // used to discriminate if we're picking up old !quit: calls.

	flog("Parsing parameters\r\n");
	while ((c = fz_getopt(argc, argv, "p:r:i:s:IsW:H:S:U:X:D:")) != -1) {
		switch (c) {
			default: usage(argv[0]); break;
			case 'i': snprintf(filename, sizeof(filename), "%s", fz_optarg); break;
			case 'p': password = fz_optarg; break;
			case 'r': currentzoom = fz_atof(fz_optarg); break;
			case 'I': currentinvert = !currentinvert; break;
			case 'W': layout_w = fz_atof(fz_optarg); break;
			case 'H': layout_h = fz_atof(fz_optarg); break;
			case 'S': layout_em = fz_atof(fz_optarg); break;
			case 'U': layout_css = fz_optarg; break;
			case 'X': layout_use_doc_css = 0; break;
			case 'D': ddiprefix = fz_optarg; break;
			case 's': ddiloadstr = fz_optarg; break;
			case 'd': debug = 1; break;
		}
	}


	if (fz_optind < argc) anchor = argv[fz_optind++];

	/* ddi setup */
	flog("DDI setup '%s'\r\n", ddiprefix);
	DDI_init(&ddi);
	DDI_set_prefix(&ddi, ddiprefix);
	DDI_set_mode(&ddi, DDI_MODE_SLAVE);

	/*
	 * DDI setup package, is the first one we receive
	 * and may contain multiple commands for us to process.
	 *
	 */
	flog("%s:%d: DDI PICKUP\r\n", FL);
	if (ddiloadstr)
		ddi_process(ddiloadstr);
	else {
		DDI_pickup(&ddi, s, sizeof(s));
		ddi_process(s);
	}

	if (this_search.mode != SEARCH_MODE_NONE) ddi_simulate_option = DDI_SIMULATE_OPTION_PREPROCESSED_SEARCH;

	if (runmode == RUNMODE_NORMAL) {
		init();
	}

	if (reload_required) {
		reload_required = 0;
		reload();
	}

	/*
	 * If fbvpdf is being invoked in headless mode
	 * then we only need to do the search string check
	 * and report back before exiting out
	 *
	 * This does run its own unique search run/engine
	 * because it doesn't need any of the next/prev
	 * handling, it's strictly a "hit or no hit" type
	 * search
	 *
	 */
	if (runmode == RUNMODE_HEADLESS) {
		int r;
		r = ddi_check_headless(headless_data);
		snprintf(s, sizeof(s), "!headlessHits:%d", r);
		DDI_dispatch(&ddi, s);
		exit(0);
	}

	if (wait_for_ddi == 0) {

		if (fz_optind < argc) {
			fz_strlcpy(filename, argv[fz_optind++], sizeof filename);
		} else {
#ifdef _WIN32
			//			win_install();
			if (!win_open_file(filename, sizeof filename)) exit(0);
#else
			usage(argv[0]);
#endif
		}
	}

	title = strrchr(filename, '/');
	if (!title) title = strrchr(filename, '\\');
	if (title) {
		++title;
	} else {
		title = filename;
	}

	flog("Initialising FlexBV-PDF. Filename = '%s'\r\n", filename);

	ctx = fz_new_context(NULL, NULL, 0);
	if (search_heuristics) ctx->flags |= FZ_CTX_FLAGS_SPACE_HEURISTIC;

	fz_register_document_handlers(ctx);

	if (layout_css) {
		fz_buffer *buf = fz_read_file(ctx, layout_css);
		fz_set_user_css(ctx, fz_string_from_buffer(ctx, buf));
		fz_drop_buffer(ctx, buf);
	}

	fz_set_use_document_css(ctx, layout_use_doc_css);

	flog("%s:%d: Loading document\r\n", FL);
	load_document();

	flog("%s:%d: Loading page\r\n", FL);
	load_page();

	flog("%s:%d: Setting memory and search\r\n", FL);
	memset(&ui, 0, sizeof ui);

	/*
	 * search_input contains the details about the string
	 * that is typed in during the ctrl-f / search
	 *
	 */
	search_input.p   = search_input.text;
	search_input.q   = search_input.p;
	search_input.end = search_input.p;
	this_search.has_hits = 0;
	this_search.not_found = 0;
	this_search.direction = 1;
	this_search.page = 0;
	this_search.inpage_index = -1;

	flog("%s:%d: ARB non-power-of-two test\r\n", FL);
	has_ARB_texture_non_power_of_two = 0;

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

	ui.fontsize   = DEFAULT_UI_FONTSIZE;
	ui.baseline   = DEFAULT_UI_BASELINE;
	ui.lineheight = DEFAULT_UI_LINEHEIGHT;

	flog("%s:%d: ui init fonts\r\n", FL);
	ui_init_fonts(ctx, ui.fontsize);

	//	shrinkwrap();

	/*
	 * NORMAL run mode,  this is when fbvpdf is being
	 * used as a viewer for the user, as opposed to
	 * being used as a headless search engine
	 *
	 */
	if (runmode == RUNMODE_NORMAL) {
		flog("%s:%d: render page\r\n", FL);
		render_page();

		flog("%s:%d: update title\r\n", FL);
		update_title();

		glViewport(0, 0, window_w, window_h);
		glClearColor(0.3f, 0.3f, 0.5f, 1.0f);

		flog("%s:%d: SDL loop starting\r\n\r\n", FL);
		while (!doquit) {

			//			if (!(SDL_getWindowFlags() & SDL_WINDOW_SHOWN)) continue;

			glClear(GL_COLOR_BUFFER_BIT);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, window_w, window_h, 0, 0.0f, 1.0f);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			while (SDL_PollEvent(&sdlEvent)) {

				sleepout = 30;

				switch (sdlEvent.type) {

					case SDL_WINDOWEVENT:
						switch (sdlEvent.window.event) {
							case SDL_WINDOWEVENT_RESIZED:
							case SDL_WINDOWEVENT_SIZE_CHANGED:
								//							case SDL_WINDOWEVENT_MAXIMIZED:
								//							case SDL_WINDOWEVENT_RESTORED:
								window_w = retina_factor  *sdlEvent.window.data1;
								window_h = retina_factor *sdlEvent.window.data2;
								glViewport(0, 0, window_w, window_h);
								break;
						}
						break;

					case SDL_QUIT: quit(); break;

					case SDL_TEXTINPUT:
										if (showsearch_input) {
											if ((search_input.text == search_input.end) && (sdlEvent.text.text[0] == '/')) {
												// do nothing
											} else {
												ui.key = sdlEvent.text.text[0];
											}
										}
										break;

					case SDL_TEXTEDITING: break;

					case SDL_KEYDOWN:
												 if (sdlEvent.key.keysym.sym == SDLK_RCTRL) {
													 ui.rctrl = 1;
												 } else if (sdlEvent.key.keysym.sym == SDLK_LCTRL) {
													 ui.lctrl = 1;
												 }
												 ui.key = sdlEvent.key.keysym.sym;
												 ui.scancode = sdlEvent.key.keysym.scancode;
												 ui.mod = SDL_GetModState();
												 do_keypress();
												 break;

					case SDL_KEYUP:
												 if (sdlEvent.key.keysym.sym == SDLK_RCTRL) {
													 ui.rctrl = 0;
												 } else if (sdlEvent.key.keysym.sym == SDLK_LCTRL) {
													 ui.lctrl = 0;
												 }
												 break;

					case SDL_MOUSEMOTION: {
													 int x, y;
													 SDL_GetMouseState(&x, &y);
													 on_motion(x*retina_factor, y*retina_factor);
												 } break;

					case SDL_MOUSEWHEEL: {
													int x, y;
													SDL_GetMouseState(&x, &y);
													on_wheel(sdlEvent.wheel.y, x*retina_factor, y*retina_factor);
												} break;

					case SDL_MOUSEBUTTONDOWN: {
														  int x, y;
														  SDL_GetMouseState(&x, &y);
														  on_mouse(sdlEvent.button.button, SDL_MOUSEBUTTONDOWN, x*retina_factor, y*retina_factor, sdlEvent.button.clicks);
													  } break;

					case SDL_MOUSEBUTTONUP: {
														int x, y;
														SDL_GetMouseState(&x, &y);
														on_mouse(sdlEvent.button.button, SDL_MOUSEBUTTONUP, x*retina_factor, y*retina_factor, sdlEvent.button.clicks);
													} break;

				} // switch sdl event type

			} // while SDL event

			/*
			 * so that we do not constantly thrash the filesystem
			 * we only check the ddi after multiple frames
			 *
			 */
			if (check_again) {
				check_again--;
			} else {
				ddi_check();
				check_again = 5; // 10?  Bigger means longer wait
			}

			run_processing_loop();

			ui.key = ui.mod = ui.plain = 0;
			SDL_GL_SwapWindow(sdlWindow);

			if (!(sleepout--)) {
#ifdef _WIN32
				Sleep(50);
#else
				usleep(50000);
#endif
				sleepout = 0;
				continue;

			} else {
				// We've had to put this 1ms interframe sleep in because
				// SDL2 or something is crashing out with deltaTime issues
				// (might just be ImGUI)
#ifdef _WIN32
				Sleep(5);
#else
				usleep(5000);
#endif
			}

				// puts fbvpdf to sleep if nothing is happening.
				//

		} // while !doquit

		flog("%s:%d: SDL loop ended\r\n", FL);

		SDL_DestroyTexture(sdlTexture);
		SDL_DestroyRenderer(sdlRenderer);
		SDL_GL_DeleteContext(glcontext);
		SDL_DestroyWindow(sdlWindow);

	} // if runmode == RUNMODE_NORMAL

	ui_finish_fonts(ctx);

	if (runmode == RUNMODE_NORMAL) {
		SDL_DestroyWindow(sdlWindow);
		SDL_Quit();
	}

#ifndef NDEBUG
	if (fz_atoi(getenv("FZ_DEBUG_STORE"))) fz_debug_store(ctx);
#endif

	fz_drop_stext_page(ctx, text);
	fz_drop_link(ctx, links);
	fz_drop_page(ctx, page);
	fz_drop_outline(ctx, outline);
	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);

	flog("%s:%d: Finished. Good bye.\r\n", FL);


	return 0;
}

#ifdef __WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	int argc;
	LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
	char **argv   = fz_argv_from_wargv(argc, wargv);
	int ret       = main_utf8(argc, argv);
	fz_free_argv(argc, argv);
	return ret;
}
#endif
