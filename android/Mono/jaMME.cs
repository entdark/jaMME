using System;
using System.Runtime.InteropServices;

using Android.App;
using Android.Content;
using Android.Graphics.Drawables;
using Android.Opengl;
using Android.OS;
using Android.Text;
using Android.Util;
using Android.Views;
using Android.Views.InputMethods;
using Android.Widget;

using Java.Nio.Channels;
using Java.IO;
using Android.Graphics;
using Javax.Microedition.Khronos.Opengles;
using Java.Lang;
using Java.Util;
using Android.Runtime;
using Javax.Microedition.Khronos.Egl;
using Android.Content.PM;
using Java.Interop;
using Android.Media;

namespace android {
	[Activity(Name = "com.jamme.jaMME",
		Label = "@string/app_name",
		MainLauncher = true,
		Icon = "@drawable/icon",
		ConfigurationChanges = ConfigChanges.Orientation | ConfigChanges.KeyboardHidden,
		Theme = "@android:style/Theme.NoTitleBar.Fullscreen",
		NoHistory = true,
		WindowSoftInputMode = SoftInput.AdjustPan,
		LaunchMode = LaunchMode.SingleTop,
		ScreenOrientation = ScreenOrientation.SensorLandscape
#if __ANDROID_11__
		, HardwareAccelerated = false
#endif
	)]
	public class jaMME : Activity {
		public static IntPtr jaMMEHandle = JNIEnv.FindClass("com/jamme/jaMME");
		public static Context Context = null;

		private Vibrator vibrator;
		private float density = 1.0f;
		private bool gameReady = false;
		private string gamePath, gameArgs = "", demoName = null;
		private Intent service;
		public static bool ServiceRunning = false, KillService = false, GameRunning = false;

		private jaMMEView view;
		private jaMMERenderer renderer;
		private jaMMEEditText et;
		private jaMMESeekBar sb = null;
		private jaMMERangeSeekBar rsb = null;

		public int surfaceWidth, surfaceHeight;

		public int flags = 0;
		static readonly int CONSOLE_ACTIVE = 1<<2;
		static readonly int CONSOLE_ACTIVE_FULLSCREEN = 1<<3;
		static readonly int UI_ACTIVE = 1<<4;
		static readonly int UI_EDITINGFIELD = 1<<5;
		static readonly int DEMO_PLAYBACK = 1<<6;
		static readonly int DEMO_PAUSED = 1<<7;
		static readonly int CHAT_SAY = 1<<8;
		static readonly int CHAT_SAY_TEAM = 1<<9;
		static readonly int INGAME = ~(CONSOLE_ACTIVE
											| CONSOLE_ACTIVE_FULLSCREEN
											| UI_ACTIVE
											| UI_EDITINGFIELD | DEMO_PLAYBACK);
		//	int DEMO_CUTTING				= 1<<17;
		//	bool demoCutting = false;
		bool keyboardActive = false;

		// Java functions called from C

		[Export]
		public static bool showTextInput(int x, int y, int w, int h) {
			return false;
			// Transfer the task to the main thread as a Runnable
			//return mSingleton.commandHandler.post(new ShowTextInputTask(x, y, w, h));
		}
		[Export]
		public static bool createGLContext(int majorVersion, int minorVersion, Java.Lang.Object attribs) {
			return true;
		}
		[Export]
		public static void deleteGLContext() {
		}
		[Export]
		public static bool setActivityTitle(Java.Lang.String title) {
			// Called from SDLMain() thread and can't directly affect the view
			return true;
			//return mSingleton.sendCommand(COMMAND_CHANGE_TITLE, title);
		}
		[Export]
		public static bool sendMessage(int command, int param) {
			return true;
			// return mSingleton.sendCommand(command, Integer.valueOf(param));
		}
		[Export]
		public static Android.Content.Context getContext() {
			return jaMME.Context;
		}
		protected static Javax.Microedition.Khronos.Egl.EGLDisplay mEGLDisplay;
		protected static Javax.Microedition.Khronos.Egl.EGLSurface mEGLSurface;
		[Export]
		public static void flipBuffers() {
			try {
				var egl = (IEGL10)Javax.Microedition.Khronos.Egl.EGLContext.EGL;
				egl.EglWaitNative(EGL10.EglCoreNativeEngine, null);

				// drawing here

				egl.EglWaitGL();
				egl.EglSwapBuffers(jaMME.mEGLDisplay, jaMME.mEGLSurface);
			} catch (Java.Lang.Exception exception) {
				Log.Verbose("SDL", "flipEGL(): " + exception);
				foreach (var s in exception.GetStackTrace()) {
					Log.Verbose("SDL", s.ToString());
				}
			}
		}
		// Audio
		protected static AudioTrack mAudioTrack;
		[Export]
		public static int audioInit(int sampleRate, bool is16Bit, bool isStereo, int desiredFrames) {
			var channelConfig = isStereo ? ChannelOut.Stereo : ChannelOut.Mono;
			var audioFormat = is16Bit ? Encoding.Pcm16bit : Encoding.Pcm8bit;
			int frameSize = (isStereo ? 2 : 1) * (is16Bit ? 2 : 1);

			Log.Verbose("SDL", "SDL audio: wanted " + (isStereo ? "stereo" : "mono") + " " + (is16Bit ? "16-bit" : "8-bit") + " " + (sampleRate / 1000f) + "kHz, " + desiredFrames + " frames buffer");

			// Let the user pick a larger buffer if they really want -- but ye
			// gods they probably shouldn't, the minimums are horrifyingly high
			// latency already
			desiredFrames = System.Math.Max(desiredFrames, (AudioTrack.GetMinBufferSize(sampleRate, channelConfig, audioFormat) + frameSize - 1) / frameSize);

			if (mAudioTrack == null) {
				mAudioTrack = new AudioTrack(Stream.Music, sampleRate,
						channelConfig, audioFormat, desiredFrames * frameSize, AudioTrackMode.Stream);

				// Instantiating AudioTrack can "succeed" without an exception and the track may still be invalid
				// Ref: https://android.googlesource.com/platform/frameworks/base/+/refs/heads/master/media/java/android/media/AudioTrack.java
				// Ref: http://developer.android.com/reference/android/media/AudioTrack.html#getState()

				if (mAudioTrack.State != AudioTrackState.Initialized) {
					Log.Error("SDL", "Failed during initialization of Audio Track");
					mAudioTrack = null;
					return -1;
				}

				mAudioTrack.Play();
			}

			Log.Verbose("SDL", "SDL audio: got " + ((mAudioTrack.ChannelCount >= 2) ? "stereo" : "mono") + " " + ((mAudioTrack.AudioFormat == Encoding.Pcm16bit) ? "16-bit" : "8-bit") + " " + (mAudioTrack.SampleRate / 1000f) + "kHz, " + desiredFrames + " frames buffer");

			return 0;
		}
		[Export]
		public static void audioWriteShortBuffer(Java.Lang.Object buffer) {
			using (var sa = Java.Lang.Object.GetObject<Android.Runtime.JavaArray<short>>(buffer.Handle, JniHandleOwnership.DoNotTransfer)) {
				if (sa.Count <= 0) {
					return;
				}
				short[] shorts = sa.ToArray<short>();
				//Log.d("SDL","audioWriteShortBuffer");
				for (int i = 0; i < sa.Count;) {
					int result = mAudioTrack.Write(shorts, i, shorts.Length - i);
					if (result > 0) {
						i += result;
					} else if (result == 0) {
						try {
							Log.Debug("SDL", "audioWriteShortBuffer sleep");
							Thread.Sleep(1);
						} catch (InterruptedException exception) {
							// Nom nom
						}
					} else {
						Log.Warn("SDL", "SDL audio: error return from write(short)");
						return;
					}
				}
			}
		}
		[Export]
		public static void audioWriteByteBuffer(Java.Lang.Object buffer) {
			using (var ba = Java.Lang.Object.GetObject<Android.Runtime.JavaArray<byte>>(buffer.Handle, JniHandleOwnership.DoNotTransfer)) {
				if (ba.Count <= 0) {
					return;
				}
				byte[] bytes = ba.ToArray<byte>();
				//Log.d("SDL","audioWriteByteBuffer");
				for (int i = 0; i < ba.Count;) {
					int result = mAudioTrack.Write(bytes, i, bytes.Length - i);
					if (result > 0) {
						i += result;
					} else if (result == 0) {
						try {
							Thread.Sleep(1);
						} catch (InterruptedException exception) {
							// Nom nom
						}
					} else {
						Log.Warn("SDL", "SDL audio: error return from write(byte)");
						return;
					}
				}
			}
		}
		[Export]
		public static void audioQuit() {
			if (mAudioTrack != null) {
				mAudioTrack.Stop();
				mAudioTrack = null;
			}
		}
		
