#include <FL/Fl.H>
#include <iconv.h>
#include <errno.h>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tree.H>
#include <curl/curl.h>
#include <stdio.h>
#include <regex.h>
#include <vector>

#include "Fl_GIF.H"
#include "folder_open_small.xpm"
#include "folder_closed_small.xpm"
#include "icons.h"
#include "plus.h"

enum types {
    CATEGORIES,
    ALBUM,
    SONG
};

struct xferinfo {
    char *buf;
    int len;
    enum types type;
    Fl_Tree *tree;
    Fl_Tree_Item *item;
    const char *url;
};

struct urlinfo {
    char *name;
    int name_len;
    char *url;
    int url_len;
};

static Fl_Pixmap folder_open(folder_open_small_xpm);
static Fl_Pixmap folder_closed(folder_closed_small_xpm);
static Fl_Pixmap document(document_xpm);
static Fl_GIF *spinner = new Fl_GIF("ajax.gif");


void Button_CB(Fl_Widget*w, void*data) {
    fprintf(stderr, "'%s' button pushed\n", w->label());
}

const char* reason_as_name(Fl_Tree_Reason reason) {
    switch ( reason ) {
    case FL_TREE_REASON_NONE:       return("none");
    case FL_TREE_REASON_SELECTED:   return("selected");
    case FL_TREE_REASON_DESELECTED: return("deselected");
    case FL_TREE_REASON_OPENED:     return("opened");
    case FL_TREE_REASON_CLOSED:     return("closed");
    default:                        return("???");
    }
}

void fix_icons(Fl_Tree *tree){
    for (Fl_Tree_Item *item = tree->first(); item; item=item->next()){
        if (item->has_children()){
            if (item->is_open()){
                item->usericon(&folder_open);
            } else {
                item->usericon(&folder_closed);
            }
        } else {
            item->usericon(&document);
        }
    }
}

void remove_spinner(Fl_Tree_Item *item){
    struct xferinfo *info = (struct xferinfo *)item->user_data();

    spinner->animating(false);
    if (info){
        info->tree->redraw();
    }
}

static void cb_tree(Fl_Tree *tree, void *data){
    Fl_Tree_Item *item = tree->callback_item();
    if (!item){
        return;
    }

    struct xferinfo *info = (struct xferinfo *)item->user_data();

    printf("label %s reason %s\n", item->label(), reason_as_name(tree->callback_reason()));
    if (tree->callback_reason() == FL_TREE_REASON_SELECTED || tree->callback_reason() == FL_TREE_REASON_DESELECTED){
        tree->open_toggle(item, 0);
    } else 
    if (item->has_children()){
        if (item->is_open()){
            item->usericon(&folder_open);
        } else {
            item->usericon(&folder_closed);
        }
    } else if (tree->callback_reason() == FL_TREE_REASON_SELECTED){
        item->usericon(spinner);
        spinner->animating(true);
        spinner->parent(tree);
        Fl::add_timeout(2.0, (Fl_Timeout_Handler)remove_spinner, (void *)item);
    }
}


struct urlinfo grab_url(regex_t *preg, char **pos){
    struct urlinfo info = {0};
    regmatch_t pmatch[3];
    size_t rm;

    rm = regexec (preg, *pos, 3, pmatch, 0);
    if (rm != REG_NOMATCH){
        info.name = *pos + pmatch[1].rm_so;
        info.url  = *pos + pmatch[2].rm_so;
        info.name_len = pmatch[1].rm_eo - pmatch[1].rm_so;
        info.url_len  = pmatch[2].rm_eo - pmatch[2].rm_so;
        *pos = *pos + pmatch[2].rm_eo;
    }
    return info;
}

int substr_count(const char *haystack, const char *needle) {
    const char *p;
    int count = 0;
    size_t len = strlen(needle);

    while ((p = strstr(haystack, needle))) {
        count++;
        haystack = p + len;
    }

    return count;
}

char* str_replace(char *from, char *to, char *str){
    int count = substr_count(str, from);

    if (count < 1) return str;

    int width_change = strlen(to) - strlen(from);
    char *res = (char *)malloc(strlen(str) + count * width_change + 1);
    res[0] = '\0';

    char *pos = str;
    char *p;
    while((p = strcasestr(pos, from))){
        strncat(res,pos,p-pos);
        strcat(res,to);
        pos = p + strlen(from);
    }
    strcat(res,pos);
    free(str);
    return res;
}

