#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <dirent.h>

#include <ncurses.h>

#define YEAR_RANGE 1
#define CAL_WIDTH 21
#define ASIDE_WIDTH 4
#define MAX_MONTH_HEIGHT 6

int cy, cx;
time_t raw_time;
struct tm today;
struct tm curs_date;
struct tm cal_start;
struct tm cal_end;

static const char* WEEKDAYS[] = {"Mo","Tu","We","Th","Fr","Sa","Su", NULL};

void get_date_str(struct tm* date, char* date_str, int date_str_size) {
    strftime(date_str, date_str_size, "%Y-%m-%d", date);
}

void draw_wdays(WINDOW* head) {
    char** wd;
    for (wd = (char**)WEEKDAYS; *wd; wd++) {
        waddstr(head, *wd);
        waddch(head, ' ');
    }
    wrefresh(head);
}

void draw_calendar(WINDOW* number_pad, WINDOW* month_pad, char* diary_dir) {
    struct tm i = cal_start;
    char month[10];
    bool has_entry;

    while (mktime(&i) <= mktime(&cal_end)) {
        has_entry = access(fpath(dirary_dir, &i), F_OK) != -1);
        getyx(number_pad, cy, cx);
        if (has_entry)
            wattron(number_pad, A_BOLD);
        wprintw(number_pad, "%2i", i.tm_mday);
        if (has_entry)
            wattroff(number_pad, A_BOLD);
        waddch(number_pad, ' ');

        if (i.tm_mday == 1) {
            strftime(month, sizeof month, "%b", &i);
            getyx(number_pad, cy, cx);
            mvwprintw(month_pad, cy, 0, "%s ", month);
        }

        i.tm_mday++;
        mktime(&i);
    }
}

void update_date(WINDOW* dh) {
    char dstr[16];
    mktime(&curs_date);
    get_date_str(&curs_date, dstr, sizeof(dstr));
    mvwaddstr(dh, 0, 0, dstr);
    wrefresh(dh);
}

char* fpath(char* dir, struct tm* date) {
    char path[100];
    char dstr[16];

    // add path of the diary dir
    strcpy(path, dir);
    if (dir[strlen(dir) - 1] != '/')
        strcat(path, "/");

    // append date to the path
    get_date_str(date, dstr, sizeof(dstr));
    strcat(path, dstr);

    return path;
}

char* edit_cmd(char* dir, struct tm* date) {
    static char ecmd[150];
    char* editor = getenv("EDITOR");
    if (editor == NULL)
        return NULL;

    strcpy(ecmd, editor);
    strcat(ecmd, " ");
    strcat(ecmd, fpath(dir, date));

    return ecmd;
}

bool is_leap(int year) {
    // normally leap is every 4 years,
    // but is skipped every 100 years,
    // unless it is divisible by 400
    return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}

WINDOW* read_diary(char* dir, struct tm* date, WINDOW* prev, int width) {
    wclear(prev);
    char buff[width];
    int i = 0;
    char* path = fpath(dir, date);

    FILE* fp =  fopen(path, "r");
    if (fp != NULL) {
        while(fgets(buff, width, fp) != NULL) {
            mvwprintw(prev, i, 0, buff);
            i++;
        }
        fclose(fp);
    }
    wrefresh(prev);

    return prev;
}

bool go_to(WINDOW* calendar, WINDOW* month_sidebar, time_t date, int* cur_pad_pos) {
    if (date < mktime(&cal_start) || date > mktime(&cal_end))
        return false;

    int diff_seconds = date - mktime(&cal_start);
    int diff_days = diff_seconds / 60 / 60 / 24;
    int diff_weeks = diff_days / 7;
    int diff_wdays = diff_days % 7;

    localtime_r(&date, &curs_date);

    getyx(calendar, cy, cx);

    // Remove the STANDOUT attribute from the day we are leaving
    chtype current_attrs = mvwinch(calendar, cy, cx) & A_ATTRIBUTES;
    // Leave every attr as is, but turn off STANDOUT
    current_attrs &= ~A_STANDOUT;
    mvwchgat(calendar, cy, cx, 2, current_attrs, 0, NULL);

    // Add the STANDOUT attribute to the day we are entering
    chtype new_attrs =  mvwinch(calendar, diff_weeks, diff_wdays * 3) & A_ATTRIBUTES;
    new_attrs |= A_STANDOUT;
    mvwchgat(calendar, diff_weeks, diff_wdays * 3, 2, new_attrs, 0, NULL);

    if (diff_weeks < *cur_pad_pos)
        *cur_pad_pos = diff_weeks;
    if (diff_weeks > *cur_pad_pos + LINES - 2)
        *cur_pad_pos = diff_weeks - LINES + 2;
    prefresh(month_sidebar, *cur_pad_pos, 0, 1,           0, LINES - 1, ASIDE_WIDTH);
    prefresh(calendar,      *cur_pad_pos, 0, 1, ASIDE_WIDTH, LINES - 1, ASIDE_WIDTH + CAL_WIDTH);

    return true;
}