		public static void nativeInit() {
			Java_org_libsdl_app_SDLActivity_nativeInit(
				JNIEnv.Handle,
				jaMME.jaMMEHandle,
				IntPtr.Zero
			);
		}
		public static void nativeQuit() {
			Java_org_libsdl_app_SDLActivity_nativeQuit(
				JNIEnv.Handle,
				jaMME.jaMMEHandle
			);
		}
		public static void nativePause() {
			Java_org_libsdl_app_SDLActivity_nativePause(
				JNIEnv.Handle,
				jaMME.jaMMEHandle
			);
		}
		public static void nativeResume() {
			Java_org_libsdl_app_SDLActivity_nativeResume(
				JNIEnv.Handle,
				jaMME.jaMMEHandle
			);
		}

		public static int init(Activity act, string[] args, string game_path, string lib_path, string app_path, string abi, string abi_alt) {
			using (var gp = new Java.Lang.String(game_path))
			using (var lp = new Java.Lang.String(lib_path))
			using (var ap = new Java.Lang.String(app_path))
			using (var ab = new Java.Lang.String(abi))
			using (var abal = new Java.Lang.String(abi_alt)) {
				int ret;
				var arguements = new Java.Lang.String[args.Length];
				for (int i = 0; i < args.Length; i++) {
					arguements[i] = new Java.Lang.String(args[i]);
				}
				var array = JNIEnv.NewObjectArray(args);
				JNIEnv.CopyArray(arguements, array);
				ret = Java_com_jamme_jaMME_init(
					JNIEnv.Handle,
					jaMME.jaMMEHandle,
					act.Handle,
					array,
					gp.Handle,
					lp.Handle,
					ap.Handle,
					ab.Handle,
					abal.Handle
				);
				foreach (var arguement in arguements)
					arguement.Dispose();
				return ret;
			}
		}
		public static void setScreenSize(int width, int height) {
			Java_com_jamme_jaMME_setScreenSize(
				JNIEnv.Handle,
				jaMME.jaMMEHandle,
				width,
				height
			);
		}
		public static int frame() {
			return Java_com_jamme_jaMME_frame(
				JNIEnv.Handle,
				jaMME.jaMMEHandle
			);
		}
		public static string getLoadingMsg() {
			using (var s = new Java.Lang.String("DELETEME"))
			using (var obj = Java.Lang.Object.GetObject<Java.Lang.Object>(Java_com_jamme_jaMME_getLoadingMsg(
					JNIEnv.Handle,
					jaMME.jaMMEHandle,
					s.Handle
			), JniHandleOwnership.DoNotTransfer))
				return obj.ToString();
		}
		public static void keypress(int down, int qkey, int unicode) {
			Java_com_jamme_jaMME_keypress(
				JNIEnv.Handle,
				jaMME.jaMMEHandle,
				down,
				qkey,
				unicode
			);
		}
		public static void textPaste(string paste) {
			using (var p = new Java.Lang.String(paste))
				Java_com_jamme_jaMME_textPaste(
					JNIEnv.Handle,
					jaMME.jaMMEHandle,
					p.Handle
				);
		}
		public static void doAction(int state, int action) {
			Java_com_jamme_jaMME_doAction(
				JNIEnv.Handle,
				jaMME.jaMMEHandle,
				state,
				action
			);
		}
		public static void analogPitch(int mode, float v) {
			Java_com_jamme_jaMME_analogPitch(
				JNIEnv.Handle,
				jaMME.jaMMEHandle,
				mode,
				v
			);
		}
		public static void analogYaw(int mode, float v) {
			Java_com_jamme_jaMME_analogYaw(
				JNIEnv.Handle,
				jaMME.jaMMEHandle,
				mode,
				v
			);
		}
		public static void mouseMove(float dx, float dy) {
			Java_com_jamme_jaMME_mouseMove(
				JNIEnv.Handle,
				jaMME.jaMMEHandle,
				dx,
				dy
			);
		}
		public static void quickCommand(string command) {
			using (var c = new Java.Lang.String(command))
				Java_com_jamme_jaMME_quickCommand(
					JNIEnv.Handle,
					jaMME.jaMMEHandle,
						c.Handle
				);
		}
		public static int demoGetLength() {
			return Java_com_jamme_jaMME_demoGetLength(
				JNIEnv.Handle,
				jaMME.jaMMEHandle
			);
		}
		public static int demoGetTime() {
			return Java_com_jamme_jaMME_demoGetTime(
				JNIEnv.Handle,
				jaMME.jaMMEHandle
			);
		}
		public static int vibrateFeedback() {
			return Java_com_jamme_jaMME_vibrateFeedback(
				JNIEnv.Handle,
				jaMME.jaMMEHandle
			);
		}

		[DllImport("SDL2")] public extern static void Java_org_libsdl_app_SDLActivity_nativeInit(IntPtr env, IntPtr jniClass, IntPtr obj);
		[DllImport("SDL2")] public extern static void Java_org_libsdl_app_SDLActivity_nativeQuit(IntPtr env, IntPtr jniClass);
		[DllImport("SDL2")] public extern static void Java_org_libsdl_app_SDLActivity_nativePause(IntPtr env, IntPtr jniClass);
		[DllImport("SDL2")] public extern static void Java_org_libsdl_app_SDLActivity_nativeResume(IntPtr env, IntPtr jniClass);

		[DllImport("jamme")] public extern static int Java_com_jamme_jaMME_init(IntPtr env, IntPtr jniClass, IntPtr act, IntPtr args, IntPtr game_path, IntPtr lib_path, IntPtr app_path, IntPtr abi, IntPtr abi_alt);
		[DllImport("jamme")] public extern static void Java_com_jamme_jaMME_setScreenSize(IntPtr env, IntPtr jniClass, int width, int height);
		[DllImport("jamme")] public extern static int Java_com_jamme_jaMME_frame(IntPtr env, IntPtr jniClass);
		[DllImport("jamme")] public extern static IntPtr Java_com_jamme_jaMME_getLoadingMsg(IntPtr env, IntPtr jniClass, IntPtr str);
		[DllImport("jamme")] public extern static void Java_com_jamme_jaMME_keypress(IntPtr env, IntPtr jniClass, int down, int qkey, int unicode);
		[DllImport("jamme")] public extern static void Java_com_jamme_jaMME_textPaste(IntPtr env, IntPtr jniClass, IntPtr paste);
		[DllImport("jamme")] public extern static void Java_com_jamme_jaMME_doAction(IntPtr env, IntPtr jniClass, int state, int action);
		[DllImport("jamme")] public extern static void Java_com_jamme_jaMME_analogPitch(IntPtr env, IntPtr jniClass, int mode, float v);
		[DllImport("jamme")] public extern static void Java_com_jamme_jaMME_analogYaw(IntPtr env, IntPtr jniClass, int mode, float v);
		[DllImport("jamme")] public extern static void Java_com_jamme_jaMME_mouseMove(IntPtr env, IntPtr jniClass, float dx, float dy);
		[DllImport("jamme")] public extern static void Java_com_jamme_jaMME_quickCommand(IntPtr env, IntPtr jniClass, IntPtr command);

		/* DEMO STUFF */
		[DllImport("jamme")] public extern static int Java_com_jamme_jaMME_demoGetLength(IntPtr env, IntPtr jniClass);
		[DllImport("jamme")] public extern static int Java_com_jamme_jaMME_demoGetTime(IntPtr env, IntPtr jniClass);
		/* FEEDBACK */
		[DllImport("jamme")] public extern static int Java_com_jamme_jaMME_vibrateFeedback(IntPtr env, IntPtr jniClass);
		private void vibrate() {
			long time = vibrateFeedback();
			if (time > 5)
				vibrator.Vibrate(time);
		}

