package com.jamme;

import java.util.HashMap;
import java.util.Map;

import android.view.KeyEvent;

public final class MobileKeycodes {
	private MobileKeycodes() {
	}
	  
	private static final Map<Integer, Integer> mobileKeyMap = new HashMap<Integer, Integer>();
	  
	static {
		mapKey(KeyEvent.KEYCODE_VOLUME_UP, JAKeycodes.A_CAP_Q);
		mapKey(KeyEvent.KEYCODE_VOLUME_DOWN, JAKeycodes.A_CAP_E);
		mapKey(KeyEvent.KEYCODE_ESCAPE, JAKeycodes.A_ESCAPE);
		mapKey(KeyEvent.KEYCODE_BACK, JAKeycodes.A_ESCAPE);
		mapKey(KeyEvent.KEYCODE_MENU, JAKeycodes.A_CONSOLE);
		mapKey(KeyEvent.KEYCODE_TAB, JAKeycodes.A_TAB);
		mapKey(KeyEvent.KEYCODE_DPAD_CENTER, JAKeycodes.A_ENTER);
		mapKey(KeyEvent.KEYCODE_ENTER, JAKeycodes.A_ENTER);
		mapKey(KeyEvent.KEYCODE_SPACE, JAKeycodes.A_SPACE);
		mapKey(KeyEvent.KEYCODE_DEL, JAKeycodes.A_BACKSPACE);
		mapKey(KeyEvent.KEYCODE_DPAD_UP, JAKeycodes.A_CURSOR_UP);
		mapKey(KeyEvent.KEYCODE_DPAD_DOWN, JAKeycodes.A_CURSOR_DOWN);
		mapKey(KeyEvent.KEYCODE_DPAD_LEFT, JAKeycodes.A_CURSOR_LEFT);
		mapKey(KeyEvent.KEYCODE_DPAD_RIGHT, JAKeycodes.A_CURSOR_RIGHT);
		mapKey(KeyEvent.KEYCODE_ALT_LEFT, JAKeycodes.A_ALT);
		mapKey(KeyEvent.KEYCODE_ALT_RIGHT, JAKeycodes.A_ALT2);
		mapKey(KeyEvent.KEYCODE_CTRL_LEFT, JAKeycodes.A_CTRL);
		mapKey(KeyEvent.KEYCODE_CTRL_RIGHT, JAKeycodes.A_CTRL2);
		mapKey(KeyEvent.KEYCODE_SHIFT_LEFT, JAKeycodes.A_SHIFT);
		mapKey(KeyEvent.KEYCODE_SHIFT_RIGHT, JAKeycodes.A_SHIFT2);
		mapKey(KeyEvent.KEYCODE_F1, JAKeycodes.A_F1);
		mapKey(KeyEvent.KEYCODE_F2, JAKeycodes.A_F2);
		mapKey(KeyEvent.KEYCODE_F3, JAKeycodes.A_F3);
		mapKey(KeyEvent.KEYCODE_F4, JAKeycodes.A_F4);
		mapKey(KeyEvent.KEYCODE_F5, JAKeycodes.A_F5);
		mapKey(KeyEvent.KEYCODE_F6, JAKeycodes.A_F6);
		mapKey(KeyEvent.KEYCODE_F7, JAKeycodes.A_F7);
		mapKey(KeyEvent.KEYCODE_F8, JAKeycodes.A_F8);
		mapKey(KeyEvent.KEYCODE_F9, JAKeycodes.A_F9);
		mapKey(KeyEvent.KEYCODE_F10, JAKeycodes.A_F10);
		mapKey(KeyEvent.KEYCODE_F11, JAKeycodes.A_F11);
		mapKey(KeyEvent.KEYCODE_F12, JAKeycodes.A_F12);
		mapKey(KeyEvent.KEYCODE_FORWARD_DEL, JAKeycodes.A_BACKSPACE);
		mapKey(KeyEvent.KEYCODE_INSERT, JAKeycodes.A_INSERT);
		mapKey(KeyEvent.KEYCODE_PAGE_UP, JAKeycodes.A_PAGE_UP);
		mapKey(KeyEvent.KEYCODE_PAGE_DOWN, JAKeycodes.A_PAGE_DOWN);
		mapKey(KeyEvent.KEYCODE_MOVE_HOME, JAKeycodes.A_HOME);
		mapKey(KeyEvent.KEYCODE_MOVE_END, JAKeycodes.A_END);
		mapKey(KeyEvent.KEYCODE_BREAK, JAKeycodes.A_PAUSE);
	}
	  
	private static final void mapKey(int mobileCode, JAKeycodes jaCode) {
		mapKey(mobileCode, jaCode.ordinal());
	}
	  
	private static final void mapKey(int mobileCode, int jaCode) {
		mobileKeyMap.put(mobileCode, jaCode);
	}
	  
	public static int getMobileKey(int acode, int unicode) {
		final Integer result = mobileKeyMap.get(acode);
		return result != null ? result : (unicode < 128 ? Character.toLowerCase(unicode) : 0);
	}
}