void setup_cal_timeframe() {
    raw_time = time(NULL);
    localtime_r(&raw_time, &today);
    curs_date = today;

    cal_start = today;
    cal_start.tm_year -= YEAR_RANGE;
    cal_start.tm_mon = 0;
    cal_start.tm_mday = 1;
    mktime(&cal_start);

    if (cal_start.tm_wday != 1) {
        // adjust start date to first Mon before 01.01
        cal_start.tm_year--;
        cal_start.tm_mon = 11;
        cal_start.tm_mday = 31 - cal_start.tm_wday + 2;
        mktime(&cal_start);
    }

    cal_end = today;
    cal_end.tm_year += YEAR_RANGE;
    cal_end.tm_mon = 11;
    cal_end.tm_mday = 31;
    mktime(&cal_end);
}

int main(int argc, char** argv) {
    // Get the diary directory via environment variable or argument
    // If both are given, the argument takes precedence
    char* diary_dir = NULL;
    if (argc < 2) {
        diary_dir = getenv("DIARY_DIR");
        if (diary_dir == NULL) {
            fprintf(stderr, "The diary directory must ge given as command line "
                            "argument or in the DIARY_DIR environment variable\n");
            return 1;
        }
    } else {
        diary_dir = argv[1];
    }

    // Check if that directory exists
    DIR* diary_dir_ptr = opendir(diary_dir);
    if (diary_dir_ptr) {
        // Directory exists, continue
        closedir(diary_dir_ptr);
    } else if (errno == ENOENT) {
        fprintf(stderr, "The directory '%s' does not exist\n", diary_dir);
        return 2;
    } else {
        fprintf(stderr, "The directory '%s' could not be opened\n", diary_dir);
        return 1;
    }

    setup_cal_timeframe();

    initscr();
    raw();
    curs_set(0);

    WINDOW* date_header = newwin(1, 10, 0, ASIDE_WIDTH + CAL_WIDTH);
    wattron(date_header, A_BOLD);
    update_date(date_header);
    WINDOW* wdays_header = newwin(1, 3 * 7, 0, ASIDE_WIDTH);
    draw_wdays(wdays_header);

    WINDOW* aside = newpad((YEAR_RANGE * 2 + 1) * 12 * MAX_MONTH_HEIGHT, ASIDE_WIDTH);
    WINDOW* cal = newpad((YEAR_RANGE * 2 + 1) * 12 * MAX_MONTH_HEIGHT, CAL_WIDTH);
    keypad(cal, TRUE);
    draw_calendar(cal, aside, diary_dir);

    int ch, pad_pos = 0;
    struct tm new_date;
    int prev_width = COLS - ASIDE_WIDTH - CAL_WIDTH;
    int prev_height = LINES - 1;

    bool ret = go_to(cal, aside, raw_time, &pad_pos);
    // Mark current day
    chtype atrs = winch(cal) & A_ATTRIBUTES;
    wchgat(cal, 2, atrs | A_UNDERLINE, 0, NULL);
    prefresh(cal, pad_pos, 0, 1, ASIDE_WIDTH, LINES - 1, ASIDE_WIDTH + CAL_WIDTH);

    WINDOW* prev = newwin(prev_height, prev_width, 1, ASIDE_WIDTH + CAL_WIDTH);
    prev = read_diary(diary_dir, &today, prev, prev_width);

    do {
        ch = wgetch(cal);
        new_date = curs_date;
        char* ecmd = edit_cmd(diary_dir, &new_date);

        switch(ch) {
            // Basic movements
            case 'j':
            case KEY_DOWN:
                new_date.tm_mday += 7;
                ret = go_to(cal, aside, mktime(&new_date), &pad_pos);
                break;
            case 'k':
            case KEY_UP:
                new_date.tm_mday -= 7;
                ret = go_to(cal, aside, mktime(&new_date), &pad_pos);
                break;
            case 'l':
            case KEY_RIGHT:
                new_date.tm_mday++;
                ret = go_to(cal, aside, mktime(&new_date), &pad_pos);
                break;
            case 'h':
            case KEY_LEFT:
                new_date.tm_mday--;
                ret = go_to(cal, aside, mktime(&new_date), &pad_pos);
                break;

            // Jump to top/bottom of page
            case 'g':
                ret = go_to(cal, aside, mktime(&cal_start), &pad_pos);
                break;
            case 'G':
                ret = go_to(cal, aside, mktime(&cal_end), &pad_pos);
                break;

            // Jump backward/forward by a month
            case 'K':
                if (new_date.tm_mday == 1)
                    new_date.tm_mon--;
                new_date.tm_mday = 1;
                ret = go_to(cal, aside, mktime(&new_date), &pad_pos);
                break;
            case 'J':
                new_date.tm_mon++;
                new_date.tm_mday = 1;
                ret = go_to(cal, aside, mktime(&new_date), &pad_pos);
                break;

            // Today shortcut
            case 't':
                new_date = today;
                ret = go_to(cal, aside, raw_time, &pad_pos);
                break;
            // Edit/create a diary entry
            case 'e':
            case '\n':
                if (ecmd) {
                    curs_set(1);
                    system(ecmd);
                    curs_set(0);
                    keypad(cal, TRUE);
                }
                break;
        }

        if (ret) {
            update_date(date_header);

            // adjust prev width if terminal was resized in the mean time
            prev_width = COLS - ASIDE_WIDTH - CAL_WIDTH;
            wresize(prev, prev_height, prev_width);

            prev = read_diary(diary_dir, &curs_date, prev, prev_width);
        }
    } while (ch != 'q');

    endwin();
    system("clear");
    return 0;
}
