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
			{Keycode.Break, JAKeycodes.A_PAUSE},
			{Keycode.A, JAKeycodes.A_LOW_A},
			{Keycode.B, JAKeycodes.A_LOW_B},
			{Keycode.C, JAKeycodes.A_LOW_C},
			{Keycode.D, JAKeycodes.A_LOW_D},
			{Keycode.E, JAKeycodes.A_LOW_E},
			{Keycode.F, JAKeycodes.A_LOW_F},
			{Keycode.G, JAKeycodes.A_LOW_G},
			{Keycode.H, JAKeycodes.A_LOW_H},
			{Keycode.I, JAKeycodes.A_LOW_I},
			{Keycode.J, JAKeycodes.A_LOW_J},
			{Keycode.K, JAKeycodes.A_LOW_K},
			{Keycode.L, JAKeycodes.A_LOW_L},
			{Keycode.M, JAKeycodes.A_LOW_M},
			{Keycode.N, JAKeycodes.A_LOW_N},
			{Keycode.O, JAKeycodes.A_LOW_O},
			{Keycode.P, JAKeycodes.A_LOW_P},
			{Keycode.Q, JAKeycodes.A_LOW_Q},
			{Keycode.R, JAKeycodes.A_LOW_R},
			{Keycode.S, JAKeycodes.A_LOW_S},
			{Keycode.T, JAKeycodes.A_LOW_T},
			{Keycode.U, JAKeycodes.A_LOW_U},
			{Keycode.V, JAKeycodes.A_LOW_V},
			{Keycode.W, JAKeycodes.A_LOW_W},
			{Keycode.X, JAKeycodes.A_LOW_X},
			{Keycode.Y, JAKeycodes.A_LOW_Y},
			{Keycode.Z, JAKeycodes.A_LOW_Z},
			{Keycode.CapsLock, JAKeycodes.A_CAPSLOCK},
			{Keycode.Num0, JAKeycodes.A_0},
			{Keycode.Num1, JAKeycodes.A_1},
			{Keycode.Num2, JAKeycodes.A_2},
			{Keycode.Num3, JAKeycodes.A_3},
			{Keycode.Num4, JAKeycodes.A_4},
			{Keycode.Num5, JAKeycodes.A_5},
			{Keycode.Num6, JAKeycodes.A_6},
			{Keycode.Num7, JAKeycodes.A_7},
			{Keycode.Num8, JAKeycodes.A_8},
			{Keycode.Num9, JAKeycodes.A_9},
			{Keycode.Minus, JAKeycodes.A_MINUS},
			{Keycode.Plus, JAKeycodes.A_PLUS},
			{Keycode.Period, JAKeycodes.A_PERIOD},
			{Keycode.Comma, JAKeycodes.A_COMMA},
			{Keycode.Semicolon, JAKeycodes.A_SEMICOLON},
			{Keycode.Apostrophe, JAKeycodes.A_SINGLE_QUOTE},
			{Keycode.Slash, JAKeycodes.A_FORWARD_SLASH},
			{Keycode.Backslash, JAKeycodes.A_BACKSLASH},
			{Keycode.LeftBracket, JAKeycodes.A_OPEN_SQUARE},
			{Keycode.RightBracket, JAKeycodes.A_CLOSE_SQUARE}
		};

		public static int GetMobileKey(Keycode acode, int unicode) {
			JAKeycodes result;
			try {
				result = MobileKeyMap[acode];
			} catch (KeyNotFoundException exception) {
				result = JAKeycodes.A_NULL;
			}
			return result != JAKeycodes.A_NULL ? (int)result : (unicode < 128 ? Java.Lang.Character.ToLowerCase(unicode) : 0);
		}
	}
}