
        while (!list_empty(&ptcp->squeue.list)) {
            ptcp_queue_entry *entry = squeue_begin(ptcp);
            assert(entry);
            if (entry->data)
              free(entry->data);
            list_del(&entry->list);
            free(entry);
        }
