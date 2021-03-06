/*****************************************************************************/
/* COPYRIGHT (C) 2009 STMicroelectronics - All Rights Reserved               */
/* ST makes no warranty express or implied including but not limited to,     */
/* any warranty of                                                           */
/*                                                                           */
/*   (i)  merchantability or fitness for a particular purpose and/or         */
/*   (ii) requirements, for a particular purpose in relation to the LICENSED */
/*        MATERIALS, which is provided AS IS, WITH ALL FAULTS. ST does not   */
/*        represent or warrant that the LICENSED MATERIALS provided here     */
/*        under is free of infringement of any third party patents,          */
/*        copyrights, trade secrets or other intellectual property rights.   */
/*        ALL WARRANTIES, CONDITIONS OR OTHER TERMS IMPLIED BY LAW ARE       */
/*        EXCLUDED TO THE FULLEST EXTENT PERMITTED BY LAW                    */
/*                                                                           */
/*****************************************************************************/
/**
 * @file     stapp_power.h
 * @brief    This is the power management file header.
 * @author   STMicroelectronics
 */

#ifndef _STAPP_POWER_H_
#define _STAPP_POWER_H_

/* C++ support */
/* ----------- */
#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
/* --------- */
#if defined(ST_5197)
#define POWER_MAX_NUMBER  1
#endif
#if defined(ST_5206)
#define POWER_MAX_NUMBER  1
#endif
#if defined(ST_7105)
#define POWER_MAX_NUMBER  1
#endif
#if defined(ST_7108)
#define POWER_MAX_NUMBER  1
#endif
#if defined(ST_7111)
#define POWER_MAX_NUMBER  1
#endif
#if defined(ST_7141)
#define POWER_MAX_NUMBER  1
#endif
#if defined(ST_H205)
#define POWER_MAX_NUMBER  1
#endif

/* Includes */
/* -------- */
#if defined(POWER_MAX_NUMBER)
#include "stpower.h"
#endif

/* External declarations */
/* --------------------- */
#if defined(POWER_MAX_NUMBER)
extern STPOWER_Handle_t POWER_Handle[POWER_MAX_NUMBER];
#endif

/* Prototypes */
/* ---------- */
ST_ErrorCode_t POWER_Init(void);
ST_ErrorCode_t POWER_Term(void);

/* C++ support */
/* ----------- */
#ifdef __cplusplus
}
#endif
#endif

