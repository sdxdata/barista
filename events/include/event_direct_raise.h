/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

static int FUNC_NAME(uint32_t id, uint16_t type, uint16_t len, const FUNC_TYPE *data)
{
    int ev_num = ev_ctx->ev_num[type];
    compnt_t **ev_list = ev_ctx->ev_list[type];

    if (ev_list == NULL) return -1;

    event_out_t ev_out = {0};
    event_t *ev = (event_t *)&ev_out;

    ev_out.id = id;
    ev_out.type = type;
    ev_out.length = len;

    ev->FUNC_DATA = data;

    ev_ctx->num_events[type]++;

    compnt_t *one_by_one = NULL;

    int i;
    for (i=0; i<ev_num; i++) {
        compnt_t *c = ev_list[i];

        if (!c) continue;
        else if (!c->activated) continue; // not activated yet

        if (c->role == COMPNT_SECURITY_V2) {
            one_by_one = c;
        }

#ifdef ODP_FUNC
        if (ODP_FUNC(c->odp, data)) continue;
#endif /* ODP_FUNC */

        if (c->perm & COMPNT_WRITE) {
            if (c->site == COMPNT_INTERNAL) { // internal site
                c->num_events[type]++;

                int ret = c->handler(ev, &ev_out);
                if (ret && c->perm & COMPNT_EXECUTE) {
                    break;
                }

                ev_out.checksum = 0;
            } else { // external site
                event_out_t *out = &ev_out;

                c->num_events[type]++;

                int ret = EV_SEND_EXT_MSG(id, type, len, data, out);
                if (ret && c->perm & COMPNT_EXECUTE) {
                    break;
                }
            }
        } else {
            if (c->site == COMPNT_INTERNAL) { // internal site
                c->num_events[type]++;

                int ret = c->handler(ev, NULL);
                if (ret && c->perm & COMPNT_EXECUTE) {
                    break;
                }
            } else { // external site
                event_out_t *out = &ev_out;

                c->num_events[type]++;

                if (c->perm & COMPNT_EXECUTE) {
                    int ret = EV_SEND_EXT_MSG(id, type, len, data, out);
                    if (ret) {
                        break;
                    }
                } else {
                    EV_PUSH_EXT_MSG(id, type, len, data);
                }
            }
        }

        if (one_by_one) {
            c = one_by_one;

            if (c->activated) {
                if (c->perm & COMPNT_WRITE) {
                    if (c->site == COMPNT_INTERNAL) { // internal site
                        c->num_events[type]++;

                        int ret = c->handler(ev, &ev_out);
                        if (ret && c->perm & COMPNT_EXECUTE) {
                            break;
                        }
                    } else { // external site
                        event_out_t *out = &ev_out;

                        c->num_events[type]++;

                        int ret = EV_SEND_EXT_MSG(id, type, len, data, out);
                        if (ret && c->perm & COMPNT_EXECUTE) {
                            break;
                        }
                    }
                } else {
                    if (c->site == COMPNT_INTERNAL) { // internal site
                        c->num_events[type]++;

                        int ret = c->handler(ev, NULL);
                        if (ret && c->perm & COMPNT_EXECUTE) {
                            break;
                        }
                    } else { // external site
                        event_out_t *out = &ev_out;

                        c->num_events[type]++;

                        if (c->perm & COMPNT_EXECUTE) {
                            int ret = EV_SEND_EXT_MSG(id, type, len, data, out);
                            if (ret) {
                                break;
                            }
                        } else {
                            EV_PUSH_EXT_MSG(id, type, len, data);
                        }
                    }
                }
            } else {
                one_by_one = NULL;
            }
        }
    }

    return 0;
}
