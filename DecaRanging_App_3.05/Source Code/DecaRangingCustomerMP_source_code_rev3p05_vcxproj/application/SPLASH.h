/*! ----------------------------------------------------------------------------
 * @file	SPLASH.h
 * @brief	interface for the SPLASH class
 *		    
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#if !defined(AFX_SPLASH_H__41182F11_BB6F_11D6_B0F5_00B0D01AD687__INCLUDED_)
#define AFX_SPLASH_H__41182F11_BB6F_11D6_B0F5_00B0D01AD687__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



class SPLASH
{
public:
    void Hide();
    void Show();
    // void Init(HWND hWnd,HINSTANCE hInst,int resid);
    void Init(HINSTANCE hInst,int resid);
    BOOL SHOWING;
    SPLASH();
    virtual ~SPLASH();

private:
    UINT TimerID;
    // HWND hParentWindow;
    HWND hSplashWnd;

};

#endif // !defined(AFX_SPLASH_H__41182F11_BB6F_11D6_B0F5_00B0D01AD687__INCLUDED_)
