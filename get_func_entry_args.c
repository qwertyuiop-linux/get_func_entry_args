#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>

/* fs/exec.c */
struct user_arg_ptr
{
    union
    {
        const char __user *const __user *native;
    }ptr;
};

static char *symbol_name = NULL;
module_param(symbol_name, charp, 0644);

static unsigned int offset = 0x00;
module_param(offset, uint, 0644);

static struct kprobe kp = {.symbol_name = "?"};

/* fs/exec.c */
static const char __user *get_user_arg_ptr(struct user_arg_ptr argv, int nr)
{
    const char __user *native;

    if (get_user(native, argv.ptr.native + nr))
    {
        return ERR_PTR(-EFAULT);
    }

    return native;
}

static int count(struct user_arg_ptr argv, int max)
{
    int i = 0;
    const char __user *p;

    if (argv.ptr.native != NULL)
    {
        for(;;)
        {
            p = get_user_arg_ptr(argv, i);

            if (!p)
            {
                break;
            }

            if (IS_ERR(p))
            {
                return -EFAULT;
            }

            if (i>max)
            {
                return -E2BIG;
            }

            ++i;

            if (fatal_signal_pending(current))
            {
                return -ERESTARTNOHAND;
            }

            cond_resched();
        }
    }

    return i;
}


static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct filename *fn;
    int argc = 0;
    int envc = 0;
    struct user_arg_ptr argv;
    struct user_arg_ptr envp;
    char argv_buf[1024] = {0};
    char envp_buf[1024] = {0};
    int mode = -1;

    printk("----------------hander_pre-------------------\n");
    printk("comm=%s pid=%d", current->comm, current->pid);
#ifdef CONFIG_ARM
    printk("pc is at %pS\n", (void*)instruction_pointer(regs));
    printk("lr is at %pS\n", (void*)regs->ARM_lr);

    printk("pc : [%08lx] lr : [%08lx] psr: %08lx\n", regs->ARM_pc, regs->ARM_lr, regs->ARM_psr );
    printk("sp : %08lx ip : %08lx fp: %08lx\n", regs->ARM_sp, regs->ARM_ip, regs->ARM_fp );
    printk("r0 : %08lx r1 : %08lx r2: %08lx\n", regs->ARM_r0, regs->ARM_r1, regs->ARM_r2 );

    if (strncmp(p->symbol_name, "do_execve", strlen(p->symbol_name)))
    {
        fn = (struct filename *)regs->ARM_r0;
        if (regs->ARM_r1 < 0xc0000000) && (regs->ARM_r1 > 0x00000000)
        {
            argv.ptr.native = (const char __user *const __user *)regs->ARM_r1;
            mode = 1;
        }
        else if (regs->ARM_r1 > 0xc0000000)
        {
            mode = 0;
        }

        if (regs->ARM_r2 < 0xc0000000) && (regs->ARM_r2 > 0x00000000)
        {
            envp.ptr.native = (const char __user *const __user *)regs->ARM_r2;
        }
        else if (regs->ARM_r2 > 0xc0000000)
        {
            ;
        }

        if (mode == 1)
        {
            argc = count(argv, 0x7fffffff);
            if (argc <= 0)
            {
                return -1;
            }

            envc = count(envp, 0x7fffffff);
            if (envc <= 0)
            {
                return -1;
            }

            if (strncpy_from_user(argv_buf, (const char __user*)(*argv.ptr.native), 1024))
            {
                return -1;
            }
            
            if (strncpy_from_user(envp_buf, (const char __user*)(*envp.ptr.native), 1024))
            {
                return -1;
            }

            printk("[user-do_execve] filename %s argc (%d) env (%d) \n", fn->name, argc, envc);
            printk("[user-do_execve] argv:%s envp:%s\n");
        }
        else if (mode == 1)
        {
            printk("[kernel-do_execve] filename %s \n", fn->name);
        }
    }
#endif
    return 0;
}


static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
    return;
}


static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
    printk("fault_hander: p->addr = 0x%p, trap #%d \n", p->addr, trapnr);
    printk("comm=%s pid=%d", current->comm, current->pid);
#ifdef CONFIG_ARM
    printk("pc is at %pS\n", (void*)instruction_pointer(regs));
    printk("lr is at %pS\n", (void*)regs->ARM_lr);

    printk("pc : [%08lx] lr : [%08lx] psr: %08lx\n", regs->ARM_pc, regs->ARM_lr, regs->ARM_psr );
    printk("sp : %08lx ip : %08lx fp: %08lx\n", regs->ARM_sp, regs->ARM_ip, regs->ARM_fp );
    printk("r0 : %08lx r1 : %08lx r2: %08lx\n", regs->ARM_r0, regs->ARM_r1, regs->ARM_r2 );
#endif
    return 0;
}


static int __init get_func_entry_args_init(void)
{
    int ret = 0;
    if (!symbol_name)
    {
        printk("symbol_name null \n");
        return -1;
    }

    kp.symbol_name = symbol_name;
    kp.offset = offset;
    kp.pre_handler = handler_pre;
    kp.post_handler = handler_post;
    kp.fault_handler = handler_fault;

    ret = register_kprobe(&kp);
    if (ret < 0)
    {
        printk("register_kprobe failed, return %d\n");
        return ret;
    }

    return 0;
}


static void __exit get_func_entry_args_exit(void)
{
    unregister_kprobe(&kp);
}


module_init(get_func_entry_args_init);
module_exit(get_func_entry_args_exit);
MODULE_LICENSE("GPL");
