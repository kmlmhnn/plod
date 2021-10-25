#include <ncurses.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <libnotify/notify.h>
#include <canberra.h>

#define L 256

#define M1() do { raw(); noecho(); nodelay(stdscr, TRUE); curs_set(FALSE); } while(0)
#define M0() do { noraw(); echo(); nodelay(stdscr, FALSE); curs_set(TRUE); } while(0)

int match(const char *s, const char *p);

int main() {
    enum {C, D, B, W, U, M} m = C;
    char s[M][L] = {};
    char p[M][L] = {};
    int  b0      = 0;

    initscr(); M1();
    notify_init("plod");
    ca_context *ca; ca_context_create(&ca);
    while(1) {
        time_t t = time(NULL); struct tm *tm = localtime(&t);
        strftime(s[D], L, "%d/%m/%y", tm);
        strftime(s[C], L, "%I:%M", tm);

        FILE *f = fopen("/sys/class/power_supply/BAT0/capacity", "r");
        fscanf(f, "%s", s[B]);
        fclose(f);

        f = fopen("/proc/net/dev", "r");
        char buf[L];
        while(fgets(buf, L, f)) {
            int  b1;
            if(sscanf(buf, "wlp1s0: %d", &b1)) {
                double db = b1 - b0;
                b0 = b1;
                sprintf(s[W], "%d", (int) db / 1024);
                break;
            }
        }
        fclose(f);

        f = fopen("/proc/loadavg", "r");
        float l; fscanf(f, "%f", &l);
        sprintf(s[U], "%.f", l * 25);
        fclose(f);

        switch(getch()) {
            case 'c': m = C; break;
            case 'd': m = D; break;
            case 'b': m = B; break;
            case 'w': m = W; break;
            case 'u': m = U; break;
            case '/': clear(); M0(); getnstr(p[m], L); M1(); break;
            case 'q': endwin(); exit(0);
        }

        clear(); mvprintw(0, 0, s[m]); mvprintw(1, 0, p[m]); refresh();

        for(int i = 0; i < M; i++) {
            if(match(s[i], p[i])) {
                char msg[3*L]; sprintf(msg, "%c matched: %s = %s", "cdbwu"[i], p[i], s[i]);
                NotifyNotification *n = notify_notification_new(msg, NULL, NULL);
                notify_notification_show(n, NULL);
                g_object_unref(G_OBJECT(n));
                ca_context_play(ca, 0, CA_PROP_EVENT_ID, "complete", CA_PROP_EVENT_DESCRIPTION, "match", NULL);
                strcpy(p[i], "");
            }
        }

        sleep(1);
    }
}

int match(const char *s, const char *p) {
    int l, m;
    for(l = strlen(s), m = strlen(p); l > 0 && (*s == *p || *p == '_'); s++, p++, --l, --m);
    return !(l+m);
}
