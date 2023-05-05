
/* this code is not standalone
 * it is included into edit.c
 * for windows.
 * It is deliberately kept separate so that
 * applications that have no need for windows
 * support can omit this
 */
static DWORD orig_consolemode = 0;

static int flush_output(struct current *current);
static void output_new_line(struct current *current);

static void refresh_start(struct current *current)
{
    (void)current;
}

static void refresh_end(struct current *current)
{
    (void)current;
}

static void refresh_start_chars(struct current *current)
{
    assert(current->output == NULL);
    /* We accumulate all output here */
    current->output = sb_alloc();
    current->ubuflen = 0;
}

static void refresh_new_line(struct current *current)
{
    DRL("<nl>");
    output_new_line(current);
}

static void refresh_end_chars(struct current *current)
{
    assert(current->output);
    flush_output(current);
    sb_free(current->output);
    current->output = NULL;
}

static int enable_raw_mode(struct current *current) {
    DWORD n;
    INPUT_RECORD irec;

    current->outh = GetStdHandle(STD_OUTPUT_HANDLE);
    current->inh = GetStdHandle(STD_INPUT_HANDLE);

    if (!PeekConsoleInput(current->inh, &irec, 1, &n)) {
        return -1;
    }
    if (get_window_size(current) != 0) {
        return -1;
    }
    if (GetConsoleMode(current->inh, &orig_consolemode)) {
        SetConsoleMode(current->inh, ENABLE_PROCESSED_INPUT);
    }
    return 0;
}

static void disable_raw_mode(struct current *current)
{
    SetConsoleMode(current->inh, orig_consolemode);
}

void ax_edit_clear_screen(void)
{
    /* XXX: This is ugly. Should just have the caller pass a handle */
    struct current current;

    current.outh = GetStdHandle(STD_OUTPUT_HANDLE);

    if (get_window_size(&current) == 0) {
        COORD topleft = { 0, 0 };
        DWORD n;

        FillConsoleOutputCharacter(current.outh, ' ',
            current.cols * current.rows, topleft, &n);
        FillConsoleOutputAttribute(current.outh,
            FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN,
            current.cols * current.rows, topleft, &n);
        SetConsoleCursorPosition(current.outh, topleft);
    }
}

static void cursor_to_left(struct current *current)
{
    COORD pos;
    DWORD n;

    pos.X = 0;
    pos.Y = (SHORT)current->y;

    FillConsoleOutputAttribute(current->outh,
        FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN, current->cols, pos, &n);
    current->x = 0;
}

static void flush_ubuf(struct current *current)
{
    COORD pos;
    DWORD nwritten;
    pos.Y = (SHORT)current->y;
    pos.X = (SHORT)current->x;
    SetConsoleCursorPosition(current->outh, pos);
    WriteConsoleW(current->outh, current->ubuf, current->ubuflen, &nwritten, 0);
    current->x += current->ubufcols;
    current->ubuflen = 0;
    current->ubufcols = 0;
}

static void add_ubuf(struct current *current, int ch)
{
    /* This code originally by: Author: Mark E. Davis, 1994. */
    static const int halfShift  = 10; /* used for shifting by 10 bits */

    static const DWORD halfBase = 0x0010000UL;
    static const DWORD halfMask = 0x3FFUL;

    #define UNI_SUR_HIGH_START  0xD800
    #define UNI_SUR_HIGH_END    0xDBFF
    #define UNI_SUR_LOW_START   0xDC00
    #define UNI_SUR_LOW_END     0xDFFF

    #define UNI_MAX_BMP 0x0000FFFF

    if (ch > UNI_MAX_BMP) {
        /* convert from unicode to utf16 surrogate pairs
         * There is always space for one extra word in ubuf
         */
        ch -= halfBase;
        current->ubuf[current->ubuflen++] = (WORD)((ch >> halfShift) + UNI_SUR_HIGH_START);
        current->ubuf[current->ubuflen++] = (WORD)((ch & halfMask) + UNI_SUR_LOW_START);
    }
    else {
        current->ubuf[current->ubuflen++] = ch;
    }
    current->ubufcols += ax_utf8_width(ch);
    if (current->ubuflen >= UBUF_MAX_CHARS) {
        flush_ubuf(current);
    }
}

static int flush_output(struct current *current)
{
    const char *pt = sb_str(current->output);
    int len = sb_len(current->output);

    /* convert utf8 in current->output into utf16 in current->ubuf
     */
    while (len) {
        int ch;
        int n = ax_utf8_tounicode(pt, &ch);

        pt += n;
        len -= n;

        add_ubuf(current, ch);
    }
    flush_ubuf(current);

    sb_clear(current->output);

    return 0;
}

