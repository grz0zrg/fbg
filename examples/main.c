#include "fragment.h"

typedef struct SDL_MouseMotionEvent
{
    uint32_t type;        /**< ::SDL_MOUSEMOTION */
    uint32_t timestamp;
    uint32_t windowID;    /**< The window with mouse focus, if any */
    uint32_t which;       /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
    uint8_t state;        /**< The current button state */
    uint8_t padding1;
    uint8_t padding2;
    uint8_t padding3;
    int32_t x;           /**< X coordinate, relative to window */
    int32_t y;           /**< Y coordinate, relative to window */
    int32_t xrel;        /**< The relative motion in the X direction */
    int32_t yrel;        /**< The relative motion in the Y direction */
} SDL_MouseMotionEvent;

SDL_MouseMotionEvent mme;

int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len) {
    struct user_session_data *usd = (struct user_session_data *)user;
    int fd;
    unsigned char pid;
//    size_t remaining_payload;
    int is_final_fragment;

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            fd = lws_get_socket_fd(wsi);
            lws_get_peer_addresses(wsi, fd, usd->peer_name, PEER_NAME_BUFFER_LENGTH,
                usd->peer_ip, PEER_ADDRESS_BUFFER_LENGTH);

            printf("Connection successfully established from %s (%s).\n",
                usd->peer_name, usd->peer_ip);
            fflush(stdout);

            usd->packet = NULL;
            usd->packet_len = 0;
            usd->packet_skip = 0;

            // testing deflate options
            // lws_set_extension_option(wsi, "permessage-deflate", "rx_buf_size", "11");
            break;

        case LWS_CALLBACK_RECEIVE:
            is_final_fragment = lws_is_final_fragment(wsi);

            if (usd->packet_skip) {
                if (is_final_fragment) {
                    usd->packet_skip = 0;
                    usd->packet_len = 0;
                }

                return 0;
            }

            usd->packet_len += len;

//            remaining_payload = lws_remaining_packet_payload(wsi);

            if (usd->packet == NULL) {
                // we initialize the first fragment or the final one
                // this mechanism depend on the rx buffer size
                usd->packet = (char *)malloc(len);
                if (usd->packet == NULL) {
                    if (is_final_fragment) {
                        printf("A packet was skipped due to alloc. error.\n");
                    } else {
                        printf("A packet will be skipped due to alloc. error.\n");

                        usd->packet_skip = 1;
                    }
                    fflush(stdout);

                    return 0;
                }

                memcpy(usd->packet, &((char *) in)[0], len);

#ifdef DEBUG
    printf("\nReceiving packet...\n");
#endif
            } else {
                // accumulate the packet fragments to construct the final one
                char *new_packet = (char *)realloc(usd->packet, usd->packet_len);

                if (new_packet == NULL) {
                    free(usd->packet);
                    usd->packet = NULL;

                    usd->packet_skip = 1;

                    printf("A packet will be skipped due to alloc. error.\n");
                    fflush(stdout);

                    return 0;
                }

                usd->packet = new_packet;

                memcpy(&(usd->packet)[usd->packet_len - len], &((char *) in)[0], len);
            }
/*
#ifdef DEBUG
if (remaining_payload != 0) {
    printf("Remaining packet payload: %lu\n", remaining_payload);
}
#endif
*/
            if (is_final_fragment) {
#ifdef DEBUG
    printf("Full packet received, length: %lu\n", usd->packet_len);
#endif
                pid = usd->packet[0];

#ifdef DEBUG
    printf("Packet id: %u\n", pid);
    fflush(stdout);
#endif

                // packets processing
                if (pid == 0) {
                    memcpy(&mme, &((char *) usd->packet)[PACKET_HEADER_LENGTH], sizeof(SDL_MouseMotionEvent));

#ifdef DEBUG
    printf("SURFACE : %u, %i, %i, %i, %i\n", mme.state, mme.x, mme.y, mme.xrel, mme.yrel);
#endif
                }

//free_packet:
                free(usd->packet);
                usd->packet = NULL;

                usd->packet_len = 0;
            }
#ifdef DEBUG
    fflush(stdout);
#endif
            break;

        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
        case LWS_CALLBACK_CLOSED:
            printf("Connection from %s (%s) closed.\n", usd->peer_name, usd->peer_ip);
            fflush(stdout);
            break;

        default:
            break;
    }

    return 0;
}

static const struct lws_extension exts[] = {
	{
		"permessage-deflate",
		lws_extension_callback_pm_deflate,
		"permessage-deflate; client_no_context_takeover; client_max_window_bits"
	},
	{
		"deflate-frame",
		lws_extension_callback_pm_deflate,
		"deflate_frame"
	},
	{ NULL, NULL, NULL }
};

static struct lws_protocols protocols[] = {
	{
		"fas-protocol",
		ws_callback,
		sizeof(struct user_session_data),
		4096,
	},
	{ NULL, NULL, 0, 0 }
};

struct lws_context *context;

char *fs_iface = NULL;

unsigned int fs_rx_buffer_size = 1500;
unsigned int fs_port = 3000;
unsigned int fs_deflate = 0;
unsigned int fs_ssl = 0;

int start_server(void) {
    protocols[0].rx_buffer_size = fs_rx_buffer_size;

    struct lws_context_creation_info context_info = {
        .port = 3003, .iface = fs_iface, .protocols = protocols, .extensions = NULL,
        .ssl_cert_filepath = NULL, .ssl_private_key_filepath = NULL, .ssl_ca_filepath = NULL,
        .gid = -1, .uid = -1, .options = 0, NULL, .ka_time = 0, .ka_probes = 0, .ka_interval = 0
    };

    context_info.port = fs_port;

    if (fs_deflate) {
        context_info.extensions = exts;
    }

    if (fs_ssl) {
        context_info.ssl_private_key_filepath = "./server.key";
        context_info.ssl_cert_filepath = "./server.crt";
        context_info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    }

    context = lws_create_context(&context_info);

    if (context == NULL) {
        fprintf(stderr, "lws_create_context failed.\n");
        return -1;
    }

    printf("Fragment successfully started and listening on %s:%u.\n", (fs_iface == NULL) ? "127.0.0.1" : fs_iface, fs_port);

    return 0;
}

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

