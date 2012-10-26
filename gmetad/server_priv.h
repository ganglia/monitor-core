#ifndef SERVER_PRIV_H
#define SERVER_PRIV_H 1

struct request_context {
  char *path;
  client_t *client;
};

static int
process_path_adapter (datum_t *key, datum_t *val, void *arg);

#endif
