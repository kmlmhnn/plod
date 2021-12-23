#include <ncurses.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <libnotify/notify.h>
#include <canberra.h>

#define L 256
enum {C, D, B, W, U, M} m = C;
char s[M][L] = {};
char p[M][L] = {};

int TO;
int sticky[M] = {};
time_t lastnotified[M] = {};

void setcmdmode(int);
void setinsmode();
void loaddata(time_t *);
void sendnotifications(ca_context *, time_t);

int match(char *, char *);

int main(int argc, char* argv[]) {
    int T  = argc > 1 ? *argv[1] - '0' : 1;
        TO = argc > 2 ? *argv[2] - '0' : 1;

    initscr();
    setcmdmode(T);

    notify_init("plod");
    ca_context *ca;
    ca_context_create(&ca);

    int c = ERR;
    time_t t0 = time(NULL);
    while(1) {
        time_t t = time(NULL);

        if(c == ERR || t-t0 >= T) {
            loaddata(&t);
            sendnotifications(ca, t);
            t0 = t;
        }

        clear(); mvprintw(0, 0, s[m]); mvprintw(1, 0, p[m]); refresh();

        switch(c = getch()) {
            case 'c': m = C; break;
            case 'd': m = D; break;
            case 'b': m = B; break;
            case 'w': m = W; break;
            case 'u': m = U; break;
            case '/': clear(); setinsmode(); getnstr(p[m], L); sticky[m] = 0; setcmdmode(T); break;
            case '?': clear(); setinsmode(); getnstr(p[m], L); sticky[m] = 1; setcmdmode(T); break;
            case 'q': endwin(); exit(0);
        }
    }
}

void setcmdmode(int t)  { raw(); noecho(); timeout(t*1000); curs_set(FALSE);}
void setinsmode()       { noraw(); echo(); timeout(-1); curs_set(TRUE);     }

void loaddata(time_t *t) {
    struct tm *tm = localtime(t);
    strftime(s[D], L, "%d/%m/%y", tm);
    strftime(s[C], L, "%I:%M", tm);

    FILE *f = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    fscanf(f, "%s", s[B]);
    fclose(f);

    static int b0 = 0;
    f = fopen("/proc/net/dev", "r");
    char buf[L];
    while(fgets(buf, L, f)) {
        int b;
        if(sscanf(buf, "wlp1s0: %d", &b)) {
            double db = b - b0;
            b0 = b;
            sprintf(s[W], "%d", (int) db / 1024);
            break;
        }
    }
    fclose(f);

    f = fopen("/proc/loadavg", "r");
    float l;
    fscanf(f, "%f", &l);
    sprintf(s[U], "%.f", l * 25);
    fclose(f);
}

void sendnotifications(ca_context *ca, time_t t) {
    for(int i = 0; i < M; i++) {
        if(match(s[i], p[i])) {
            if(t-lastnotified[i] >= TO*60) {
                char msg[3*L]; sprintf(msg, "%c matched: %s = %s", "cdbwu"[i], p[i], s[i]);
                NotifyNotification *n = notify_notification_new(msg, NULL, NULL);
                notify_notification_show(n, NULL);
                g_object_unref(G_OBJECT(n));
                ca_context_play(ca, 0, CA_PROP_EVENT_ID, "complete", CA_PROP_EVENT_DESCRIPTION, "match", NULL);

                lastnotified[i] = sticky[i] ? t : 0;

                if(!sticky[i]) {
                    strcpy(p[i], "");
                }
            }
        }
    }
}

int match(char *s, char *p) {
    int l, m;
    for(l = strlen(s), m = strlen(p); l > 0 && (*s == *p || *p == '_'); ++s, ++p, --l, --m);
    return !(l+m);
}