struct _fbg_img *texture;

struct _fragment_user_data {
    float pr;
    float xxm;
    float motion;
};

void *fragmentStart(struct _fbg *fbg) {
    struct _fragment_user_data *user_data = (struct _fragment_user_data *)calloc(1, sizeof(struct _fragment_user_data));

    user_data->xxm = 0;
    user_data->motion = 0;
    user_data->pr = 8.0f;

    return user_data;
}

void fragmentStop(struct _fbg *fbg, struct _fragment_user_data *data) {
    free(data);
}

void fragment(struct _fbg *fbg, struct _fragment_user_data *user_data) {
    float perlin_freq = 0.03 + abs(sin(user_data->motion * 8.)) * 8.;

    fbg_fclear(fbg, 0);
    //fbg_fade_down(fbg, 2);

    int x, y;
    for (y = fbg->task_id; y < fbg->height; y += 3) {
        float perlin_x = user_data->xxm * fbg->height;
        float perlin_y = user_data->xxm * fbg->height;

        int rep = 4;

        float rdx = (((float)rand() / RAND_MAX) * 2 - 1);
        float rdy = (((float)rand() / RAND_MAX) * 2 - 1);
        for (x = 0; x < fbg->width; x += 1) {
            float p = perlin2d(x + perlin_x, y, perlin_freq, 2);

            int r = p*255;
            int g = p*255;
            int b = p*255;

            int yy = fmin(fmax(y + user_data->pr * p, 0), fbg->height - 1);
            int xx = x;

            int ytl = (((yy * rep - (int)(user_data->motion * 4)) % texture->height) << 2) * texture->width;
            int xtl = ytl + (((x * rep + (int)(user_data->motion * 4)) % texture->width) << 2);

            r = texture->data[xtl] * (1. - p);
            g = texture->data[xtl + 1] * (1. - p);
            b = texture->data[xtl + 2] * fmin(1., (1. - p) * 4.);

            p *= 3;

            fbg_pixel(fbg, xx, yy, r, g, b);
        }
    }

    unsigned char tc[9] = {255, 0, 0, 0, 255, 0, 0, 0, 255};
    y = 0;
    for (y = fbg->task_id - 1; y < fbg->height; y += 3) {
        for (x = 0; x < fbg->width; x += 1) {
            int i = (x + y * fbg->width) * 3;
            fbg->back_buffer[i] = (fbg->task_id - 1) * 85;
            fbg->back_buffer[i + 1] = 0;
            fbg->back_buffer[i + 2] = 0;
        }
    }

    fbg_rect(fbg, fbg->task_id * 32, 0, 32, 32, 0, 0, 255);

    user_data->xxm += 0.001;
    user_data->motion += 0.001;
}

int main(int argc, char* argv[]) {
    int i, x, y;

    srand(time(NULL));

    if (start_server() < 0) {
        goto ws_error;
    }

    struct _fbg *fbg = fbg_init();


#ifdef __unix__
    signal(SIGINT, int_handler);
#endif

    texture = fbg_loadPNG("grass.png");
    struct _fbg_img *bbimg = fbg_loadPNG("bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bbimg, 8, 8, 33);

    fbg_createFragment(fbg, fragmentStart, fragment, fragmentStop, 3, 63);

    do {
        lws_service(context, 1);

        fbg_fclear(fbg, 0);

        //fbg_fade_down(fbg, 2);

        //usleep(1000 * 4);

        fbg_draw(fbg, 1);

        if (mme.state == 1 || mme.state == 4) {
            fbg_rect(fbg, mme.xrel, mme.yrel, 16, 16, 255, 255, 255);
        } else {
            fbg_rect(fbg, mme.xrel, mme.yrel, 4, 4, 255, 255, 255);
        }

        fbg_rect(fbg, fbg->width / 2, 0, 1, fbg->height, 255, 255, 255);

        fbg_write(fbg, "Fragment (framebuffer)", 4, 2);
        fbg_write(fbg, "Stream from RPI 3B", 512 - 18 * 8 - 4, 240 - 8 - 2);

        fbg_write(fbg, "FPS", 4, 12+8);
        fbg_write(fbg, "#0 (Main): ", 4, 22+8);
        fbg_write(fbg, "#1: ", 4, 32+8);
        fbg_write(fbg, "#2: ", 4, 32+8+2+8);
        fbg_write(fbg, "#3: ", 4, 32+16+4+8);
        fbg_drawFramerate(fbg, NULL, 0, 4+32+48+8, 22+8, 255, 255, 255);
        fbg_drawFramerate(fbg, NULL, 1, 4+32, 32+8, 255, 255, 255);
        fbg_drawFramerate(fbg, NULL, 2, 4+32, 32+8+2+8, 255, 255, 255);
        fbg_drawFramerate(fbg, NULL, 3, 4+32, 32+16+4+8, 255, 255, 255);

        fbg_flip(fbg);

#if defined(_WIN32) || defined(_WIN64)
	if (_kbhit()) {
            break;
	}
    } while (1);
#else
    } while (keep_running);
#endif

    lws_context_destroy(context);

    fbg_close(fbg);

    fbg_freeImage(texture);
    fbg_freeImage(bbimg);
    fbg_freeFont(bbfont);

ws_error:

    printf("Bye.\n");

    return 0;
}