		private void InitVariables() {
			jaMME.NullJavaObject(this._longPressed);
			this._longPressed = new Java.Lang.Runnable(() => {
				if (this.twoPointers) {
					jaMME.quickCommand("screenshot");
				} else if ((this.flags & DEMO_PLAYBACK) != 0) {
					jaMME.quickCommand("cut it");
				} else if (!this.textPasted && ((this.flags & CONSOLE_ACTIVE) != 0
						|| (this.flags & UI_EDITINGFIELD) != 0
						|| (this.flags & CHAT_SAY) != 0
						|| (this.flags & CHAT_SAY_TEAM) != 0)) {
					string paste = this.PasteFromClipboard();
					if (paste != null) {
						try {
							jaMME.textPaste(paste);
							this.textPasted = true;
						} catch (UnsupportedEncodingException exception) {
							exception.PrintStackTrace();
						}
					}
/*				} else if ((this.flags & DEMO_PAUSED) != 0) {
					if (this.demoCutting) {
						this.demoCutting = false;
					} else {
						this.demoCutting = true;
					}*/
				} else if (this.flags == 0) {
					jaMME.quickCommand("+scores");
				}
				this.longPress = true;
			});
			jaMME.NullJavaObject(this._loadingMessage);
			this._loadingMessage = new Java.Lang.Runnable(() => {
				this.loadingMsg = jaMME.getLoadingMsg();
				this.RunOnUiThread(() => {
					if (!jaMME.equals(this.loadingMsg, this.lmLast)) {
						this.lmLast = this.loadingMsg;
						if (this.pd != null) {
							this.pd.Dismiss();
							this.pd.Dispose();
						}
						this.pd = null;
					}
					if (this.pd == null && !jaMME.equals(this.loadingMsg, "")) {
						this.pd = ProgressDialog.Show(this, "", this.loadingMsg, true);
					}
				});
				this._handler.PostDelayed(this._loadingMessage, LOADING_MESSAGE_UPDATE_TIME);
			});
			jaMME.NullJavaObject(this.renderer);
			this.renderer = new jaMMERenderer(this);
		}

