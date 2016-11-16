#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>

#define YEAR_RANGE 1
#define CAL_WIDTH 21
#define ASIDE_WIDTH 4
#define MAX_MONTH_HEIGHT 6

int cy, cx;
time_t raw_time;
struct tm cur_date;
struct tm curs_date;
char curs_date_str[70];
struct tm cal_start;
struct tm cal_end;

void draw_wdays(WINDOW* head) {
    char* weekdays[]    = {"Mo","Tu","We","Th","Fr","Sa","Su"};
    for (int wd = 0; wd < sizeof(weekdays)/sizeof(weekdays[0]); wd++) {
        waddstr(head, weekdays[wd]);
        waddch(head, ' ');
    }
    wrefresh(head);
}

void draw_calendar(WINDOW* number_pad, WINDOW* month_pad) {
    struct tm i = cal_start;
    char month[10];

    while (timelocal(&i) <= timelocal(&cal_end)) {
        getyx(number_pad, cy, cx);
        mvwprintw(number_pad, cy, cx, "%2i", i.tm_mday);
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
    mktime(&curs_date);
    strftime(curs_date_str, sizeof curs_date_str, "%Y-%m-%d", &curs_date);
    mvwaddstr(dh, 0, 0, curs_date_str);
    wrefresh(dh);
}

char* curs_date_file_path(char* dir) {
    static char path[100];

    strcpy(path, dir);
    if (dir[strlen(dir) - 1] != '/')
        strcat(path, "/");
    strcat(path, curs_date_str);

    return path;
}

char* curs_date_edit_cmd(char* dir) {
    static char edit_cmd[150];
    char* editor = getenv("EDITOR");
    if (editor == NULL)
        return NULL;

    strcpy(edit_cmd, editor);
    strcat(edit_cmd, " ");
    strcat(edit_cmd, curs_date_file_path(dir));

    return edit_cmd;
}

bool is_leap(int year) {
    // normally leap is every 4 years,
    // but is skipped every 100 years,
    // unless it is divisible by 400
    if (year % 400 == 0)
        return true;
    if (year % 4 == 0 && year % 100 != 0)
        return true;

    return false;
}

void read_diary(char* dir) {
    int width = COLS - ASIDE_WIDTH - CAL_WIDTH;
    WINDOW* prev = newpad(LINES - 1, width);

    wclear(prev);
    char buff[width];
    int i = 0;
    char* path = curs_date_file_path(dir);

    FILE* fp =  fopen(path, "r");
    if (fp != NULL) {
        while(fgets(buff, width, fp) != NULL) {
            mvwprintw(prev, i, 0, buff);
            i++;
        }
        fclose(fp);
    }
    prefresh(prev, 0, 0, 1, CAL_WIDTH + ASIDE_WIDTH, LINES, COLS);
}

bool go_to(WINDOW* calendar, WINDOW* month_sidebar, time_t date, int* cur_pad_pos) {
    if (date < timelocal(&cal_start) || date > timelocal(&cal_end))
        return false;

    int diff_seconds = date - timelocal(&cal_start);
    int diff_days = diff_seconds / 60 / 60 / 24;
    int diff_weeks = diff_days / 7;
    int diff_wdays = diff_days % 7;

    localtime_r(&date, &curs_date);

    getyx(calendar, cy, cx);
    mvwchgat(calendar, cy        , 0             , -1, A_NORMAL,   0, NULL);
    mvwchgat(calendar, diff_weeks, diff_wdays * 3,  2, A_STANDOUT, 0, NULL);

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
    localtime_r(&raw_time, &cur_date);
    curs_date = cur_date;

    cal_start = cur_date;
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

    cal_end = cur_date;
    cal_end.tm_year += YEAR_RANGE;
    cal_end.tm_mon = 11;
    cal_end.tm_mday = 31;
    mktime(&cal_end);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Expect diary directory as command line argument\n");
        return 0;
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
    draw_calendar(cal, aside);

    int ch;
    struct tm new_date;
    char* diary_dir = argv[1];
    // init the current pad possition at the very end,
    // such that the cursor is displayed top of screen
    int pad_pos = 9999999;

    wmove(cal, 0, 0);
    getyx(cal, cy, cx);
    bool ret = go_to(cal, aside, raw_time, &pad_pos);
    read_diary(diary_dir);

    do {
        ch = wgetch(cal);
        new_date = curs_date;
        char* ecmd = curs_date_edit_cmd(diary_dir);

        switch(ch) {
            case 'j':
            case KEY_DOWN:
                new_date.tm_mday += 7;
                ret = go_to(cal, aside, timelocal(&new_date), &pad_pos);
                break;
            case 'k':
            case KEY_UP:
                new_date.tm_mday -= 7;
                ret = go_to(cal, aside, timelocal(&new_date), &pad_pos);
                break;
            case 'l':
            case KEY_RIGHT:
                new_date.tm_mday++;
                ret = go_to(cal, aside, timelocal(&new_date), &pad_pos);
                break;
            case 'h':
            case KEY_LEFT:
                new_date.tm_mday--;
                ret = go_to(cal, aside, timelocal(&new_date), &pad_pos);
                break;
            case 't':
                new_date = cur_date;
                ret = go_to(cal, aside, raw_time, &pad_pos);
                break;
            case 'e':
                if (ecmd)
                    system(ecmd);
                curs_set(0);
                break;
        }

        if (ret) {
            update_date(date_header);
            read_diary(diary_dir);
        }
    } while (ch != 'q');

    endwin();
    system("clear");
    return 0;
}
