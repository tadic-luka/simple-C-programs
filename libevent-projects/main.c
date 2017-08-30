#include <err.h>
#include <time.h>
#include <linux/input.h>
#include <event.h>
#include <alsa/asoundlib.h>
#include <libnotify/notify.h>
#include "../screenshot/screenshot.h"

#define KEY_PRESS 1
#define KEY_RELEASE 0
#define PRTSCR 99
#define APP_NAME "VOLUME_MIXER"

static void
set_notification(NotifyNotification **notif, const char *header, const char *body, const char *icon, const char *key, GVariant *value);
int
setnonblock(int fd);
static void
key_input(int fd, short kind, void *userp);


struct event_data {
	int fd;
	NotifyNotification *notification;
	snd_mixer_t *handle;
	snd_mixer_elem_t* elem;
	long max_vol, min_vol;
	long cur_vol;
	char muted;
	time_t last_notif;
	char left_meta_pressed;
};

const char *card = "default";
const char *selem_name = "Master";
const char *file = "/dev/input/event0";
const char *scr_filename = "/home/luka/Pictures/screen.jpg";


static int
init_program(struct event *ev, struct event_data *data)
{
	int fd;

	event_init();
	notify_init(APP_NAME);
	data->notification = NULL;
	data->last_notif = time(NULL);
	data->left_meta_pressed = 0;
	fd = open(file, O_RDONLY | O_NONBLOCK);

	if(fd < 0) {
		err(1, "open %s", file);
	}

	data->fd = fd;

	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);

	if(snd_mixer_selem_register(handle, NULL, NULL) != 0) {
		fprintf(stderr, "Error with registering handle!\n");
		exit(1);
	}
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

	long min, max;
	long val;
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &val);
	data->cur_vol = val;
	data->handle = handle;
	data->elem = elem;
	data->min_vol = min;
	data->max_vol = max;
	data->muted = 0;

	event_set(ev, fd, EV_READ | EV_PERSIST, key_input, data);
	event_add(ev, NULL);
}
/* 
 *   Set a socket to non-blocking mode.
 */
int
setnonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0)
		return -1;

	return 0;
}

static void
set_volume_notification(struct event_data *data)
{
	time_t now = time(NULL);
	time_t delta = now - data->last_notif;
	long vol = 100 * (data->cur_vol - data->min_vol) / (data->max_vol - data->min_vol);
	data->last_notif = now;
	if(delta > 3) {
		notify_uninit();
		data->notification = NULL;
	}
	char *icon = NULL;
	if(data->muted) {
		icon = "audio-volume-muted";
		set_notification(&(data->notification), "0", NULL, icon, "value", g_variant_new_int32(vol));
		return;
	}
	if(vol > 69) {
		icon = "audio-volume-high";
	} else if(vol > 29) {
		icon = "audio-volume-medium";
	} else if(vol) {
		icon = "audio-volume-low";
	} else {
		icon = "audio-volume-muted";
	}
	char buf[5];
	sprintf(buf, "%ld", vol);
	set_notification(&(data->notification), buf, NULL, icon, "value", g_variant_new_int32(vol));
}
static void
set_notification(NotifyNotification **notif, const char *header, const char *body, const char *icon, const char *key, GVariant *value)
{
	if(*notif == NULL) {
		notify_init(APP_NAME);
		*notif = notify_notification_new(header, body, icon);
		notify_notification_set_timeout (*notif, 3000);
	}
	else
		notify_notification_update(*notif, header, body, icon);

	notify_notification_set_hint(*notif, key, value);
	notify_notification_show(*notif, NULL);
}

static void
set_volume(snd_mixer_elem_t* elem, long vol) {
	snd_mixer_selem_set_playback_switch_all(elem, vol);
	/*snd_mixer_selem_set_playback_volume_all(elem, min);*/
}
static void
change_volume(struct event_data *data, int sign)
{
	data->cur_vol += 0.1 * data->max_vol * sign;

	if(data->cur_vol > data->max_vol) 
		data->cur_vol = data->max_vol;
	else if(data->cur_vol < data->min_vol) 
		data->cur_vol = data->min_vol;

	snd_mixer_selem_set_playback_volume_all(data->elem, data->cur_vol);
}


static void
key_input(int fd, short kind, void *userp)
{
	long vol;
	int sign = 0;
	struct event_data *data = (struct event_data *) userp;
	struct input_event in_ev;
	if(read(fd, &in_ev, sizeof(struct input_event)) <= 0) {
		return;
	}
	if(in_ev.type == EV_KEY && in_ev.value == KEY_PRESS) {
		switch(in_ev.code) {
			case KEY_VOLUMEUP:
				sign = 1;
				break;
			case KEY_VOLUMEDOWN:
				sign = -1;
				break;
			case KEY_MUTE:
				if(data->muted) {
					data->muted = 0;
					vol = 100 * (data->cur_vol - data->min_vol) / (data->max_vol - data->min_vol);
					set_volume(data->elem, data->cur_vol);	
				} else {
					data->muted = 1;
					set_volume(data->elem, data->min_vol);	
					vol = 100 * (data->cur_vol - data->min_vol) / (data->max_vol - data->min_vol);
				}
				set_volume_notification(data);
				break;
			case PRTSCR:
				if(data->left_meta_pressed)
					scrshot(0, scr_filename);
				else 
					scrshot(1, scr_filename);
				break;
			case KEY_LEFTMETA:
				data->left_meta_pressed = 1;
				break;
		}
	} else if(in_ev.type == EV_KEY && in_ev.value == KEY_RELEASE) {
		if(in_ev.code == KEY_LEFTMETA) 
			data->left_meta_pressed = 0;
	}
	if(sign) {
		change_volume(data, sign);
			vol = 100 * (data->cur_vol - data->min_vol) / (data->max_vol - data->min_vol);
		set_volume_notification(data);
	}	
}

	static void
clean_program(int fd, short kind, void *userp)
{
	struct event_data *data = (struct event_data *) userp;
	snd_mixer_close(data->handle);
	notify_uninit();
	close(data->fd);
	exit(0);
}
int main(void)
{
	struct event_data data;
	struct event ev;
	struct event signal_event;

	init_program(&ev, &data);

	event_set(&signal_event, SIGTERM, EV_SIGNAL, clean_program, &data);
	event_add(&signal_event, NULL);

	if (daemon(1, 0) == -1) {
		err(1, "daemon");
	}

	event_dispatch();


	return EXIT_SUCCESS;
}