		protected override void OnCreate(Bundle savedInstanceState) {
			base.OnCreate(savedInstanceState);

			jaMME.NullJavaObject(jaMME.Context);
			jaMME.Context = this.ApplicationContext;

			JavaSystem.LoadLibrary("SDL2");
			JavaSystem.LoadLibrary("jamme");

			this.InitVariables();
			this.vibrator = (Vibrator)this.GetSystemService(Context.VibratorService);
			this.density = this.Resources.DisplayMetrics.Density;
			// fullscreen
			this.RequestWindowFeature(WindowFeatures.NoTitle);
			this.Window.SetFlags(WindowManagerFlags.Fullscreen, WindowManagerFlags.Fullscreen);
			// keep screen on 
			this.Window.SetFlags(WindowManagerFlags.KeepScreenOn, WindowManagerFlags.KeepScreenOn);
			jaMME.NullJavaObject(this.view);
			this.view = new jaMMEView(this);
			this.view.SetEGLConfigChooser(new BestEglChooser(this.ApplicationContext));
			this.view.SetRenderer(this.renderer);
			this.view.KeepScreenOn = true;
			this.SetContentView(view);
			this.view.RequestFocus();
			this.view.FocusableInTouchMode = true;
			string[] parameters = { "", "", "" };
			try {
				parameters = loadParameters();
			} catch (IOException exception) {
				exception.PrintStackTrace();
			}
			string baseDirectory = "";
			if (parameters.Length > 0)
				baseDirectory = parameters[0];
			string argsStr = "";
			if (parameters.Length > 1)
				argsStr = parameters[1];
			bool logSave = false;
			if (parameters.Length > 2)
				logSave = bool.Parse(parameters[2]);
			try {
				if (checkAssets(baseDirectory) && openIntent()) {
					gameReady = true;
				}
			} catch (IOException exception) {
				exception.PrintStackTrace();
			}
			if (!gameReady) {
				addLauncherItems(this, baseDirectory, argsStr, logSave);
			}
			this.SetTheme(Android.Resource.Style.ThemeHolo);
		}
		protected override void OnPause() {
			Log.Info("jaMME", "OnPause");
			base.OnPause();
//			view.OnPause();
			if (gameReady) {
				this.service = new Intent(this, typeof(jaMMEService));
				this.StartService(this.service);
				jaMME.nativePause();
			}
		}
		protected override void OnResume() {
			Log.Info("jaMME", "OnResume");
			base.OnResume();
			this.stopService();
			view.OnResume();
			if (gameReady) {
				jaMME.nativeResume();
			}
		}
		protected override void OnRestart() {
			Log.Info("jaMME", "OnRestart");
			base.OnRestart();
			this.stopService();
		}
		protected override void OnStop() {
			Log.Info("jaMME", "OnStop");
			base.OnStop();
		}
		protected override void OnDestroy() {
			Log.Info("jaMME", "OnDestroy");
			base.OnDestroy();
			jaMME.NullJavaObject(jaMME.Context);
			this.stopService();
			jaMME.GameRunning = false;
			if (!gameReady) {
				return;
			}
			//stop demo to be able to remove currently opened demo file
			keypress(1, (int)JAKeycodes.A_ESCAPE, 0);
			keypress(0, (int)JAKeycodes.A_ESCAPE, 0);
			do {
				this.flags = jaMME.frame();
			} while ((flags & DEMO_PLAYBACK) != 0);

			//if user closed the application from OS side on the demo playback after opening them externally
			if (gamePath != null && demoName != null) {
				int i = 0;
				while (demoExtList[i] != null) {
					string demosFolder = gamePath + "/base/demos/";
					if (demoName.Contains(".mme"))
						demosFolder = gamePath + "/base/mmedemos/";
					using (var demo = new File(demosFolder + demoName + demoExtList[i])) {
						Log.Info("jaMME", "OnDestroy: removing " + demo.ToString());
						if (demo.Exists()) {
							Log.Info("jaMME", "OnDestroy: removed");
							demo.Delete();
						}
						i++;
					}
				}
			}

			jaMME.nativeQuit();
		}
		private void stopService() {
			if (this.service != null) {
				if (!isServiceRunning(typeof(jaMMEService))) {
					jaMME.KillService = false;
					jaMME.ServiceRunning = false;
				} else {
					jaMME.KillService = true;
//					bool result = this.StopService(this.service);
				}
				this.service.Dispose();
				this.service = null;
			} else {
				if (isServiceRunning(typeof(jaMMEService))) {
					jaMME.KillService = true;
				}
			}
		}
		//gay way, but fastest
		private static bool equals(string s1, string s2) {
			return s1.Length == s2.Length && s1.GetHashCode() == s2.GetHashCode();
		}
		private bool openDemo(string demo) {
			int i = 0;
			while (demoExtList[i] != null) {
				if (demo.Contains(demoExtList[i]))
					break;
				i++;
			}
			if (demoExtList[i] != null) {
				string demosFolder = gamePath + "/base/demos/";
				if (demo.Contains(".mme"))
					demosFolder = gamePath + "/base/mmedemos/";
				var from = new File(demo);
				var to = new File(demosFolder + "_" + demoExtList[i]);
				while (to.Exists()) {
					string name = string.Copy(to.Name);
					to.Dispose();
					to = new File(demosFolder + "_" + name);
				}

				copyFile(from, to);
				demoName = to.Name;
				int pos = demoName.LastIndexOf(".");
				if (pos > 0) {
					demoName = demoName.Substring(0, pos);
				}
				gameArgs = "+demo \"" + demoName + "\" del";
				to.Dispose();
				from.Dispose();
				return true;
			}
			return false;
		}
		private bool openURI(string uri) {
			if (uri.Contains(".") || uri.Contains("localhost")) {
				gameArgs = uri;
				return true;
			}
			return false;
		}
		private bool openIntent() {
			Log.Info("jaMME", "openDemo");
			var intent = this.Intent;
			if (intent == null) {
				return false;
			}
			string action = intent.Action;
			var uri = intent.Data;
			if (action != null && uri != null && action.CompareTo(Intent.ActionView) == 0) {
				string data = Java.Net.URLDecoder.Decode((uri.EncodedPath), "UTF-8");
				return openDemo(data) || openURI(data);
			}
			return false;
		}
		private static void copyFile(File sourceFile, File destFile) {
			if (!destFile.Exists()) {
				destFile.CreateNewFile();
			}
			FileChannel source = null;
			FileChannel destination = null;
			try {
				source = new FileInputStream(sourceFile).Channel;
				destination = new FileOutputStream(destFile).Channel;
				destination.TransferFrom(source, 0, source.Size());
			} finally {
				if (source != null) {
					source.Close();
					source.Dispose();
				}
				if (destination != null) {
					destination.Close();
					destination.Dispose();
				}
			}
		}
		RelativeLayout layout;
		EditText args, baseDirET;
		Button startGame, baseDir;
		CheckBox logSave;
		private void addLauncherItems(Context context, string baseDirectory, string argsStr, bool saveLog) {
			float d = density;
/* LAYOUT */
			Drawable bg = Resources.GetDrawable(Resource.Drawable.jamme);
			bg.SetAlpha(0x17);
			jaMME.NullJavaObject(layout);
			layout = new RelativeLayout(this);
			layout.SetBackgroundColor(Color.Argb(0xFF, 0x13, 0x13, 0x13));
			layout.Background = bg;
/* ARGS */
			jaMME.NullJavaObject(args);
			args = new EditText(context);
			args.SetMaxLines(1);
			args.InputType = (InputTypes.ClassText | InputTypes.TextVariationShortMessage);
			args.Hint = "+startup args";
			args.SetHintTextColor(Color.Gray);
			args.Text = argsStr;
			args.SetTextColor(Color.Argb(0xFF, 0xC3, 0x00, 0x00));
			args.Background = Resources.GetDrawable(Resource.Drawable.jamme_edit_text);
			args.ClearFocus();
			args.ImeOptions = (ImeAction)(ImeFlags.NoExtractUi | ImeFlags.NoFullscreen);
			args.Id = 5;
			RelativeLayout.LayoutParams argsParams =
					new RelativeLayout.LayoutParams(
						RelativeLayout.LayoutParams.MatchParent,
						RelativeLayout.LayoutParams.WrapContent);
			argsParams.AddRule(LayoutRules.CenterHorizontal);
			argsParams.AddRule(LayoutRules.CenterVertical);
/* START */
			jaMME.NullJavaObject(startGame);
			startGame = new Button(context);
			startGame.Background = Resources.GetDrawable(Resource.Drawable.jamme_btn);
			startGame.Click += (object sender, EventArgs e) => {
				string gp = "";
				if (baseDirET != null && baseDirET.Text != null && baseDirET.Text.ToString() != null)
					gp = baseDirET.Text.ToString();
				if (checkAssets(gp)) {
					if (args != null && args.Text != null && args.Text.ToString() != null)
						gameArgs = args.Text.ToString();
					logging = logSave.Checked;
					string[] parameters = { gp, gameArgs, logging.ToString() };
					try {
						saveParameters(parameters);
					} catch (IOException exception) {
						// TODO Auto-generated catch block
						exception.PrintStackTrace();
					}
					removeView(startGame);
					removeView(args);
					removeView(baseDirET);
					removeView(baseDir);
					removeView(logSave);
					removeView(layout);
					gameReady = true;
				} else {
					using (var alertDialogBuilder = new AlertDialog.Builder(context)) {
						alertDialogBuilder.SetTitle("Missing assets[0-3].pk3");
						alertDialogBuilder
							.SetMessage("Select base folder where assets[0-3].pk3 are placed in")
							.SetNeutralButton("Ok", (se, ev) => { });
						using (var alertDialog = alertDialogBuilder.Create()) {
							alertDialog.Show();
						}
					}
				}
			};
			startGame.Text = "Start";
			RelativeLayout.LayoutParams startParams =
					new RelativeLayout.LayoutParams(
						ViewGroup.LayoutParams.WrapContent,
						RelativeLayout.LayoutParams.WrapContent);
			startParams.AddRule(LayoutRules.Below, args.Id);
			startParams.AddRule(LayoutRules.CenterHorizontal);
			startParams.SetMargins(0, (int)(3.5f* d), 0, 0);
/* BASE DIRECTORY EDITABLE */
			jaMME.NullJavaObject(baseDirET);
			baseDirET = new EditText(context);
			baseDirET.SetMaxLines(1);
			baseDirET.InputType = (InputTypes.ClassText | InputTypes.TextVariationShortMessage);
			baseDirET.Hint = "game directory";
			baseDirET.SetHintTextColor(Color.Gray);
			baseDirET.Text = baseDirectory;
			baseDirET.SetTextColor(Color.Argb(0xFF, 0xC3, 0x00, 0x00));
			baseDirET.Background = Resources.GetDrawable(Resource.Drawable.jamme_edit_text);
			baseDirET.ClearFocus();
			baseDirET.ImeOptions = (ImeAction)(ImeFlags.NoExtractUi | ImeFlags.NoFullscreen);
			baseDirET.Id = 17;
			RelativeLayout.LayoutParams baseDirETParams =
					new RelativeLayout.LayoutParams(
						ViewGroup.LayoutParams.MatchParent,
						RelativeLayout.LayoutParams.WrapContent);
			baseDirETParams.AddRule(LayoutRules.Above, args.Id);
			baseDirETParams.AddRule(LayoutRules.CenterVertical);
			baseDirETParams.SetMargins(0, 0, 0, (int)(3.5f* d));
/* BASE DIRECTORY */
			jaMME.NullJavaObject(baseDir);
			baseDir = new Button(context);
			baseDir.Background = Resources.GetDrawable(Resource.Drawable.jamme_btn);
			baseDir.Click += (object sender, EventArgs e) => {
				DirectoryChooserDialog directoryChooserDialog =
					new DirectoryChooserDialog(context, new jaMMEChosenDirectoryListener(baseDirET));
				string chooseDir = "";
				if (baseDirET != null && baseDirET.Text != null && baseDirET.Text.ToString() != null)
					chooseDir = baseDirET.Text;
				directoryChooserDialog.chooseDirectory(chooseDir);
			};
			baseDir.Text = "Game Directory";
			RelativeLayout.LayoutParams baseDirParams =
						new RelativeLayout.LayoutParams(
							RelativeLayout.LayoutParams.WrapContent,
							RelativeLayout.LayoutParams.WrapContent);
			baseDirParams.AddRule(LayoutRules.Above, baseDirET.Id);
			baseDirParams.AddRule(LayoutRules.CenterHorizontal);
			baseDirParams.SetMargins(0, 0, 0, (int)(3.5f* d));
/* LOGGING */
			jaMME.NullJavaObject(logSave);
			logSave = new CheckBox(context);
			logSave.Checked = saveLog;
			logSave.SetButtonDrawable(Resources.GetDrawable(Resource.Drawable.jamme_checkbox));
			logSave.CheckedChange += (sender, ev) => {
				logging = ev.IsChecked;
			};
			logSave.Text = "Save Log";
			logSave.SetTextColor(Color.Argb(0xFF, 0xC3, 0x00, 0x00));
			RelativeLayout.LayoutParams logSaveParams =
						new RelativeLayout.LayoutParams(
							RelativeLayout.LayoutParams.WrapContent,
							RelativeLayout.LayoutParams.WrapContent);
			logSaveParams.AddRule(LayoutRules.AlignParentLeft);
			logSaveParams.AddRule(LayoutRules.AlignParentBottom);
/* ADD TO LAYOUT */
			layout.AddView(args, argsParams);
			layout.AddView(startGame, startParams);
			layout.AddView(baseDirET, baseDirETParams);
			layout.AddView(baseDir, baseDirParams);
			layout.AddView(logSave, logSaveParams);
			AddContentView(layout, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MatchParent, ViewGroup.LayoutParams.MatchParent));
		}
		private class jaMMEChosenDirectoryListener : DirectoryChooserDialog.ChosenDirectoryListener {
			private EditText et;

