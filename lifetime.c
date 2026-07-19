#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500

#include <math.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Configuration
#define PRECISION 10
static const char *BIRTHDAY = "2000-01-01 00:00:00 +0000";
static const long SLEEP_NANOSECONDS = 10'000'000;

#define _STRINGIFY(x) #x
#define _STRINGIFY_MACRO(x) _STRINGIFY(x)
#define _PRECISION_STR _STRINGIFY_MACRO(PRECISION)
#define _STRFTIME_BUF_LEN 128
#define _STRFTIME_FORMAT "%F %T %z"

static inline bool is_leap(int year) {
  return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}

static void finish(int sig) {
  (void)sig;
  endwin();
  exit(EXIT_SUCCESS);
}

static void handle_resize(int sig) {
  (void)sig;
  endwin();
  refresh();
  clear();
}

int main() {
  struct tm birth_tp = {0};

  // parse and verify
  {
    // parse
    const auto *const result = strptime(BIRTHDAY, _STRFTIME_FORMAT, &birth_tp);
    if (result == nullptr || *result != '\0') {
      fprintf(stderr, "error: Invalid date string: %s\n", BIRTHDAY);
      exit(EXIT_FAILURE);
    }

    // convert to utc
    const auto gmtoff = birth_tp.tm_gmtoff;
    const auto birth = timegm(&birth_tp) - gmtoff;
    birth_tp = *gmtime(&birth);

    // verify that birthday happened in past
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    if (now.tv_sec < birth) {
      char buf[_STRFTIME_BUF_LEN];
      strftime(buf, _STRFTIME_BUF_LEN, _STRFTIME_FORMAT,
               localtime(&now.tv_sec));
      fprintf(stderr, "error: Birthday (%s) should be earlier than now (%s)\n",
              BIRTHDAY, buf);
      exit(EXIT_FAILURE);
    }
  }

  // exclude leaplings
  if (birth_tp.tm_mon == 1 && birth_tp.tm_mday == 29) {
    fprintf(stderr, "error: Leap birthday is unsupported\n");
    exit(EXIT_FAILURE);
  }

  // init ncurses
  {
    initscr();
    timeout(0);
    noecho();
    curs_set(0);
    signal(SIGINT, finish);
    signal(SIGWINCH, handle_resize);
  }

  while (1) {
    if (getch() == 'q') {
      break;
    }

    // get current age
    const auto years = ({
      struct timespec now;
      clock_gettime(CLOCK_REALTIME, &now);
      const auto *const now_tp = gmtime(&now.tv_sec);

      int last_birthday_year = now_tp->tm_year;
      time_t last_birthday;
      {
        auto this_year_birthday_tp = birth_tp;
        this_year_birthday_tp.tm_year = now_tp->tm_year;
        const auto this_year_birthday = timegm(&this_year_birthday_tp);

        if (this_year_birthday > now.tv_sec) {
          auto last_year_birthday_tp = this_year_birthday_tp;
          last_year_birthday_tp.tm_year--;
          last_birthday_year--;
          last_birthday = timegm(&last_year_birthday_tp);
        } else {
          last_birthday = this_year_birthday;
        }
      };

      const auto include_leap_day = birth_tp.tm_mon < 1
                                        ? is_leap(last_birthday_year + 1900)
                                        : is_leap(last_birthday_year + 1901);
      const auto total_days = 365 + include_leap_day;
      const auto now_nsec = now.tv_sec * 1'000'000'000 + now.tv_nsec;
      const auto last_birthday_nsec = last_birthday * 1'000'000'000;
      const auto total_nsec = (double)(now_nsec - last_birthday_nsec);
      (last_birthday_year - birth_tp.tm_year) +
          total_nsec / 1'000'000'000 / 60 / 60 / 24 / total_days;
    });

    // write
    {
      const auto row = getmaxy(stdscr);
      const auto col = getmaxx(stdscr);
      const auto centerX = col / 2;
      const auto centerY = row / 2;
      const auto int_part = (int)years != 0 ? ((int)log10(years) + 1) : 1;
      const auto width = int_part + 1 + PRECISION;

      // write text
      mvprintw(centerY, centerX - width / 2, "%." _PRECISION_STR "f", years);

      // write box
      const auto y1 = centerY - 1;
      const auto y2 = centerY + 1;
      const auto x1 = centerX - width / 2 - 1;
      const auto x2 = centerX + (width + 1) / 2;
      mvhline(y1, x1, 0, x2 - x1);
      mvhline(y2, x1, 0, x2 - x1);
      mvvline(y1, x1, 0, y2 - y1);
      mvvline(y1, x2, 0, y2 - y1);
      mvaddch(y1, x1, ACS_ULCORNER);
      mvaddch(y2, x1, ACS_LLCORNER);
      mvaddch(y1, x2, ACS_URCORNER);
      mvaddch(y2, x2, ACS_LRCORNER);
    }

    refresh();
    nanosleep(&(struct timespec){.tv_sec = 0, .tv_nsec = SLEEP_NANOSECONDS},
              nullptr);
  }

  endwin();
  return 0;
}
