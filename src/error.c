/**
 * @file error.c
 * @author Honbo (hehongbo918@gmail.com)
 * @brief print registers to analys fault
 * @version 1.0
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stdio.h>
#include "bsp.h"

void error_print(void)
{
    int ipsr, lr, cfsr, hfsr;

    asm volatile (
    "mrs %0, ipsr\n"
    "mov %1, lr\n"
    "ldr %2, =0xe000ed28\n"
    "ldr %2, [%2]\n"
    "ldr %3, =0xe000ed2c\n"
    "ldr %3, [%3]\n"
    :"=r"(ipsr),"=r"(lr),"=r"(cfsr),"=r"(hfsr)
    ::"memory","cc"
    );

    pr_info("Error: Unhandled exception: IPSR = 0x%08x LR = 0x%08x CFSR = 0x%08x HFSR = 0x%08x",
            ipsr, lr, cfsr, hfsr);
}