			private jaMMEChosenDirectoryListener() {}
			public jaMMEChosenDirectoryListener(EditText et) {
				this.et = et;
			}
			public void onChosenDir(string chosenDir) {
				if (chosenDir != null)
					this.et.Text = chosenDir;
				else
					this.et.Text = "";
			}
		}

		private static string getRemovableStorage() {
			string value = Java.Lang.JavaSystem.Getenv("SECONDARY_STORAGE");
			if (!TextUtils.IsEmpty(value)) {
				string[] paths = value.Split(new char[] {':'});
				foreach (string path in paths) {
					using (var file = new File(path)) {
						if (file.IsDirectory) {
							return file.ToString();
						}
					}
				}
			}
			return null; // Most likely, a removable micro sdcard doesn't exist
		}
		private bool checkAssets(string path) {
			if (path == null)
				return false;
			var dirs = new System.Collections.Generic.List<string>();
			dirs.Add(path);
			if (path.EndsWith("/base")) {
				int l = path.LastIndexOf("/base");
				string baseRoot = path.Substring(0, l);
				dirs.Add(baseRoot);
			}
			foreach (var d in dirs) {
				using (var dir = new File(d + "/base")) {
					if (dir.Exists() && dir.IsDirectory) {
						for (int i = 0; i < 3; i++) {
							using (var assets = new File(d + "/base/assets" + i + ".pk3")) {
								if (!assets.Exists()) {
									return false;
								}
							}
						}
						gamePath = d;
						return true;
					}
				}
			}
			return false;
		}
		private string[] loadParameters() {
			string [] ret = { "",""};
			using (var f = new File(this.FilesDir.ToString() + "/parameters")) {
				if (f.Exists()) {
					long length = f.Length();
					byte[] buffer = new byte[length];
					var input = new FileInputStream(f);
					try {
						input.Read(buffer);
					} finally {
						input.Close();
						input.Dispose();
					}
					string s = System.Text.Encoding.Default.GetString(buffer);
					ret = s.Split(new string[] { "\n\n\n\n\n" }, StringSplitOptions.None);
				}
				return ret;
			}
		}
		private void saveParameters(string[] parameters) {
			using (var f = new File(this.FilesDir.ToString() + "/parameters"))
			using (var stream = new FileOutputStream(f)) {
				try {
					foreach (string p in parameters) {
						string s = p + "\n\n\n\n\n";
						stream.Write(System.Text.Encoding.ASCII.GetBytes(s));
					}
				} finally {
					stream.Close();
				}
			}
		}
		private bool ignoreAfter = false;
		private int prevLength = 0;
		private class jaMMEEditText : EditText {
			private jaMME jamme;
			public jaMMEEditText(jaMME context) : base(context) {
				this.jamme = context;
				this.SetY(jamme.surfaceHeight + 32);
				this.SetX(jamme.surfaceWidth + 32);
				this.jamme.ignoreAfter = false;
				this.jamme.prevLength = 0;
				this.Alpha = 0.0f;
				this.AddTextChangedListener(new jaMMETextWatcher(context));
				this.ClearFocus();
				this.ImeOptions = (ImeAction)(ImeFlags.NoExtractUi | ImeFlags.NoFullscreen);
				this.LayoutParameters = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MatchParent, ViewGroup.LayoutParams.WrapContent);
				this.EditorAction += (object sender, EditorActionEventArgs ev) => {
					if (ev == null) {
						ev.Handled = false;
						return;
					}

					int uc = ev.Event.GetUnicodeChar(ev.Event.MetaState);

					var keyAction = ev.Event.Action;

					var keyCode = ev.Event.KeyCode;
					if (keyAction == KeyEventActions.Down) {
						if (keyCode == Keycode.Back) {
							(sender as jaMMEEditText).ClearFocus();
							if ((this.jamme.flags & CONSOLE_ACTIVE) != 0 || (this.jamme.flags & CONSOLE_ACTIVE_FULLSCREEN) != 0) {
								jaMME.quickCommand("toggleconsole");
								ev.Handled = true;
								return;
							} else if ((this.jamme.flags & CHAT_SAY) != 0
									|| (this.jamme.flags & UI_EDITINGFIELD) != 0
									|| (this.jamme.flags & CHAT_SAY_TEAM) != 0) {
								jaMME.keypress(1, (int)JAKeycodes.A_ESCAPE, 0);
								jaMME.keypress(0, (int)JAKeycodes.A_ESCAPE, 0);
								ev.Handled = true;
								return;
							}
						}
						jaMME.keypress(1, MobileKeycodes.GetMobileKey((int)keyCode, uc), uc);
						jaMME.keypress(0, MobileKeycodes.GetMobileKey((int)keyCode, uc), uc);
						ev.Handled = true;
						return;
					}
					ev.Handled = false;
					return;
				};
			}
			public override bool OnKeyPreIme(Keycode keyCode, KeyEvent ev) {
				if (ev.Action == KeyEventActions.Down) {
					if (ev.KeyCode == Keycode.Back) {
						if ((this.jamme.flags & CONSOLE_ACTIVE) != 0) {
							jaMME.quickCommand("toggleconsole");
						} else if ((this.jamme.flags & CHAT_SAY) != 0
								|| (this.jamme.flags & UI_EDITINGFIELD) != 0
								|| (this.jamme.flags & CHAT_SAY_TEAM) != 0) {
							jaMME.keypress(1, (int)JAKeycodes.A_ESCAPE, 0);
							jaMME.keypress(0, (int)JAKeycodes.A_ESCAPE, 0);
						}
					}
				}
				return base.DispatchKeyEvent(ev);
			}
		}
		private class jaMMESeekBar : SeekBar {
			string LOG = "jaMME SeekBar";
			
			public jaMMESeekBar(jaMME context) : base(context) {
				float d = context.density;
				this.Max = jaMME.demoGetLength();
				this.Progress = jaMME.demoGetTime();
				this.Left = 10;
				this.SetY(context.surfaceHeight - 42*d);
				this.SetPadding((int)(50*d), 0, (int)(50*d), 0);
				this.ThumbOffset = (int)(50*d);
				this.ProgressDrawable = Resources.GetDrawable(Resource.Drawable.jamme_progress);
				this.SetThumb(Resources.GetDrawable(Resource.Drawable.jamme_control));
				ViewGroup.LayoutParams lp = new ViewGroup.LayoutParams
						(ViewGroup.LayoutParams.MatchParent,
						ViewGroup.LayoutParams.WrapContent);
				this.LayoutParameters = lp;
				this.ProgressChanged += (object sender, ProgressChangedEventArgs ev) => {
					Log.Debug(LOG, "onProgressChanged");
					if (ev.FromUser)
						jaMME.quickCommand("seek "+(ev.Progress/1000.0));
				};
			}
		}
		private Integer lastMin = new Integer(0), lastMax = new Integer(0);
		private class jaMMERangeSeekBar : RangeSeekBar<Integer> {
			string LOG = "jaMME RangeSeekBar";
			private jaMME jamme;
			public short lastChange = 0;
			public jaMMERangeSeekBar(Integer min, Integer max, jaMME context) : base(min, max, (int)(50*context.density), context) {
				this.jamme = context;
				float d = this.jamme.density;
//				setMax(demoGetLength());
				this.jamme.lastMin = new Integer(0);
				this.jamme.lastMax = this.getAbsoluteMaxValue();
//				setProgress(demoGetTime());
				this.setNotifyWhileDragging(true);
				this.Left = 10;
				this.SetY(this.jamme.surfaceHeight - 87*d);
				this.SetPadding((int)(50*d), 0, (int)(50*d), 0);
//				this.setThumbOffset((int)(50*d));
//				setProgressDrawable(getResources().getDrawable(R.drawable.jamme_progress));
//				setThumb(getResources().getDrawable(R.drawable.jamme_control));
				ViewGroup.LayoutParams lp = new ViewGroup.LayoutParams
						(ViewGroup.LayoutParams.MatchParent,
						ViewGroup.LayoutParams.WrapContent);
				this.LayoutParameters = lp;
				this.setOnRangeSeekBarChangeListener(new OnRangeSeekBarChangeListener(this.jamme));
			}
			private class OnRangeSeekBarChangeListener : IOnRangeSeekBarChangeListener<Integer> {
				string LOG = "jaMME RangeSeekBar";

				private jaMME jamme;

				private OnRangeSeekBarChangeListener() {}
				public OnRangeSeekBarChangeListener(jaMME context) {
					this.jamme = context;
				}

				public void onRangeSeekBarValuesChanged(RangeSeekBar<Integer> bar, Integer minValue, Integer maxValue) {
					Log.Debug(LOG, "onRangeSeekBarValuesChanged");
					if ((int)this.jamme.lastMin != (int)minValue) {
						jaMME.quickCommand(string.Format("seek {0:D}.{1:D3}", ((int)minValue/1000), ((int)minValue%1000)));
						this.jamme.lastMin = minValue;
						(bar as jaMMERangeSeekBar).lastChange = -1;
					} else if ((int)this.jamme.lastMax != (int)maxValue) {
						jaMME.quickCommand(string.Format("seek {0:D}.{1:D3}", ((int)maxValue/1000), ((int)maxValue%1000)));
						this.jamme.lastMax = maxValue;
						(bar as jaMMERangeSeekBar).lastChange = 1;
					}
				}
			}
			public override bool OnTouchEvent(MotionEvent ev) {
				var action = ev.Action;
				var actionCode = action & MotionEventActions.Mask;
				if (actionCode == MotionEventActions.Up) {
					if (lastChange < 0) {
						quickCommand(string.Format("cut start {0:D}.{1:D3}", ((int)this.jamme.lastMin/1000), ((int)this.jamme.lastMin%1000)));
					} else if (lastChange > 0) {
						quickCommand(string.Format("cut end {0:D}.{1:D3}", ((int)this.jamme.lastMax/1000), ((int)this.jamme.lastMax%1000)));
					}
					lastChange = 0;
//					return true;
				}
				return base.OnTouchEvent(ev);
			}
		}
		private class jaMMEView : GLSurfaceView {
			static readonly string LOG = "jaMME View";

			public jaMMEView(Context context) : base(context) {
				this.FocusableInTouchMode = true;
			}
			public override bool OnCheckIsTextEditor() {
				Log.Debug(LOG, "onCheckIsTextEditor");
				return true;
			}
			public override IInputConnection OnCreateInputConnection(EditorInfo outAttrs) {
				Log.Debug(LOG, "onCreateInputConnection");
				using (var fic = new BaseInputConnection(this, true)) {
					outAttrs.ActionLabel = null;
					outAttrs.InputType = InputTypes.ClassText;
					outAttrs.ImeOptions = (ImeFlags.NoExtractUi | ImeFlags.NoFullscreen);
					return fic;
				}
			}
		}
		private class jaMMETextWatcher : Java.Lang.Object, ITextWatcher {

			static readonly string LOG = "jaMME TextWatcher";

			private jaMME jamme;
			
			private jaMMETextWatcher() { }
			public jaMMETextWatcher(jaMME context) {
				this.jamme = context;
			}

			public void BeforeTextChanged(ICharSequence s, int start, int count, int after) {
				Log.Debug(LOG, "beforeTextChanged");
			}
			public void OnTextChanged(ICharSequence s, int start, int before, int count) {
				Log.Debug(LOG, "onTextChanged");
			}
			public void AfterTextChanged(IEditable s) {
				Log.Debug(LOG, "afterTextChanged");
				string ss = s.ToString();
				if (this.jamme.ignoreAfter) {
					this.jamme.ignoreAfter = false;
					return;
				}
				if (ss == null)
					return;
				int l = ss.Length;
				if (l < this.jamme.prevLength && l >= 0) {
//					int d = prevLength - l;
//					for (int i = d; i > 0; i--) {
					keypress(1, (int)JAKeycodes.A_BACKSPACE, 0);
					keypress(0, (int)JAKeycodes.A_BACKSPACE, 0);
//					}
					this.jamme.prevLength = 0;
					s.Replace(0, s.ToString().Length, "");
				} else if (l > 0) {
					int d = l - this.jamme.prevLength;
					for (int i = d; i > 0; i--) {
						char ch = ss.ToCharArray()[l-i];
						keypress(1, MobileKeycodes.GetMobileKey(0, ch), ch);
						keypress(0, MobileKeycodes.GetMobileKey(0, ch), ch);
					}
					this.jamme.prevLength = l;
				}
			}
		}
		private Handler _handler = new Handler();
		ProgressDialog pd = null;
		private string loadingMsg = "Loading...", lmLast = "Loading...";
		private static int LOADING_MESSAGE_TIME = 2000;
		private static int LOADING_MESSAGE_UPDATE_TIME = 1337;
		private Java.Lang.Runnable _loadingMessage;
		private bool logging = false;
		private void saveLog() {
			if (!logging)
				return;
			try {
				string[] command = new string[] { "logcat", "-v", "threadtime", "-f", gamePath+"/jamme.log" };
				var process = Java.Lang.Runtime.GetRuntime().Exec(command);
			} catch (IOException exception) {
				Log.Error("jaMME saveLog", "getCurrentProcessLog failed", exception);
			}
		}
		private void removeView(View view) {
			if (view != null) {
				ViewGroup vg = (ViewGroup)view.Parent;
				if (vg != null) {
					vg.RemoveView(view);
				}
				view.Dispose();
				view = null;
			}
		}
		private class jaMMERenderer : Java.Lang.Object, GLSurfaceView.IRenderer  {
			static readonly string LOG = "jaMME Renderer";

			private jaMME jamme;

			private jaMMERenderer() {}
			public jaMMERenderer(jaMME context) {
				this.jamme = context;
			}

			public void OnSurfaceCreated(IGL10 gl, Javax.Microedition.Khronos.Egl.EGLConfig config) {
				Log.Debug(LOG, "OnSurfaceCreated");
			}
			private void initRenderer(int width, int height) {
				this.jamme.saveLog();
				Log.Info(LOG, "screen size: " + width + "x"+ height);
				jaMME.setScreenSize(width, height);
				Log.Info(LOG, "Init start");
				//TODO: filter out fs_basepath from gameArgs 
				string[] args_array = this.jamme.createArgs("+set fs_basepath \""+this.jamme.gamePath+"\" "+this.jamme.gameArgs);
				//entTODO: need to test on different devices
				string abi = Build.CpuAbi, abi_alt = Build.CpuAbi2;
				jaMME.init(this.jamme, args_array, this.jamme.gamePath, this.jamme.ApplicationInfo.NativeLibraryDir, this.jamme.FilesDir.ToString(), abi, abi_alt);
				Log.Info(LOG, "Init done");
			}
			public void OnSurfaceChanged(IGL10 gl, int width, int height) {
				//surface changed called on surface created so.....
				Log.Debug(LOG, string.Format("OnSurfaceChanged {0}x{1}", width, height));
				this.jamme.surfaceWidth = width;
				this.jamme.surfaceHeight = height;
			}
			int notifiedflags = 0;
			bool inited = false;
			
			public void OnDrawFrame(IGL10 gl) {
				if (!this.jamme.gameReady)
					return;
				if (jaMME.ServiceRunning)
					return;
				this.jamme._handler.PostDelayed(this.jamme._loadingMessage, LOADING_MESSAGE_TIME);
				if (!inited) {
					jaMME.nativeInit();
					inited = true;
					initRenderer(this.jamme.surfaceWidth, this.jamme.surfaceHeight);
				}
				jaMME.GameRunning = false;
				this.jamme.flags = frame();
				jaMME.GameRunning = true;
				this.jamme._handler.RemoveCallbacks(this.jamme._loadingMessage);
				this.jamme.RunOnUiThread(() => {
					if (this.jamme.pd != null) {
						this.jamme.pd.Dismiss();
						this.jamme.pd.Dispose();
					}
					this.jamme.pd = null;
				});
//				flags |= (demoCutting) ? (DEMO_CUTTING) : (0);
				if (this.jamme.flags != notifiedflags) {
					if (((this.jamme.flags ^ notifiedflags) & CONSOLE_ACTIVE) != 0
							|| ((this.jamme.flags ^ notifiedflags) & UI_EDITINGFIELD) != 0
							|| ((this.jamme.flags ^ notifiedflags) & CHAT_SAY) != 0
							|| ((this.jamme.flags ^ notifiedflags) & CHAT_SAY_TEAM) != 0) {
						Log.Debug(LOG, "keyboard");
						int fl = this.jamme.flags;
						this.jamme.RunOnUiThread(() => {
							InputMethodManager im = (InputMethodManager)this.jamme.GetSystemService(Context.InputMethodService);
							if (im != null) {
								if ((!this.jamme.keyboardActive && (fl & CONSOLE_ACTIVE) != 0
										|| (fl & UI_EDITINGFIELD) != 0
										|| (fl & CHAT_SAY) != 0
										|| (fl & CHAT_SAY_TEAM) != 0)) {
									this.jamme.removeView(this.jamme.et);
									this.jamme.et = new jaMMEEditText(this.jamme);
									this.jamme.AddContentView(this.jamme.et, this.jamme.et.LayoutParameters);
									this.jamme.et.RequestFocus();
									this.jamme.et.RequestFocusFromTouch();
									//getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
									im.ShowSoftInput(this.jamme.et, 0);//InputMethodManager.SHOW_FORCED);
									this.jamme.keyboardActive = true;
								} else {
									//getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
									im.HideSoftInputFromWindow(this.jamme.view.WindowToken, 0);
									this.jamme.keyboardActive = false;
								}
							} else {
								Log.Error(LOG, "IMM failed");
							}
						});
					} else if ((((this.jamme.flags ^ notifiedflags) & DEMO_PAUSED) != 0)
//						|| ((flags ^ notifiedflags) & DEMO_CUTTING) != 0
						) {
						Log.Debug(LOG, "seekbar");
						int fl = this.jamme.flags;
						this.jamme.RunOnUiThread(() => {
							//always draw demo cut inteface
							if ((fl & DEMO_PAUSED) != 0) {
								if (this.jamme.sb == null) {
									this.jamme.sb = new jaMMESeekBar(this.jamme);
									this.jamme.AddContentView(this.jamme.sb, this.jamme.sb.LayoutParameters);
								} else {
									this.jamme.sb.Visibility = ViewStates.Visible;
								}
								if (this.jamme.rsb == null) {
									this.jamme.rsb = new jaMMERangeSeekBar(new Integer(0), new Integer(jaMME.demoGetLength()), this.jamme);
									this.jamme.AddContentView(this.jamme.rsb, this.jamme.rsb.LayoutParameters);
								} else {
									this.jamme.rsb.Visibility = ViewStates.Visible;
								}
							} else {
								if (this.jamme.sb != null) {
									this.jamme.sb.Visibility = ViewStates.Invisible;
								}
								if (this.jamme.rsb != null) {
									this.jamme.rsb.Visibility = ViewStates.Invisible;
								}
							}
						});
					} else if (((this.jamme.flags ^ notifiedflags) & DEMO_PLAYBACK) != 0) {
						Log.Debug(LOG,"seekbar reset");
						this.jamme.RunOnUiThread(() => {
							if (this.jamme.sb != null) {
								ViewGroup vg = (ViewGroup)this.jamme.sb.Parent;
								if (vg != null)
									vg.RemoveView(this.jamme.sb);
								this.jamme.sb.Dispose();
								this.jamme.sb = null;
							}
							if (this.jamme.rsb != null) {
								ViewGroup vg = (ViewGroup)this.jamme.rsb.Parent;
								if (vg != null)
									vg.RemoveView(this.jamme.rsb);
								this.jamme.rsb.Dispose();
								this.jamme.rsb = null;
							}
						});
					}
					notifiedflags = this.jamme.flags;
				}
				if ((this.jamme.flags & DEMO_PAUSED) != 0 && this.jamme.sb != null)
					this.jamme.sb.Progress = jaMME.demoGetTime();
				this.jamme.vibrate();
			}
		}
		public override bool OnKeyDown(Keycode keyCode, KeyEvent ev) {
			int uc = 0;
			if (ev !=null)
				uc = ev.UnicodeChar;
			if (keyCode == Keycode.VolumeDown) {
				jaMME.quickCommand("chase targetNext");
			} else if (keyCode == Keycode.VolumeUp) {
				jaMME.quickCommand("chase targetPrev");
			} else/* if (keyCode == Keycode.Escape
				   || keyCode == Keycode.Back
				   || keyCode == Keycode.Menu
				   || keyCode == Keycode.Del)*/ {
				if (((this.flags & CONSOLE_ACTIVE) != 0 || (this.flags & CONSOLE_ACTIVE_FULLSCREEN) != 0)
						&& keyCode == Keycode.Back) {
					jaMME.quickCommand("toggleconsole");
					return true;
				} else if ((this.flags & CONSOLE_ACTIVE) != 0 && keyCode == Keycode.Menu) {
					jaMME.quickCommand("toggleconsole");
					return true;
				}
				jaMME.keypress(1, MobileKeycodes.GetMobileKey((int)keyCode, uc), uc);
				jaMME.keypress(0, MobileKeycodes.GetMobileKey((int)keyCode, uc), uc);
				if (keyCode == Keycode.Del && this.et != null) {
					this.et.Text = "";
				}
			}/* else {
				return base.OnKeyDown(keyCode, ev); 
			}*/
			return true;
		}
		private string PasteFromClipboard() {
			using (var clipboard = (Android.Content.ClipboardManager)GetSystemService(Context.ClipboardService)) {
				string paste = null;
				if (clipboard != null
						&& clipboard.HasPrimaryClip
						&& clipboard.PrimaryClipDescription != null
						&& clipboard.PrimaryClipDescription.HasMimeType(ClipDescription.MimetypeTextPlain)
						&& clipboard.PrimaryClip != null
						&& clipboard.PrimaryClip.GetItemAt(0) != null) {
					var item = clipboard.PrimaryClip.GetItemAt(0);
					if (item.Text != null) {
						paste = item.Text;
					}
				}
				return paste;
			}
		}
		private static int LONG_PRESS_TIME = 717;
		Java.Lang.Runnable _longPressed;
		private float touchX = 0, touchY = 0;
		private float touchYtotal = 0;
		private bool touchMotion = false, longPress = false, textPasted = false, twoPointers = false;
		public override bool OnTouchEvent(MotionEvent ev) {
			var action = ev.Action;
			var actionCode = action & MotionEventActions.Mask;
			int pointerCount = ev.PointerCount;
			if (pointerCount == 2) {
				twoPointers = true;
			}
			if (actionCode == MotionEventActions.Down) {
				this.touchX = ev.GetX();
				this.touchY = ev.GetY();
				_handler.PostDelayed(_longPressed, LONG_PRESS_TIME);
			} else if (actionCode == MotionEventActions.Move && !twoPointers) {
				float d = density;
				float deltaX = (ev.GetX() - touchX) / surfaceWidth;
				float deltaY = (ev.GetY() - touchY) / surfaceHeight;
				float adx = System.Math.Abs(deltaX*1.7f*d);
				float ady = System.Math.Abs(deltaY*1.7f*d);
				adx *= 300;
				ady *= 300;
				if (!touchMotion && longPress) {
					return true;
				} else if ((!touchMotion && (adx >= 4.5f || ady >= 4.5f))) {
					touchMotion = true;
					_handler.RemoveCallbacks(_longPressed);
				} else if (!touchMotion) {
					return true;
				}
				if ((flags & CONSOLE_ACTIVE) == 0 && (flags & CONSOLE_ACTIVE_FULLSCREEN) == 0) {
					mouseMove(deltaX*1.7f*d, deltaY*1.7f*d);
				} else {
					touchYtotal += deltaY*23* d*2;
					int tYt = (int)System.Math.Abs(touchYtotal);
					if (tYt > 1) {
						var qkey = JAKeycodes.A_NULL;
						//scroll the console
						if ((flags & CONSOLE_ACTIVE_FULLSCREEN) != 0) {
							if (deltaY< 0) {
								qkey = JAKeycodes.A_MWHEELDOWN;
							} else if (deltaY > 0) {
								qkey = JAKeycodes.A_MWHEELUP;
							}
							//scroll the console input history
						} else if ((flags & CONSOLE_ACTIVE) != 0) {
							if (deltaY< 0) {
								qkey = JAKeycodes.A_CURSOR_DOWN;
							} else if (deltaY > 0) {
								qkey = JAKeycodes.A_CURSOR_UP;
							}
							tYt = (int)(tYt / 1.7);
						}
						for (int i = 0; i<tYt; i++) {
							keypress(1, (int)qkey, 0);
							keypress(0, (int)qkey, 0);
						}
						touchYtotal -= (int)touchYtotal;
					}
				}
				touchX = ev.GetX();
				touchY = ev.GetY();
			} else if (actionCode == MotionEventActions.PointerUp && twoPointers) {
				if (!longPress) {
					quickCommand("toggleconsole");
				}
				touchMotion = true;
			} else if (actionCode == MotionEventActions.Up) {
				if (!touchMotion) {
					if ((flags & ~INGAME) == 0 && !longPress) {
						if ((flags & CHAT_SAY) == 0 && (flags & CHAT_SAY_TEAM) == 0) {
							quickCommand("messagemode");
						} else if ((flags & CHAT_SAY) != 0) {
							//hack: the 1st call resets chat, and the 2nd call actually calls team chat
							quickCommand("messagemode2; messagemode2");
						} else if ((flags & CHAT_SAY_TEAM) != 0) {
							keypress(1, (int)JAKeycodes.A_ESCAPE, 0);
							keypress(0, (int)JAKeycodes.A_ESCAPE, 0);
						}
					} else if ((flags & CONSOLE_ACTIVE) == 0 && (flags & CONSOLE_ACTIVE_FULLSCREEN) == 0) {
						if ((flags & DEMO_PLAYBACK) == 0) {
							if (!longPress) {
								keypress(1, (int)JAKeycodes.A_MOUSE1, 0);
								keypress(0, (int)JAKeycodes.A_MOUSE1, 0);
							}
						} else {
							quickCommand("pause");
						}
					}
				}
				if (longPress && !twoPointers)
					quickCommand("-scores");
				this.touchMotion = false;
				this.longPress = false;
				this.textPasted = false;
				this.twoPointers = false;
				this.touchYtotal = 0;
				this._handler.RemoveCallbacks(_longPressed);
			}
			return true;
		}
		private bool isServiceRunning(Type serviceClass) {
			ActivityManager manager = (ActivityManager)GetSystemService(Context.ActivityService);
			foreach (var service in manager.GetRunningServices(int.MaxValue)) {
				if (serviceClass.Name.Equals(service.Service.ClassName)) {
					Log.Debug("jaMME", "service " + serviceClass.Name + " has been already running");
					return true;
				}
			}
			Log.Debug("jaMME", "service " + serviceClass.Name + " is not yet started");
			return false;
		}

		public static void NullJavaObject(Java.Lang.Object obj) {
			if (obj != null) {
				obj.Dispose();
				obj = null;
			}
		}

		private string[] createArgs(string appArgs) {
			return appArgs.Split(new char[] { ' ' });
		}
		private static string[] demoExtList = {
			".dm_26",
			".dm_25",
			".mme",
			null
		};
	}

	public class BestEglChooser : Java.Lang.Object, GLSurfaceView.IEGLConfigChooser {
		private static readonly string LOG = "BestEglChooser";

		Context ctx;

		public BestEglChooser(Context ctx) {
			this.ctx = ctx;
		}
		public Javax.Microedition.Khronos.Egl.EGLConfig ChooseConfig(IEGL10 egl, Javax.Microedition.Khronos.Egl.EGLDisplay display) {
			Log.Info(LOG, "chooseConfig");


			int[][] mConfigSpec = new int[][] {
				new int[] {
					EGL10.EglRedSize, 8,
					EGL10.EglGreenSize, 8,
					EGL10.EglBlueSize, 8,
					EGL10.EglAlphaSize, 8,
					EGL10.EglDepthSize, 24,
					EGL10.EglStencilSize, 8,
					EGL10.EglNone
				},
				new int[] {
					EGL10.EglRedSize, 8,
					EGL10.EglGreenSize, 8,
					EGL10.EglBlueSize, 8,
					EGL10.EglAlphaSize, 8,
					EGL10.EglDepthSize, 24,
					EGL10.EglStencilSize, 8,
					EGL10.EglNone
				},
				new int[] {
					EGL10.EglRedSize, 8,
					EGL10.EglGreenSize, 8,
					EGL10.EglBlueSize, 8,
					EGL10.EglAlphaSize, 8,
					EGL10.EglDepthSize, 24,
					EGL10.EglStencilSize, 8,
					EGL10.EglNone
				},
				new int[] {
					EGL10.EglRedSize, 8,
					EGL10.EglGreenSize, 8,
					EGL10.EglBlueSize, 8,
					EGL10.EglAlphaSize, 8,
					EGL10.EglDepthSize, 24,
					EGL10.EglStencilSize, 8,
					EGL10.EglNone
				},
			};

			Log.Info(LOG, "Number of specs to test: " + mConfigSpec.Length);

			int specChosen;
			int[] num_config = new int[1];
			int numConfigs = 0;
			for (specChosen=0; specChosen<mConfigSpec.Length; specChosen++) {
				egl.EglChooseConfig(display, mConfigSpec[specChosen], null, 0, num_config);
				if (num_config[0] >0) {
					numConfigs =  num_config[0];
					break;
				}
			}

			if (specChosen ==mConfigSpec.Length) {
				throw new IllegalArgumentException(
						"No EGL configs match configSpec");
			}

			var configs = new Javax.Microedition.Khronos.Egl.EGLConfig[numConfigs];
			egl.EglChooseConfig(display, mConfigSpec[specChosen], configs, numConfigs, num_config);

			string eglConfigsString = "";
			for (int n = 0; n<configs.Length; n++) {
				Log.Info(LOG, "found EGL config : " + printConfig(egl, display, configs[n]));
				eglConfigsString += n + ": " +  printConfig(egl, display, configs[n]) + ",";
			}
			Log.Info(LOG, eglConfigsString);
			//		AppSettings.setStringOption(ctx, "egl_configs", eglConfigsString);


			int selected = 0;

			int overrideConfig = 0;//AppSettings.getIntOption(ctx, "egl_config_selected", 0);
			if (overrideConfig < configs.Length) {
				selected = overrideConfig;
			}

			// best choice : select first config
			Log.Info(LOG, "selected EGL config[" + selected + "]: " + printConfig(egl, display, configs[selected]));

			return configs[selected];
		}

		private string printConfig(IEGL10 egl, Javax.Microedition.Khronos.Egl.EGLDisplay display,
				Javax.Microedition.Khronos.Egl.EGLConfig config) {

			int r = findConfigAttrib(egl, display, config,
					EGL10.EglRedSize, 0);
			int g = findConfigAttrib(egl, display, config,
					EGL10.EglGreenSize, 0);
			int b = findConfigAttrib(egl, display, config,
					EGL10.EglBlueSize, 0);
			int a = findConfigAttrib(egl, display, config,
					EGL10.EglAlphaSize, 0);
			int d = findConfigAttrib(egl, display, config,
					EGL10.EglDepthSize, 0);
			int s = findConfigAttrib(egl, display, config,
					EGL10.EglStencilSize, 0);

			/*
			 * 
			 * EGL_CONFIG_CAVEAT value 

		 #define EGL_NONE		       0x3038	
		 #define EGL_SLOW_CONFIG		       0x3050	
		 #define EGL_NON_CONFORMANT_CONFIG      0x3051	
			 */

			return string.Format("rgba={0}{1}{2}{3} z={4} sten={5}", r, g, b, a, d, s)
					+ " n=" + findConfigAttrib(egl, display, config, EGL10.EglNativeRenderable, 0)
					+ " b=" + findConfigAttrib(egl, display, config, EGL10.EglBufferSize, 0)
				    + string.Format(" c=0x{0:X04}", findConfigAttrib(egl, display, config, EGL10.EglConfigCaveat, 0))
					;

		}

		private int findConfigAttrib(IEGL10 egl, Javax.Microedition.Khronos.Egl.EGLDisplay display,
				Javax.Microedition.Khronos.Egl.EGLConfig config, int attribute, int defaultValue) {

			int[] mValue = new int[1];
			if (egl.EglGetConfigAttrib(display, config, attribute, mValue)) {
				return mValue[0];
			}
			return defaultValue;
		}
	}
}