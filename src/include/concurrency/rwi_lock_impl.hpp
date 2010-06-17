
#ifndef __RWI_LOCK_IMPL_HPP__
#define __RWI_LOCK_IMPL_HPP__

template<class config_t>
bool rwi_lock<config_t>::lock(access_t access, void *state) {
    if(try_lock(access)) {
        return true;
    }
    else {
        enqueue_request(access, state);
        return false;
    }
}

template<class config_t>
bool rwi_lock<config_t>::upgrade_intent_to_write(void *state) {
    assert(this->state == rwis_reading_with_intent);
    if(try_upgrade_intent_to_write()) {
        return true;
    } else {
        // TODO: enqueue_request(rwi_upgrade, state);
        return false;
    }
}

// Call if you've locked for read or write, or upgraded to write,
// and are now unlocking.
template<class config_t>
void rwi_lock<config_t>::unlock() {
    switch(state) {
    case rwis_reading:
        nreaders--;
        if(nreaders == 0)
            state = rwis_unlocked;
        assert(nreaders >= 0);
        break;
    case rwis_writing:
        state = rwis_unlocked;
        break;
    case rwis_reading_with_intent:
        nreaders--;
        if(nreaders == 0)
            state = rwis_unlocked;
        assert(nreaders >= 0);
        break;
    }
        
    // See if we can satisfy any requests from the queue
    process_queue();
}

// Call if you've locked for intent before, didn't upgrade to
// write, and are now unlocking.
template<class config_t>
void rwi_lock<config_t>::unlock_intent() {
    assert(state == rwi_intent);
    if(nreaders == 0)
        state = rwis_unlocked;
    else
        state = rwis_reading;

    // See if we can satisfy any requests from the queue
    process_queue();
}

template<class config_t>
bool rwi_lock<config_t>::try_lock(access_t access) {
    switch(access) {
    case rwi_read:
        return try_lock_read();
    case rwi_write:
        return try_lock_write();
    case rwi_intent:
        return try_lock_intent();
    }
    assert(0);
}

template<class config_t>
bool rwi_lock<config_t>::try_lock_read() {
    if(queue.head() && queue.head()->op == rwi_write)
        return false;
        
    switch(state) {
    case rwis_unlocked:
        assert(nreaders == 0);
        state = rwis_reading;
        nreaders++;
        return true;
    case rwis_reading:
        nreaders++;
        return true;
    case rwis_writing:
        assert(nreaders == 0);
        return false;
    case rwis_reading_with_intent:
        nreaders++;
        return true;
    }
    assert(0);
}

template<class config_t>
bool rwi_lock<config_t>::try_lock_write() {
    if(queue.head() &&
       (queue.head()->op == rwi_write ||
        queue.head()->op == rwi_read ||
        queue.head()->op == rwi_intent))
        return false;
        
    switch(state) {
    case rwis_unlocked:
        assert(nreaders == 0);
        state = rwis_writing;
        return true;
    case rwis_reading:
        assert(nreaders >= 0);
        return false;
    case rwis_writing:
        assert(nreaders == 0);
        return false;
    case rwis_reading_with_intent:
        return false;
    }
    assert(0);
}
    
template<class config_t>
bool rwi_lock<config_t>::try_lock_intent() {
    if(queue.head() &&
       (queue.head()->op == rwi_write ||
        queue.head()->op == rwi_intent))
        return false;
        
    switch(state) {
    case rwis_unlocked:
        assert(nreaders == 0);
        state = rwis_reading_with_intent;
        return true;
    case rwis_reading:
        state = rwis_reading_with_intent;
        return true;
    case rwis_writing:
        assert(nreaders == 0);
        return false;
    case rwis_reading_with_intent:
        return false;
    }
    assert(0);
}

template<class config_t>
bool rwi_lock<config_t>::try_upgrade_intent_to_write() {
    assert(state == rwis_reading_with_intent);
    if(nreaders == 0) {
        state = rwis_writing;
        return true;
    } else
        return false;
}
    
template<class config_t>
void rwi_lock<config_t>::enqueue_request(access_t access, void *state) {
    queue.push_back(new lock_request_t(access, state));
}

template<class config_t>
void rwi_lock<config_t>::process_queue() {
    lock_request_t *req = queue.head();
    while(req) {
        if(!try_lock(req->op))
            break;
        else
            send_notify(req);
        req = queue.head()->next;
    }
}

template<class config_t>
void rwi_lock<config_t>::send_notify(lock_request_t *req) {
    // TODO: figure out notification sending
}

#endif // __RWI_LOCK_IMPL_HPP__

