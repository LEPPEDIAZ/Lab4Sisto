void init_casio_rq(struct casio_rq* casio_rq){
    casio_rq->casio_rb_root=RB_ROOT;
    INIT_LIST_HEAD(&casio_rq->casio_list_head);
    atomic_set(&casio_rq->nr_running, 0);
}
void add_casio_task_2_list(struct casio_rq* rq, struct task_struct* p){
    struct list_head* ptr = NULL;
    struct casio_task* new = NULL;
    struct casio_task* casio_task = NULL;
    //char msg
    if (rq && p){
        new = (struct casio_task*)kzalloc(sizeof(struct casio_task), GFP_KERNEL);
        if (new){
            casio_task = NULL;
            new->task = p;
            new->absolute_deadline = 0;
            list_for_each(ptr, &rq->casio_list_head){
                casio_task = list_entry(ptr, struct casio_task, casio_list_node);
                if (casio_task){
                    if (new->task->casio_id < casio_task->task->casio_id){
                        list_add(&new->casio_list_node, ptr);
                        return;
                    }
                }
            }
            list_add(&new->casio_list_node, &rq->casio_list_head);
            //logs
        } else {
            printk(KERN_ALERT “add_casio_task_2_list: kzalloc\n”);
        }
    } else {
        printk(KERN_ALERT “add_casio_task_2_list: null pointers\n”);
    }
}
void rem_casio_task_list(struct casio_rq* rq, struct task_struct* p){
    struct list_head* ptr = NULL;
    struct list_head* next = NULL;
    struct casio_task* casio_task = NULL;
    //char msg
    if (rq && p){
        list_for_each_safe(ptr, next, &rq->casio_list_head){
            casio_task = list_entry(ptr, struct casio_task, casio_list_node);
            if (casio_task){
                if (casio_task->task->casio_id == p->casio_id){
                    list_del(ptr);
                    //logs
                    kfree(casio_task);
                    return;
                }
            }
        }
    }
}
struct casio_task* find_casio_task_list(struct casio_rq* rq, struct task_struct* p){
    struct list_head* ptr = NULL;
    struct casio_task* casio_task = NULL;
    if (rq && p){
        list_for_each(ptr, &rq->casio_list_head){
            casio_task = list_entry(ptr, struct casio_task, casio_list_node);
            if (casio_task){
                if (casio_task->task->casio_id == p->casio_id){
                    return casio_task;
                }
            }
        }
    }
return NULL;
}
void insert_casio_task_rb_tree(struct casio_rq* rq, struct casio_task* p){
    struct rb_node** node = NULL;
    struct rb_node* parent = NULL;
    struct casio_task* entry = NULL;
    node = &rq->casio_rb_root.rb_node;
    while(*node != NULL){
        parent = *node;
        entry = rb_entry(parent, struct casio_task, casio_rb_node);
        if (entry){
            if (p->absolute_deadline < entry->absolute_deadline){
                node = &parent->rb_left;
            } else {
                node = &parent->rb_right;
            }
        }
    }
    rb_link_node(&p->casio_rb_node, parent, node);
    rb_insert_color(&p->casio_rb_node, &rq->casio_rb_root);
}
void remove_casio_task_rb_tree(struct casio_rq* rq, struct casio_task* p){
    rb_erase(&(p->casio_rb_node), &(rq->casio_rb_root));
    p->casio_rb_node.rb_left = p->casio_rb_node.rb_right = NULL;
}
struct casio_task* earliest_deadline_casio_task_rb_tree(struct casio_rq* rq){
    struct rb_node* node = NULL;
    struct casio_task* p = NULL;
    node = rq->casio_rb_root.rb_node;
    if (node == NULL)
        return NULL;
    while (node->rb_left != NULL){
        node = node->rb_left;
    }
    p = rb_entry(node, struct casio_task, casio_rb_node);
    return p;
}
static void enqueue_task_casio(struct rq* rq, struct task_struct* p, int wakeup)
{
    struct casio_task* t = NULL;
    //char msg
    if (p){
        t = find_casio_task_list(&rq->casio_rq, p);
        if (t){
            t->absolute_deadline = sched_clock() + p->deadline;
            insert_casio_task_rb_tree(&rq->casio_rq, t);
            atomic_inc(&rq->casio_rq.nr_running);
            //logs
        } else {
            printk(KERN_ALERT "enqueue_task_casio\n");
        }
    }
}
static void dequeue_task_casio(struct rq* rq, struct task_struct* p, int sleep)
{
    struct casio_task* t = NULL;
    //char msg
    if(p){
        t = find_casio_task_list(&rq->casio_rq,p);
        if (t){
            //logs
            remove_casio_task_rb_tree(&rq->casio_rq, t);
            atomic_dec(&rq->casio_rq.nr_running);
            if(t->task->state == TASK_DEAD || t->task->state == EXIT_DEAD
                    || t->task->state==EXIT_ZOMBIE){
                    rem_casio_task_list(&rq->casio_rq, t->task);
            }
        } else {
        printk(KERN_ALERT "dequeue_task_casio\n");
        }
    }
}
const struct sched_class casio_sched_class = {
    .next                   = &rt_sched_class,
    .enqueue_task           = enqueue_task_casio,
    .dequeue_task           = dequeue_task_casio,
    .check_preempt_curr     = check_preempt_curr_casio,
    .pick_next_task         = pick_next_task_casio,
    .put_prev_task          = put_prev_task_casio,
#ifdef CONFIG_SMP
    .load_balance           = load_balance_casio,
    .move_one_task          = move_one_task_casio,
#endif
    .set_curr_task          = set_curr_task_casio,
    .task_tick              = task_tick_casio,
};