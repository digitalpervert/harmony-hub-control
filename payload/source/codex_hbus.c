#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static int read_some(int fd, unsigned char *buf, size_t len) {
    ssize_t n = recv(fd, buf, len, 0);
    if (n <= 0) {
        return -1;
    }
    return (int)n;
}

static int read_exact(int fd, unsigned char *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(fd, buf + got, len - got, 0);
        if (n <= 0) {
            return -1;
        }
        got += (size_t)n;
    }
    return 0;
}

static int connect_local(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    struct timeval tv;
    if (fd < 0) {
        return -1;
    }
    tv.tv_sec = 8;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8088);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int websocket_handshake(int fd, const char *hub_id) {
    char req[512];
    unsigned char resp[2048];
    int n;
    snprintf(req, sizeof(req),
        "GET /?domain=svcs.myharmony.com&hubId=%s HTTP/1.1\r\n"
        "Host: 127.0.0.1:8088\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: MDEyMzQ1Njc4OWFiY2RlZg==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n", hub_id);
    if (send(fd, req, strlen(req), 0) < 0) {
        return -1;
    }
    n = read_some(fd, resp, sizeof(resp) - 1);
    if (n < 0) {
        return -1;
    }
    resp[n] = 0;
    if (!strstr((char *)resp, "101 Switching Protocols")) {
        fprintf(stderr, "%s\n", resp);
        return -1;
    }
    return 0;
}

static int send_ws_text(int fd, const char *payload) {
    size_t len = strlen(payload);
    unsigned char hdr[14];
    unsigned char mask[4] = {0x13, 0x37, 0x42, 0x99};
    size_t hlen = 0;
    size_t i;
    hdr[hlen++] = 0x81;
    if (len < 126) {
        hdr[hlen++] = 0x80 | (unsigned char)len;
    } else if (len <= 65535) {
        hdr[hlen++] = 0x80 | 126;
        hdr[hlen++] = (unsigned char)((len >> 8) & 0xff);
        hdr[hlen++] = (unsigned char)(len & 0xff);
    } else {
        hdr[hlen++] = 0x80 | 127;
        for (i = 0; i < 8; i++) {
            hdr[hlen++] = (unsigned char)((len >> (56 - 8 * i)) & 0xff);
        }
    }
    memcpy(hdr + hlen, mask, 4);
    hlen += 4;
    if (send(fd, hdr, hlen, 0) < 0) {
        return -1;
    }
    for (i = 0; i < len; i += 1024) {
        unsigned char out[1024];
        size_t j;
        size_t chunk = len - i > sizeof(out) ? sizeof(out) : len - i;
        for (j = 0; j < chunk; j++) {
            out[j] = (unsigned char)payload[i + j] ^ mask[(i + j) & 3];
        }
        if (send(fd, out, chunk, 0) < 0) {
            return -1;
        }
    }
    return 0;
}

static int recv_ws_text(int fd) {
    unsigned char hdr[2];
    unsigned long len;
    unsigned char *payload;
    if (read_exact(fd, hdr, 2) != 0) {
        return -1;
    }
    len = hdr[1] & 0x7f;
    if (len == 126) {
        unsigned char ext[2];
        if (read_exact(fd, ext, 2) != 0) return -1;
        len = ((unsigned long)ext[0] << 8) | ext[1];
    } else if (len == 127) {
        unsigned char ext[8];
        int i;
        if (read_exact(fd, ext, 8) != 0) return -1;
        len = 0;
        for (i = 0; i < 8; i++) {
            len = (len << 8) | ext[i];
        }
    }
    payload = (unsigned char *)malloc(len + 1);
    if (!payload) {
        return -1;
    }
    if (read_exact(fd, payload, len) != 0) {
        free(payload);
        return -1;
    }
    payload[len] = 0;
    printf("%s\n", payload);
    free(payload);
    return 0;
}

int main(int argc, char **argv) {
    const char *hub_id;
    const char *cmd;
    const char *params;
    char *payload;
    int fd;
    int rc = 1;
    if (argc < 3) {
        fprintf(stderr, "usage: %s <hub_id> <cmd> [params-json]\n", argv[0]);
        return 2;
    }
    hub_id = argv[1];
    cmd = argv[2];
    params = argc > 3 ? argv[3] : "{}";
    payload = (char *)malloc(strlen(hub_id) + strlen(cmd) + strlen(params) + 256);
    if (!payload) {
        return 1;
    }
    snprintf(payload, strlen(hub_id) + strlen(cmd) + strlen(params) + 256,
        "{\"hubId\":\"%s\",\"timeout\":30,\"hbus\":{\"id\":\"codex-%ld\",\"cmd\":\"%s\",\"params\":%s}}",
        hub_id, (long)time(NULL), cmd, params);
    fd = connect_local();
    if (fd < 0) {
        perror("connect");
        goto out;
    }
    if (websocket_handshake(fd, hub_id) != 0) {
        goto close_out;
    }
    if (send_ws_text(fd, payload) != 0) {
        perror("send");
        goto close_out;
    }
    if (recv_ws_text(fd) == 0) {
        rc = 0;
    }
close_out:
    close(fd);
out:
    free(payload);
    return rc;
}
