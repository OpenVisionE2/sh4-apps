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
 * @file     stapp_gui.h
 * @brief    This is the graphical user interface file header.
 * @author   STMicroelectronics
 */

#ifndef _STAPP_GUI_H_
#define _STAPP_GUI_H_

/* C++ support */
/* ----------- */
#ifdef __cplusplus
extern "C" {
#endif

/* Prototypes */
/* ---------- */
ST_ErrorCode_t GUI_Init(void);
ST_ErrorCode_t GUI_Term(void);
ST_Revision_t  STGUI_GetRevision(void);
#if defined(STGUI)
ST_ErrorCode_t STGUI_Init(void);
ST_ErrorCode_t STGUI_Term(void);
#endif

/* C++ support */
/* ----------- */
#ifdef __cplusplus
}
#endif
#endif