static int output_chars(struct current *current, const char *buf, int len)
{
    if (len < 0) {
        len = strlen(buf);
    }
    assert(current->output);

    sb_append_len(current->output, buf, len);

    return 0;
}

static void output_new_line(struct current *current)
{
    /* On the last row output a newline to force a scroll */
    if (current->y + 1 == current->rows) {
        output_chars(current, "\n", 1);
    }
    flush_output(current);
    current->x = 0;
    current->y++;
}

static void set_output_highlight(struct current *current, const int *props, int nprops)
{
    int colour = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
    int bold = 0;
    int reverse = 0;
    int i;

    for (i = 0; i < nprops; i++) {
        switch (props[i]) {
            case 0:
               colour = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
               bold = 0;
               reverse = 0;
               break;
            case 1:
               bold = FOREGROUND_INTENSITY;
               break;
            case 7:
               reverse = 1;
               break;
            case 30:
               colour = 0;
               break;
            case 31:
               colour = FOREGROUND_RED;
               break;
            case 32:
               colour = FOREGROUND_GREEN;
               break;
            case 33:
               colour = FOREGROUND_RED | FOREGROUND_GREEN;
               break;
            case 34:
               colour = FOREGROUND_BLUE;
               break;
            case 35:
               colour = FOREGROUND_RED | FOREGROUND_BLUE;
               break;
            case 36:
               colour = FOREGROUND_BLUE | FOREGROUND_GREEN;
               break;
            case 37:
               colour = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
               break;
        }
    }

    flush_output(current);

    if (reverse) {
        SetConsoleTextAttribute(current->outh, BACKGROUND_INTENSITY);
    }
    else {
        SetConsoleTextAttribute(current->outh, colour | bold);
    }
}

static void erase_eol(struct current *current)
{
    COORD pos;
    DWORD n;

    pos.X = (SHORT) current->x;
    pos.Y = (SHORT) current->y;

    FillConsoleOutputCharacter(current->outh, ' ', current->cols - current->x, pos, &n);
}

static void set_cursor_xy(struct current *current)
{
    COORD pos;

    pos.X = (SHORT) current->x;
    pos.Y = (SHORT) current->y;

    SetConsoleCursorPosition(current->outh, pos);
}


static void set_cursor_pos(struct current *current, int x)
{
    current->x = x;
    set_cursor_xy(current);
}

static void cursor_up(struct current *current, int n)
{
    current->y -= n;
    set_cursor_xy(current);
}

static void cursor_down(struct current *current, int n)
{
    current->y += n;
    set_cursor_xy(current);
}

static int fd_read(struct current *current)
{
    while (1) {
        INPUT_RECORD irec;
        DWORD n;
        if (WaitForSingleObject(current->inh, INFINITE) != WAIT_OBJECT_0) {
            break;
        }
        if (!ReadConsoleInputW(current->inh, &irec, 1, &n)) {
            break;
        }
        if (irec.EventType == KEY_EVENT) {
            KEY_EVENT_RECORD *k = &irec.Event.KeyEvent;
            if (k->bKeyDown || k->wVirtualKeyCode == VK_MENU) {
                if (k->dwControlKeyState & ENHANCED_KEY) {
                    switch (k->wVirtualKeyCode) {
                     case VK_LEFT:
                        return SPECIAL_LEFT;
                     case VK_RIGHT:
                        return SPECIAL_RIGHT;
                     case VK_UP:
                        return SPECIAL_UP;
                     case VK_DOWN:
                        return SPECIAL_DOWN;
                     case VK_INSERT:
                        return SPECIAL_INSERT;
                     case VK_DELETE:
                        return SPECIAL_DELETE;
                     case VK_HOME:
                        return SPECIAL_HOME;
                     case VK_END:
                        return SPECIAL_END;
                     case VK_PRIOR:
                        return SPECIAL_PAGE_UP;
                     case VK_NEXT:
                        return SPECIAL_PAGE_DOWN;
                    }
                }
                /* Note that control characters are already translated in AsciiChar */
                else if (k->wVirtualKeyCode == VK_CONTROL)
                    continue;
                else {
                    return k->uChar.UnicodeChar;
                }
            }
        }
    }
    return -1;
}

static int get_window_size(struct current *current)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(current->outh, &info)) {
        return -1;
    }
    current->cols = info.dwSize.X;
    current->rows = info.dwSize.Y;
    if (current->cols <= 0 || current->rows <= 0) {
        current->cols = 80;
        return -1;
    }
    current->y = info.dwCursorPosition.Y;
    current->x = info.dwCursorPosition.X;
    return 0;
}
