/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * @file
 * @author Willem Thiart himself@willemthiart.com
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* for varags */
#include <stdarg.h>

#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"

void raft_set_election_timeout(raft_server_t* me_, int millisec)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    me->election_timeout = millisec;
    raft_randomize_election_timeout(me_);
}

void raft_set_request_timeout(raft_server_t* me_, int millisec)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    me->request_timeout = millisec;
}

raft_node_id_t raft_get_nodeid(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    if (!me->node)
        return -1;
    return raft_node_get_id(me->node);
}

int raft_get_election_timeout(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->election_timeout;
}

int raft_get_request_timeout(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->request_timeout;
}

int raft_get_num_nodes(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->num_nodes;
}

int raft_get_num_voting_nodes(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    int i, num = 0;
    for (i = 0; i < me->num_nodes; i++)
        if (raft_node_is_active(me->nodes[i]) && raft_node_is_voting(me->nodes[i]))
            num++;
    return num;
}

int raft_get_timeout_elapsed(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->timeout_elapsed;
}

raft_index_t raft_get_log_count(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    return log_count(me->log);
}

int raft_get_voted_for(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    return me->voted_for;
}

int raft_set_current_term(raft_server_t* me_, const raft_term_t term)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    if (me->current_term < term)
    {
        int voted_for = -1;
        if (me->cb.persist_term)
        {
            int e = me->cb.persist_term(me_, me->udata, term, voted_for);
            if (0 != e)
                return e;
        }
        me->current_term = term;
        me->voted_for = voted_for;
    }
    return 0;
}

raft_term_t raft_get_current_term(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->current_term;
}

raft_index_t raft_get_current_idx(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    return log_get_current_idx(me->log);
}

void raft_set_commit_idx(raft_server_t* me_, raft_index_t idx)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    assert(me->commit_idx <= idx);
    assert(idx <= raft_get_current_idx(me_));
    me->commit_idx = idx;
}

void raft_set_last_applied_idx(raft_server_t* me_, raft_index_t idx)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    me->last_applied_idx = idx;
}

raft_index_t raft_get_last_applied_idx(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->last_applied_idx;
}

raft_index_t raft_get_commit_idx(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->commit_idx;
}

void raft_set_state(raft_server_t* me_, int state)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    me->state = state;
    /* if became the leader, then update the current leader entry */
    if (state == RAFT_STATE_LEADER)
    {
        me->current_leader = me->node;
        if (me->cb.became_leader)
        {
            me->cb.became_leader(me_, me->udata);
        }
    } else if (state == RAFT_STATE_FOLLOWER)
    {
        if (me->cb.became_follower)
        {
            me->cb.became_follower(me_, me->udata);
        }
    }
}

int raft_get_state(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->state;
}

raft_node_t* raft_get_node(raft_server_t *me_, raft_node_id_t nodeid)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    int i;

    for (i = 0; i < me->num_nodes; i++)
        if (nodeid == raft_node_get_id(me->nodes[i]))
            return me->nodes[i];

    return NULL;
}

raft_node_t* raft_get_my_node(raft_server_t *me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    int i;

    for (i = 0; i < me->num_nodes; i++)
        if (raft_get_nodeid(me_) == raft_node_get_id(me->nodes[i]))
            return me->nodes[i];

    return NULL;
}

raft_node_t* raft_get_node_from_idx(raft_server_t* me_, const raft_index_t idx)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    return me->nodes[idx];
}

raft_node_id_t raft_get_current_leader(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    if (me->current_leader)
        return raft_node_get_id(me->current_leader);
    return -1;
}

raft_node_t* raft_get_current_leader_node(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    return me->current_leader;
}

void* raft_get_udata(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->udata;
}

int raft_is_follower(raft_server_t* me_)
{
    return raft_get_state(me_) == RAFT_STATE_FOLLOWER;
}

int raft_is_leader(raft_server_t* me_)
{
    return raft_get_state(me_) == RAFT_STATE_LEADER;
}

int raft_is_candidate(raft_server_t* me_)
{
    return raft_get_state(me_) == RAFT_STATE_CANDIDATE;
}

raft_term_t raft_get_last_log_term(raft_server_t* me_)
{
    raft_index_t current_idx = raft_get_current_idx(me_);
    if (0 < current_idx)
    {
        raft_entry_t* ety = raft_get_entry_from_idx(me_, current_idx);
        if (ety)
            return ety->term;
    }
    return 0;
}

int raft_is_connected(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->connected;
}

int raft_snapshot_is_in_progress(raft_server_t *me_)
{
    return ((raft_server_private_t*)me_)->snapshot_in_progress;
}

int raft_is_apply_allowed(raft_server_t* me_)
{
    return (!raft_snapshot_is_in_progress(me_) ||
            (((raft_server_private_t*)me_)->snapshot_flags & RAFT_SNAPSHOT_NONBLOCKING_APPLY));
}

raft_entry_t *raft_get_last_applied_entry(raft_server_t *me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    if (raft_get_last_applied_idx(me_) == 0)
        return NULL;
    return log_get_at_idx(me->log, raft_get_last_applied_idx(me_));
}

raft_index_t raft_get_snapshot_last_idx(raft_server_t *me_)
{
    return ((raft_server_private_t*)me_)->snapshot_last_idx;
}

raft_term_t raft_get_snapshot_last_term(raft_server_t *me_)
{
    return ((raft_server_private_t*)me_)->snapshot_last_term;
}

void raft_set_snapshot_metadata(raft_server_t *me_, raft_term_t term, raft_index_t idx)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    me->saved_snapshot_last_term = me->snapshot_last_term;
    me->saved_snapshot_last_idx = me->snapshot_last_idx;
    me->snapshot_last_term = term;
    me->snapshot_last_idx = idx;
}