void look_for_categories(char *buf, Fl_Tree *tree){
    regex_t preg;
    char *pos = buf;
    struct urlinfo info;

    regcomp (&preg, "<a [^>]*title=\"([^\"]+)[^>]*href=\"([^\"]+)\">", REG_EXTENDED);

    while ((info = grab_url(&preg, &pos)), info.name_len){
        char *name = strndup(info.name, info.name_len);
        name = str_replace("【","/",name);
        name = str_replace("】","/",name);
        name = str_replace("专辑","/",name);
        Fl_Tree_Item *item = tree->add(name);
        free(name);
        struct xferinfo *xinfo = (struct xferinfo *)malloc(sizeof(struct xferinfo));
        xinfo->tree = tree;
        xinfo->url = strndup(info.url, info.url_len);
        xinfo->buf = NULL;
        xinfo->len = 0;
        xinfo->type = ALBUM;
        xinfo->item = item;
        item->user_data(xinfo);
    }

    regfree(&preg);

    int skip = 2; /* don't close the root or the first category */
    for(Fl_Tree_Item *item = tree->first(); item; item=item->next()){
        if (skip){
            skip--;
        } else {
            tree->close(item, 0);
        }
    }

    fix_icons(tree);

    tree->redraw();

    regfree (&preg);
}

char* to_utf8(char *src, size_t srclen){
    char *dest = (char *)malloc(srclen * 2);
    size_t destlen = srclen * 2;

    char * pIn = src;
    char * pOut = dest;

    iconv_t conv = iconv_open("utf8", "gb2312");
    iconv(conv, &pIn, &srclen, &pOut, &destlen);
    iconv_close(conv);

    free(src);
    return dest;
}
	
void handle_completed_transfer(CURLM *multi){
    CURLMsg *msg; 
    CURL *easy;
    int num;
    while ((msg = curl_multi_info_read(multi, &num))){
        if (msg->msg == CURLMSG_DONE){
            struct xferinfo *info;
            easy = msg->easy_handle;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, (char **)&info);
            if (info->type == CATEGORIES){
                info->buf = to_utf8(info->buf, info->len);
                look_for_categories(info->buf, info->tree);
                free(info->buf);
            }
            free(info);
            curl_multi_remove_handle(multi, easy);
            curl_easy_cleanup(easy);
        }
    }
}

size_t writefunc(char *ptr, size_t size, size_t nmemb, void *userdata){
    struct xferinfo *info = (struct xferinfo *)userdata;

    info->buf = (char *)realloc(info->buf, info->len + size * nmemb);
    memcpy(info->buf + info->len, ptr, size * nmemb);
    info->len += size * nmemb;

    return size * nmemb;
}

void do_xfer(CURLM *multi){
    int still_running;
    curl_multi_perform(multi, &still_running);
    if (still_running){
        Fl::repeat_timeout(0.1, (Fl_Timeout_Handler)do_xfer, (void *)multi);
    } else {
        handle_completed_transfer(multi);
    }
}

void load_categories(CURLM *multi, Fl_Tree *tree){
    CURL *easy = curl_easy_init();
    struct xferinfo *info = (struct xferinfo *)malloc(sizeof(struct xferinfo));
    info->type = CATEGORIES;
    info->buf = NULL;
    info->len = 0;
    info->tree = tree;
    info->url = NULL;
    curl_easy_setopt(easy, CURLOPT_URL, "localhost/djayan/List_1.html");
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, (void *)info);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(easy, CURLOPT_PRIVATE, (void *)info);
    curl_easy_setopt(easy, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(easy, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)");
    curl_multi_add_handle(multi, easy);
    Fl::add_timeout(0.1, (Fl_Timeout_Handler)do_xfer, (void *)multi);
}

int main(int argc, char **argv) {
    Fl::set_font(FL_HELVETICA, "Droid Sans Fallback");
    Fl_Double_Window *window = new Fl_Double_Window(550, 450, "tree");
    Fl_Tree *tree = new Fl_Tree(5, 5, 550-10, 450-10, "Tree");
    tree->end();
    window->end();

    tree->showroot(0);
    tree->callback((Fl_Callback *)cb_tree, (void *)NULL);

    CURLM *multi = curl_multi_init();
    load_categories(multi, tree);

    window->resizable(tree);
    window->end();

    window->show(argc, argv);
    return Fl::run();
}
