/**
 * ----------------------------------------------------------------------
 * @Date  21-7-2015
 * @Revision  V0.01
 * @author  Peter Zhai
 * @Project  shell API
 * @Title  shell.h header file
 ******************************************************************************
*/ 
#ifndef __SHELL_H__
#define	__SHELL_H__

void DisplayTaskInf(void);
void ShellTask(void);
#define SHELL_TASK_STACK_SIZE	1024
#define SHELL_TASK_PRIO			3

#endif