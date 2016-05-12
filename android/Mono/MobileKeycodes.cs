using System.Collections.Generic;

using Android.Views;

namespace android {
	public static class MobileKeycodes {
		private static Dictionary<Keycode, JAKeycodes> MobileKeyMap = new Dictionary<Keycode, JAKeycodes>() {
			{Keycode.VolumeUp, JAKeycodes.A_CAP_Q},
			{Keycode.VolumeDown, JAKeycodes.A_CAP_E},
			{Keycode.Escape, JAKeycodes.A_ESCAPE},
			{Keycode.Back, JAKeycodes.A_ESCAPE},
			{Keycode.Menu, JAKeycodes.A_CONSOLE},
			{Keycode.Tab, JAKeycodes.A_TAB},
			{Keycode.DpadCenter, JAKeycodes.A_ENTER},
			{Keycode.Enter, JAKeycodes.A_ENTER},
			{Keycode.Space, JAKeycodes.A_SPACE},
			{Keycode.Del, JAKeycodes.A_BACKSPACE},
			{Keycode.DpadUp, JAKeycodes.A_CURSOR_UP},
			{Keycode.DpadDown, JAKeycodes.A_CURSOR_DOWN},
			{Keycode.DpadLeft, JAKeycodes.A_CURSOR_LEFT},
			{Keycode.DpadRight, JAKeycodes.A_CURSOR_RIGHT},
			{Keycode.AltLeft, JAKeycodes.A_ALT},
			{Keycode.AltRight, JAKeycodes.A_ALT2},
			{Keycode.CtrlLeft, JAKeycodes.A_CTRL},
			{Keycode.CtrlRight, JAKeycodes.A_CTRL2},
			{Keycode.ShiftLeft, JAKeycodes.A_SHIFT},
			{Keycode.ShiftRight, JAKeycodes.A_SHIFT2},
			{Keycode.F1, JAKeycodes.A_F1},
			{Keycode.F2, JAKeycodes.A_F2},
			{Keycode.F3, JAKeycodes.A_F3},
			{Keycode.F4, JAKeycodes.A_F4},
			{Keycode.F5, JAKeycodes.A_F5},
			{Keycode.F6, JAKeycodes.A_F6},
			{Keycode.F7, JAKeycodes.A_F7},
			{Keycode.F8, JAKeycodes.A_F8},
			{Keycode.F9, JAKeycodes.A_F9},
			{Keycode.F10, JAKeycodes.A_F10},
			{Keycode.F11, JAKeycodes.A_F11},
			{Keycode.F12, JAKeycodes.A_F12},
			{Keycode.ForwardDel, JAKeycodes.A_BACKSPACE},
			{Keycode.Insert, JAKeycodes.A_INSERT},
			{Keycode.PageUp, JAKeycodes.A_PAGE_UP},
			{Keycode.PageDown, JAKeycodes.A_PAGE_DOWN},
			{Keycode.MoveHome, JAKeycodes.A_HOME},
			{Keycode.MoveEnd, JAKeycodes.A_END},
			{Keycode.Break, JAKeycodes.A_PAUSE}
		};

		public static int GetMobileKey(int acode, int unicode) {
			JAKeycodes result;
			try {
				result = MobileKeyMap[(Keycode)acode];
			} catch (KeyNotFoundException exception) {
				result = JAKeycodes.A_NULL;
			}
			return result != JAKeycodes.A_NULL ? (int)result : (unicode < 128 ? Java.Lang.Character.ToLowerCase(unicode) : 0);
		}
	}
}