/*
 * qian can <qiancan31863@gmail.com>
 */


#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/signal.h>
#include <asm/stacktrace.h>
#include <asm/traps.h>


static char *symbol_name = NULL;
module_param(symbol_name, charp, 0644);

static unsigned int offset = 0x00;
module_param(offset, uint, 0644);

static struct kprobe kp = {.symbol_name = "?"};

#ifdef CONFIG_ARM
static void show_backtrace(struct pt_regs *regs)
{
    struct stackframe frame;
    int urc;
    unsigned long where;
    
    frame.fp = regs->ARM_fp;
    frame.sp = regs->ARM_sp;
    frame.lr = regs->ARM_lr;
    /* use lr as frame.pc, because use pc cant dumpstack, maybe in kprobes */
    frame.pc = regs->ARM_lr;
        
    while(1)
    {
        where = frame.pc;
        urc = unwind_frame(&frame);
        if (urc < 0)
        {
            break;
        }

        dump_backtrace_entry(where, frame.pc, frame.sp-4);
    }    
}
#endif

static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    unsigned long clone_flags = 0;
    unsigned long stack_start = 0;
    struct filename *filename = NULL;

    int sig = 0;
    struct siginfo *info = NULL;
    struct task_struct *t = NULL;

    printk("----------------hander_pre-------------------\n");
    printk("comm=%s pid=%d\n", current->comm, current->pid);
#ifdef CONFIG_ARM
    printk("pc is at %pS\n", (void*)instruction_pointer(regs));
    printk("lr is at %pS\n", (void*)regs->ARM_lr);

    printk("pc : [%08lx] lr : [%08lx]\n", regs->ARM_pc, regs->ARM_lr);
    printk("sp : %08lx\n", regs->ARM_sp);
    printk("r0 : %08lx r1 : %08lx r2: %08lx\n", regs->ARM_r0, regs->ARM_r1, regs->ARM_r2 );

    if (strncmp(p->symbol_name, "_do_fork", 8) == 0)
    {
        clone_flags = (unsigned long)regs->ARM_r0;
        stack_start = (unsigned long)regs->ARM_r1;
        if (clone_flags & CLONE_UNTRACED)
        {
            printk("[_do_fork] kernel_thread \n");
        }
        else if (clone_flags & CLONE_VFORK)
        {
            printk("[_do_fork] vfork \n");
        }
        else if (clone_flags == SIGCHLD)
        {
            printk("[_do_fork] fork \n");
        }
        else
        {
            printk("[_do_fork] clone \n");
        }
    }
    else if (strncmp(p->symbol_name, "do_execveat_common", 18) == 0)
    {
        filename = (struct filename *)regs->ARM_r1;
        if (filename)
        {
            printk("[do_execveat_common] filename : %s \n", filename->name);
        }
    }
    else if (strncmp(p->symbol_name, "__send_signal", 13) == 0)
    {
        sig = (int)regs->ARM_r0;
        info = (struct siginfo *)regs->ARM_r1;
        t = (struct task_struct *)regs->ARM_r2;

        printk("[%s-%d] send signal %d to [%s-%d] \n", current->comm, current->pid, sig, t->comm, t->pid);
    }
    else
    {
        printk("symbol_name: %s\n", p->symbol_name);
    }
    
    show_backtrace(regs);
#endif
    return 0;
}


static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
    return;
}


static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
    return 0;
}


int __init qtool_init(void)
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
        printk("register_kprobe failed, return %d\n" ,ret);
        return ret;
    }

    return 0;
}


void __exit qtool_exit(void)
{
    unregister_kprobe(&kp);
}


module_init(qtool_init);
module_exit(qtool_exit);
MODULE_LICENSE("GPL");
