package com.jamme;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.regex.Pattern;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

import org.libsdl.app.SDLActivity;

import com.jamme.DirectoryChooserDialog;
import com.jamme.R;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Vibrator;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.EGLConfigChooser;
import android.text.Editable;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

public class jaMME extends Activity {
	Vibrator v;
	float density = 1.0f;
	boolean gameReady = false;
	String gamePath, gameArgs = "", demoName = null;
	jaMMEView view;
	jaMMERenderer renderer = new jaMMERenderer();
	Activity act;
	jaMMEEditText et;
	jaMMESeekBar sb = null;
	jaMMERangeSeekBar rsb = null;
	int surfaceWidth,surfaceHeight;
//	int PORT_ACT_ATTACK = 13;
//	int PORT_ACT_ALT_ATTACK = 64;
	int flags = 0;
	int CONSOLE_ACTIVE				= 1<<2;
	int CONSOLE_ACTIVE_FULLSCREEN	= 1<<3;
	int UI_ACTIVE					= 1<<4;
	int UI_EDITINGFIELD				= 1<<5;
	int DEMO_PLAYBACK				= 1<<6;
	int DEMO_PAUSED					= 1<<7;
	int CHAT_SAY					= 1<<8;
	int CHAT_SAY_TEAM				= 1<<9;
	int INGAME						= ~(CONSOLE_ACTIVE
										| CONSOLE_ACTIVE_FULLSCREEN
										| UI_ACTIVE
										| UI_EDITINGFIELD | DEMO_PLAYBACK);
//	int DEMO_CUTTING				= 1<<17;
//	boolean demoCutting = false;
	boolean keyboardActive = false;
	static void loadLibraries() {
		System.loadLibrary("SDL2");
        System.loadLibrary("jamme");
    }
	public static native int init(Activity act,String[] args,String game_path,String lib_path,String app_path,String abi,String abi_alt);
	public static native void setScreenSize(int width, int height);	
	public static native int frame();
	public static native String getLoadingMsg();
	public static native void keypress(int down, int qkey, int unicode);
	public static native void textPaste(byte []paste);
	public static native void doAction(int state, int action);
	public static native void analogPitch(int mode,float v);
	public static native void analogYaw(int mode,float v);
	public static native void mouseMove(float dx, float dy);
	public static native void quickCommand(String command);
	/* DEMO STUFF */
	public static native int demoGetLength();
	public static native int demoGetTime();
	/* FEEDBACK */
	public static native int vibrateFeedback();
	public void vibrate() {
		long time = vibrateFeedback();
		if (time > 5)
			v.vibrate(time);
	}
    @Override
    protected void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        act = this;
        v = (Vibrator)getSystemService(Context.VIBRATOR_SERVICE);
        density = getResources().getDisplayMetrics().density;
		// fullscreen
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
		// keep screen on 
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
				WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		loadLibraries();
		view = new jaMMEView(this);
		view.setEGLConfigChooser(new BestEglChooser(getApplicationContext()));
        view.setRenderer(renderer);
        view.setKeepScreenOn(true);
        setContentView(view);
        view.requestFocus();
        view.setFocusableInTouchMode(true);
        String[] parameters = {"","",""};
		try {
			parameters = loadParameters();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		String baseDirectory = "";
		if (parameters.length > 0)
			baseDirectory = parameters[0];
        String argsStr = "";
		if (parameters.length > 1)
			argsStr = parameters[1];
		boolean logSave = false;
		if (parameters.length > 2)
			logSave = Boolean.parseBoolean(parameters[2]);
		try {
			if (checkAssets(baseDirectory) && openDemo()) {
				gameReady = true;
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		if (!gameReady) {
			addLauncherItems(this, baseDirectory, argsStr, logSave);
		}
		act.setTheme(android.R.style.Theme_Holo);
    }
	@Override
	protected void onPause() {
		Log.i("jaMME", "onPause");
		super.onPause();
//		view.onPause();
		if (gameReady) {
			SDLActivity.nativePause();
		}
	}
	@Override
	protected void onResume() {
		Log.i("jaMME", "onResume");
    	super.onResume();
        view.onResume();
		if (gameReady) {
			SDLActivity.nativeResume();
		}
	}
	@Override
	protected void onRestart() {
		Log.i("jaMME", "onRestart");
		super.onRestart();
	}
	@Override
	protected void onStop() {
		Log.i("jaMME", "onStop");
		super.onStop();	
	}
	@Override
	protected void onDestroy() {
		Log.i("jaMME", "onDestroy");
		super.onDestroy();
		//stop demo to be able to remove currently opened demo file
		keypress(1, jk_keycodes.A_ESCAPE.ordinal(), 0);
		keypress(0, jk_keycodes.A_ESCAPE.ordinal(), 0);
		do {
			flags = frame();
		} while((flags & DEMO_PLAYBACK) != 0);
		
		if (gameReady) {
			SDLActivity.nativeQuit();
		}
		//if user closed the application from OS side on the demo playback after opening them externally
		if (gamePath != null && demoName != null) {
			int i = 0;
			while(demoExtList[i] != null) {
				String demosFolder = gamePath + "/base/demos/";
        		if (demoName.contains(".mme"))
        			demosFolder = gamePath + "/base/mmedemos/";
				File demo = new File(demosFolder + demoName + demoExtList[i]);
				Log.i("jaMME", "onDestroy: removing " + demo.toString());
				if (demo.exists()) {
					Log.i("jaMME", "onDestroy: removed");
			    	demo.delete();
				}
				i++;
			}
		}
		System.exit(0);
	}
	//gay way, but fastest
	public static boolean equals(final String s1, final String s2) {
		return s1.length() == s2.length() && s1.hashCode() == s2.hashCode();
	}
	private boolean openDemo() throws IOException {
		Log.i("jaMME", "openDemo");
		Intent intent = getIntent();
		if (intent == null) {
			return false;
		}
        String action = intent.getAction();
    	Uri uri = intent.getData();
        if (action != null && uri != null && action.compareTo(Intent.ACTION_VIEW) == 0) {
        	String demo = java.net.URLDecoder.decode((uri.getEncodedPath()), "UTF-8");
        	int i = 0;
        	while(demoExtList[i] != null) {
        		if (demo.contains(demoExtList[i]))
        			break;
        		i++;
			}
        	if (demoExtList[i] != null) {
        		String demosFolder = gamePath + "/base/demos/";
        		if (demo.contains(".mme"))
        			demosFolder = gamePath + "/base/mmedemos/";
		    	File from = new File(demo);
		    	File to = new File(demosFolder + from.getName());
		    	while (to.exists()) {
		        	to = new File(demosFolder + "_" + to.getName());
		    	}
		    	copyFile(from, to);
		    	demoName = to.getName();
		    	int pos = demoName.lastIndexOf(".");
		    	if (pos > 0) {
		    		demoName = demoName.substring(0, pos);
		    	}
				gameArgs = "+demo \"" + demoName + "\" del";
		    	return true;
        	}
        }
        return false;
	}
	public static void copyFile(File sourceFile, File destFile) throws IOException {
	    if (!destFile.exists()) {
	        destFile.createNewFile();
	    }
	    FileChannel source = null;
	    FileChannel destination = null;
	    try {
	        source = new FileInputStream(sourceFile).getChannel();
	        destination = new FileOutputStream(destFile).getChannel();
	        destination.transferFrom(source, 0, source.size());
	    } finally {
	        if (source != null) {
	            source.close();
	        }
	        if (destination != null) {
	            destination.close();
	        }
	    }
	}
	RelativeLayout layout;
	EditText args, baseDirET;
	Button startGame, baseDir;
	CheckBox logSave;
	private void addLauncherItems(Context context, String baseDirectory, String argsStr, boolean saveLog) {
		float d = density;
/* LAYOUT */
		Drawable bg = getResources().getDrawable(R.drawable.jamme);
		bg.setAlpha(0x17);
		layout = new RelativeLayout(this);
		layout.setBackgroundColor(Color.argb(0xFF, 0x13, 0x13, 0x13));
		layout.setBackground(bg);
/* ARGS */
		args = new EditText(context);
		args.setMaxLines(1);
		args.setInputType(EditorInfo.TYPE_CLASS_TEXT | EditorInfo.TYPE_TEXT_VARIATION_SHORT_MESSAGE);
		args.setHint("+startup args");
		args.setHintTextColor(Color.GRAY);
		args.setText(argsStr);
		args.setTextColor(Color.argb(0xFF, 0xC3, 0x00, 0x00));
		args.setBackground(getResources().getDrawable(R.drawable.jamme_edit_text));
		args.clearFocus();
		args.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN);
		args.setId(5);
		RelativeLayout.LayoutParams argsParams = 
		        new RelativeLayout.LayoutParams(
		        	ViewGroup.LayoutParams.MATCH_PARENT, 
		            RelativeLayout.LayoutParams.WRAP_CONTENT);
		argsParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
		argsParams.addRule(RelativeLayout.CENTER_VERTICAL);
/* START */
		startGame = new Button(context);
		startGame.setBackground(getResources().getDrawable(R.drawable.jamme_btn));
		startGame.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				String gp = "";
				if (baseDirET != null && baseDirET.getText() != null && baseDirET.getText().toString() != null)
					gp = baseDirET.getText().toString();
				if (checkAssets(gp)) {
					if (args != null && args.getText() != null && args.getText().toString() != null)
						gameArgs = args.getText().toString();
					logging = logSave.isChecked();
					String []parameters = {gp,gameArgs,String.valueOf(logging)};
					try {
						saveParameters(parameters);
					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					removeView(startGame);
					removeView(args);
					removeView(baseDirET);
					removeView(baseDir);
					removeView(logSave);
					removeView(layout);
					gameReady = true;
				} else {
					AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(act);
					alertDialogBuilder.setTitle("Missing assets[0-3].pk3");
					alertDialogBuilder
						.setMessage("Select base folder where assets[0-3].pk3 are placed in")
						.setNeutralButton("Ok", null);
					AlertDialog alertDialog = alertDialogBuilder.create();
					alertDialog.show();
				}
			}
		});
		startGame.setText("Start");
		RelativeLayout.LayoutParams startParams = 
		        new RelativeLayout.LayoutParams(
		            RelativeLayout.LayoutParams.WRAP_CONTENT,   
		            RelativeLayout.LayoutParams.WRAP_CONTENT);
		startParams.addRule(RelativeLayout.BELOW, args.getId());
		startParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
		startParams.setMargins(0, (int)(3.5f*d), 0, 0);
/* BASE DIRECTORY EDITABLE */
		baseDirET = new EditText(context);
		baseDirET.setMaxLines(1);
		baseDirET.setInputType(EditorInfo.TYPE_CLASS_TEXT | EditorInfo.TYPE_TEXT_VARIATION_SHORT_MESSAGE);
		baseDirET.setHint("game directory");
		baseDirET.setHintTextColor(Color.GRAY);
		baseDirET.setText(baseDirectory);
		baseDirET.setTextColor(Color.argb(0xFF, 0xC3, 0x00, 0x00));
		baseDirET.setBackground(getResources().getDrawable(R.drawable.jamme_edit_text));
		baseDirET.clearFocus();
		baseDirET.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN);
		baseDirET.setId(17);
		RelativeLayout.LayoutParams baseDirETParams = 
		        new RelativeLayout.LayoutParams(
		        	ViewGroup.LayoutParams.MATCH_PARENT, 
		            RelativeLayout.LayoutParams.WRAP_CONTENT);
		baseDirETParams.addRule(RelativeLayout.ABOVE, args.getId());
		baseDirETParams.addRule(RelativeLayout.CENTER_VERTICAL);
		baseDirETParams.setMargins(0, 0, 0, (int)(3.5f*d));
/* BASE DIRECTORY */
		baseDir = new Button(context);
		baseDir.setBackground(getResources().getDrawable(R.drawable.jamme_btn));
		baseDir.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				DirectoryChooserDialog directoryChooserDialog = 
					new DirectoryChooserDialog(act, new DirectoryChooserDialog.ChosenDirectoryListener() {
						@Override
						public void onChosenDir(String chosenDir) {
							if (chosenDir != null)
								baseDirET.setText(chosenDir);
							else
								baseDirET.setText("");
						}
					});
				String chooseDir = "";
				if (baseDirET != null && baseDirET.getText() != null && baseDirET.getText().toString() != null)
					chooseDir = baseDirET.getText().toString();
				directoryChooserDialog.chooseDirectory(chooseDir);
			}
		});
		baseDir.setText("Game Directory");
		RelativeLayout.LayoutParams baseDirParams = 
		        new RelativeLayout.LayoutParams(
		            RelativeLayout.LayoutParams.WRAP_CONTENT,   
		            RelativeLayout.LayoutParams.WRAP_CONTENT);
		baseDirParams.addRule(RelativeLayout.ABOVE, baseDirET.getId());
		baseDirParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
		baseDirParams.setMargins(0, 0, 0, (int)(3.5f*d));
/* LOGGING */
		logSave = new CheckBox(context);
		logSave.setChecked(saveLog);
		logSave.setButtonDrawable(getResources().getDrawable(R.drawable.jamme_checkbox));
		logSave.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				logging = isChecked;
			}
		});
		logSave.setText("Save Log");
		logSave.setTextColor(Color.argb(0xFF, 0xC3, 0x00, 0x00));
		RelativeLayout.LayoutParams logSaveParams = 
		        new RelativeLayout.LayoutParams(
		            RelativeLayout.LayoutParams.WRAP_CONTENT,   
		            RelativeLayout.LayoutParams.WRAP_CONTENT);
		logSaveParams.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
		logSaveParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
/* ADD TO LAYOUT */
		layout.addView(args, argsParams);
		layout.addView(startGame, startParams);
		layout.addView(baseDirET, baseDirETParams);
		layout.addView(baseDir, baseDirParams);
		layout.addView(logSave, logSaveParams);
		addContentView(layout, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
	}
	public static String getRemovableStorage() {
		String value = System.getenv("SECONDARY_STORAGE");
        if (!TextUtils.isEmpty(value)) {
            String[] paths = value.split(":");
            for (String path : paths) {
            	File file = new File(path);
                if (file.isDirectory()) {
                	return file.toString();
                }
            }
        }
        return null; // Most likely, a removable micro sdcard doesn't exist
	}
	private boolean checkAssets(String path) {
		if (path == null)
			return false;
		ArrayList<String> dirs = new ArrayList<String>();
		dirs.add(path);
		if (path.endsWith("/base")) {
			int l = path.lastIndexOf("/base");
			String baseRoot = path.substring(0, l);
			dirs.add(baseRoot);
		}
		for (Object d: dirs.toArray()) {
			File dir = new File((String)d + "/base");
			if(dir.exists() && dir.isDirectory()) {
				for (int i = 0; i < 3; i++) {
					File assets = new File((String)d + "/base/assets" + i + ".pk3");
					if (!assets.exists()) {
						return false;
					}
				}
				gamePath = (String)d;
				return true;
			}
		}
		return false;
	}
	private String []loadParameters() throws IOException {
		String []ret = {"",""};
		File f = new File(this.getFilesDir().toString() + "/parameters");
		if (f.exists()) {
			int length = (int)f.length();
			byte []buffer = new byte[length];
			FileInputStream in = new FileInputStream(f);
			try {
			    in.read(buffer);
			} finally {
			    in.close();
			}
			String s = new String(buffer);
			ret = s.split(Pattern.quote("\n\n\n\n\n"));
		}
		return ret;
	}
	private void saveParameters(String []parameters) throws IOException {
		File f = new File(this.getFilesDir().toString() + "/parameters");
		FileOutputStream stream = new FileOutputStream(f);
		try {
		    for (String p: parameters) {
		    	String s = p+"\n\n\n\n\n";
		    	stream.write(s.getBytes());
		    }
		} finally {
		    stream.close();
		}
	}
	boolean ignoreAfter = false;
	int prevLength = 0;
	class jaMMEEditText extends EditText {
		public jaMMEEditText(Context context) {
			super(context);
			setY(surfaceHeight + 32);
			setX(surfaceWidth + 32);
			ignoreAfter = false;
			prevLength = 0;
			setAlpha(0.0f);
			addTextChangedListener(new jaMMETextWatcher());
	        clearFocus();
	        setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN);
	        setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
	        setOnEditorActionListener(new EditText.OnEditorActionListener() {
	            @Override
	            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
	                if (event == null)
	                	return false;
	            	int uc = event.getUnicodeChar(event.getMetaState());
	        		int keyAction = event.getAction();
	        		int keyCode = event.getKeyCode();
	        		if (keyAction == KeyEvent.ACTION_DOWN) {
						if (keyCode == KeyEvent.KEYCODE_BACK) {
							v.clearFocus();
							if ((flags & CONSOLE_ACTIVE) != 0 || (flags & CONSOLE_ACTIVE_FULLSCREEN) != 0) {
	    						quickCommand("toggleconsole");
	    						return true;
							} else if ((flags & CHAT_SAY) != 0
									|| (flags & UI_EDITINGFIELD) != 0
									|| (flags & CHAT_SAY_TEAM) != 0) {
								keypress(1, jk_keycodes.A_ESCAPE.ordinal(), 0);
								keypress(0, jk_keycodes.A_ESCAPE.ordinal(), 0);
	    						return true;
							}
						}
						keypress(1, mapKey(keyCode, uc), uc);
						keypress(0, mapKey(keyCode, uc), uc);
						return true;
					}
	        		return false;
	            }
	        });
		}
		@Override
		public boolean onKeyPreIme(int keyCode, KeyEvent event) {
			if (event.getAction() == KeyEvent.ACTION_DOWN) {
				if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
					if ((flags & CONSOLE_ACTIVE) != 0) {
						quickCommand("toggleconsole");
					} else if ((flags & CHAT_SAY) != 0
							|| (flags & UI_EDITINGFIELD) != 0
							|| (flags & CHAT_SAY_TEAM) != 0) {
						keypress(1, jk_keycodes.A_ESCAPE.ordinal(), 0);
						keypress(0, jk_keycodes.A_ESCAPE.ordinal(), 0);
					}
				}
			}
			return super.dispatchKeyEvent(event);
		}
	}
	class jaMMESeekBar extends SeekBar {
		String LOG = "jaMME SeekBar";
    	public jaMMESeekBar(Context context) {
			super(context);
			float d = density;
			setMax(demoGetLength());
			setProgress(demoGetTime());
			setLeft(10);
			setY(surfaceHeight - 42*d);
			this.setPadding((int)(50*d), 0, (int)(50*d), 0);
			this.setThumbOffset((int)(50*d));
			setProgressDrawable(getResources().getDrawable(R.drawable.jamme_progress));
			setThumb(getResources().getDrawable(R.drawable.jamme_control));
			ViewGroup.LayoutParams lp = new ViewGroup.LayoutParams
					(ViewGroup.LayoutParams.MATCH_PARENT,
					ViewGroup.LayoutParams.WRAP_CONTENT);
			setLayoutParams(lp);
			setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
				@Override
				public void onProgressChanged(SeekBar seekBar, int progressValue, boolean fromUser) {
				  	Log.d(LOG, "onProgressChanged");
		            if (fromUser)
		            	quickCommand("seek "+(progressValue/1000.0));
				}
				@Override
				public void onStartTrackingTouch(SeekBar seekBar) {
				}
				@Override
				public void onStopTrackingTouch(SeekBar seekBar) {
				}
			});
		}
	}
	Integer lastMin = 0, lastMax = 0;
	class jaMMERangeSeekBar extends RangeSeekBar<Integer> {
		String LOG = "jaMME RangeSeekBar";
		short lastChange = 0;
		public jaMMERangeSeekBar(Integer min, Integer max, Context context) {
			super(min, max, (int)(50*density), context);
			float d = density;
//			setMax(demoGetLength());
			lastMin = 0;
			lastMax = this.getAbsoluteMaxValue();
//			setProgress(demoGetTime());
			setNotifyWhileDragging(true);
			setLeft(10);
			setY(surfaceHeight - 87*d);
			setPadding((int)(50*d), 0, (int)(50*d), 0);
//			this.setThumbOffset((int)(50*d));
//			setProgressDrawable(getResources().getDrawable(R.drawable.jamme_progress));
//			setThumb(getResources().getDrawable(R.drawable.jamme_control));
			ViewGroup.LayoutParams lp = new ViewGroup.LayoutParams
					(ViewGroup.LayoutParams.MATCH_PARENT,
					ViewGroup.LayoutParams.WRAP_CONTENT);
			setLayoutParams(lp);
			setOnRangeSeekBarChangeListener(new OnRangeSeekBarChangeListener<Integer>() {
				@SuppressLint("DefaultLocale")
				@Override
				public void onRangeSeekBarValuesChanged(RangeSeekBar<?> bar, Integer minValue, Integer maxValue) {
				  	Log.d(LOG, "onRangeSeekBarValuesChanged");
					if ((int)lastMin != (int)minValue) {
						quickCommand(String.format("seek %d.%03d", (minValue/1000), (minValue%1000)));
						lastMin = minValue;
						lastChange = -1;
					} else if ((int)lastMax != (int)maxValue) {
						quickCommand(String.format("seek %d.%03d", (maxValue/1000), (maxValue%1000)));
						lastMax = maxValue;
						lastChange = 1;
					}
				}
			});
		}
		@SuppressLint({ "ClickableViewAccessibility", "DefaultLocale" })
		@Override
		public boolean onTouchEvent(MotionEvent event) {
			int action = event.getAction();
			int actionCode = action & MotionEvent.ACTION_MASK;
			if (actionCode == MotionEvent.ACTION_UP) {
				if (lastChange < 0) {
					quickCommand(String.format("cut start %d.%03d", (lastMin/1000), (lastMin%1000)));
				} else if (lastChange > 0) {
					quickCommand(String.format("cut end %d.%03d", (lastMax/1000), (lastMax%1000)));
				}
				lastChange = 0;
//				return true;
			}
			return super.onTouchEvent(event);
		}
	}
    class jaMMEView extends GLSurfaceView {
    	String LOG = "jaMME View";
    	@SuppressLint("ClickableViewAccessibility")
		public jaMMEView(Context context) {
    		super(context);
            setFocusableInTouchMode(true);
    	}
        @Override
        public boolean onCheckIsTextEditor() {
            Log.d(LOG, "onCheckIsTextEditor");
            return true;
        }
        @Override
        public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
            Log.d(LOG, "onCreateInputConnection");
            BaseInputConnection fic = new BaseInputConnection(this, true);
            outAttrs.actionLabel = null;
            outAttrs.inputType = InputType.TYPE_CLASS_TEXT;
            outAttrs.imeOptions = (EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN);
            return fic;
        }
    }
	public static boolean isLatinLetter(char c) {
		return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	}
	public static boolean isDigit(char c) {
		return (c >= '0' && c <= '9');
	}
	class jaMMETextWatcher implements TextWatcher {
    	String LOG = "jaMME TextWatcher";
		@Override
		public void beforeTextChanged(CharSequence s, int start, int count, int after) {
			Log.d(LOG, "beforeTextChanged");
		}
		@Override
		public void onTextChanged(CharSequence s, int start, int before, int count) {
			Log.d(LOG, "onTextChanged");
		}
		@Override
		public void afterTextChanged(Editable s) {
			Log.d(LOG, "afterTextChanged");
			String ss = s.toString();
			if (ignoreAfter) {
				ignoreAfter = false;
				return;
			}
			if (ss == null)
				return;
			int l = ss.length();
			if (l < prevLength && l >= 0) {
//				int d = prevLength - l;
//				for (int i = d; i > 0; i--) {
					keypress(1, jk_keycodes.A_BACKSPACE.ordinal(), 0);
					keypress(0, jk_keycodes.A_BACKSPACE.ordinal(), 0);
//				}
				prevLength = 0;
				s.replace(0, s.length(), "");
			} else if (l > 0) {
				int d = l - prevLength;
				for (int i = d; i > 0; i--) {
					char ch = ss.toCharArray()[l-i];
					keypress(1, mapKey(0, ch), ch);
					keypress(0, mapKey(0, ch), ch);
				}
				prevLength = l;
			}
		}
    }
	ProgressDialog pd = null;
	String loadingMsg = "Loading...", lmLast = "Loading...";
	public static int LOADING_MESSAGE_TIME = 2000;
	public static int LOADING_MESSAGE_UPDATE_TIME = 1337;
	Runnable _loadingMessage = new Runnable() { 
	    public void run() {
	    	loadingMsg = getLoadingMsg();
			act.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					if (!jaMME.equals(loadingMsg, lmLast)) {
						lmLast = loadingMsg;
						if (pd != null)
							pd.dismiss();
						pd = null;
					}
					if (pd == null && !jaMME.equals(loadingMsg, "")) {
						pd = ProgressDialog.show(act, "", loadingMsg, true);
					}
				}
			});
	        _handler.postDelayed(_loadingMessage, LOADING_MESSAGE_UPDATE_TIME);
	    }   
	};
	boolean logging = false;
	public void saveLog() {
		if (!logging)
			return;
        try {
            String[] command = new String[] {"logcat", "-v", "threadtime", "-f", gamePath+"/jamme.log"};
            Process process = Runtime.getRuntime().exec(command);
/*            BufferedReader bufferedReader = 
              new BufferedReader(new InputStreamReader(process.getInputStream()));

            String line;
            while ((line = bufferedReader.readLine()) != null){
                if(line.contains(processId)) {
                    LogFileWriter.write(line);
                }
            }*/
        } catch (IOException ex) {
            Log.e("jaMME saveLog", "getCurrentProcessLog failed", ex);
        }
    }
	private void removeView(View v) {
		if (v != null) {
			ViewGroup vg = (ViewGroup)v.getParent();
			if (vg != null)
				vg.removeView(v);
			v = null;
		}
	}
    class jaMMERenderer implements GLSurfaceView.Renderer {
    	String LOG = "jaMME Renderer";
		@Override
		public void onSurfaceCreated(GL10 gl, EGLConfig config) {
			Log.d(LOG, "onSurfaceCreated");
		}
		private void initRenderer(int width, int height){
			saveLog();
			Log.i(LOG, "screen size : " + width + "x"+ height);		
			setScreenSize(width,height);
			Log.i(LOG, "Init start");
			//TODO: filter out fs_basepath from gameArgs 
			String[] args_array = createArgs("+set fs_basepath \""+gamePath+"\" "+gameArgs);
			//entTODO: need to test on different devices
			String abi = Build.CPU_ABI, abi_alt = Build.CPU_ABI2;
			init(act,args_array,gamePath,getApplicationInfo().nativeLibraryDir,act.getFilesDir().toString(),abi,abi_alt);
			Log.i(LOG, "Init done");
		}
		@Override
		public void onSurfaceChanged(GL10 gl, int width, int height) {
			//surface changed called on surface created so.....
			Log.d(LOG, String.format("onSurfaceChanged %dx%d", width,height) );
			surfaceWidth = width;
			surfaceHeight = height;
		}
		int notifiedflags = 0;
		boolean inited = false;
		@Override
		public void onDrawFrame(GL10 gl) {
			if (!gameReady)
				return;
	        _handler.postDelayed(_loadingMessage, LOADING_MESSAGE_TIME);
			if (!inited) {
				SDLActivity.nativeInit();
				inited = true;
				initRenderer(surfaceWidth, surfaceHeight);
			}
			flags = frame();
	        _handler.removeCallbacks(_loadingMessage);
			act.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					if (pd != null)
						pd.dismiss();
					pd = null;
				}
		    });
//			flags |= (demoCutting) ? (DEMO_CUTTING) : (0);
			if (flags != notifiedflags) {
				if (((flags ^ notifiedflags) & CONSOLE_ACTIVE) != 0
						|| ((flags ^ notifiedflags) & UI_EDITINGFIELD) != 0
						|| ((flags ^ notifiedflags) & CHAT_SAY) != 0
						|| ((flags ^ notifiedflags) & CHAT_SAY_TEAM) != 0) {
					Log.d(LOG,"keyboard");
					final int fl = flags;
					Runnable r = new Runnable() { //doing this on the ui thread because android sucks.
						public void run() {
							InputMethodManager im = (InputMethodManager)act.getSystemService(Context.INPUT_METHOD_SERVICE);
							if (im != null) {
								if ((!keyboardActive && (fl & CONSOLE_ACTIVE) != 0
										|| (fl & UI_EDITINGFIELD) != 0
										|| (fl & CHAT_SAY) != 0
										|| (fl & CHAT_SAY_TEAM) != 0)) {
									removeView(et);
									et = new jaMMEEditText(act);
							        addContentView(et, et.getLayoutParams());
							        et.requestFocus();
									et.requestFocusFromTouch();
									//getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
									im.showSoftInput(et, 0);//InputMethodManager.SHOW_FORCED);
									keyboardActive = true;
								} else {
									//getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
									im.hideSoftInputFromWindow(view.getWindowToken(), 0);
									keyboardActive = false;
								}
							} else {
								Log.e(LOG, "IMM failed");
							}
						}	
					};
					act.runOnUiThread(r);
				} else if ((((flags ^ notifiedflags) & DEMO_PAUSED) != 0)
//						|| ((flags ^ notifiedflags) & DEMO_CUTTING) != 0
						) {
					Log.d(LOG,"seekbar");
					final int fl = flags;
					Runnable r = new Runnable() { //doing this on the ui thread because android sucks.
						public void run() {
//always draw demo cut inteface
							if ((fl & DEMO_PAUSED) != 0) {
						        if (sb == null) {
									sb = new jaMMESeekBar(act);
									addContentView(sb, sb.getLayoutParams());
						        } else {
						        	sb.setVisibility(View.VISIBLE);
						        }
								if (rsb == null) {
							        rsb = new jaMMERangeSeekBar(0, demoGetLength(), act);
									addContentView(rsb, rsb.getLayoutParams());
								} else {
						        	rsb.setVisibility(View.VISIBLE);
						        }
							} else {
								if (sb != null) {
						        	sb.setVisibility(View.INVISIBLE);
						        }
								if (rsb != null) {
						        	rsb.setVisibility(View.INVISIBLE);
						        }
							}
						}	
					};
					act.runOnUiThread(r);
				} else if (((flags ^ notifiedflags) & DEMO_PLAYBACK) != 0) {
					Log.d(LOG,"seekbar reset");
					Runnable r = new Runnable() { //doing this on the ui thread because android sucks.
						public void run() {
							if (sb != null) {
								ViewGroup vg = (ViewGroup)sb.getParent();
								if (vg != null)
									vg.removeView(sb);
								sb = null;
							}
							if (rsb != null) {
								ViewGroup vg = (ViewGroup)rsb.getParent();
								if (vg != null)
									vg.removeView(rsb);
								rsb = null;
							}
						}	
					};
					act.runOnUiThread(r);
//					}
				}
				notifiedflags = flags;
			}
			if ((flags & DEMO_PAUSED) != 0 && sb != null)
				sb.setProgress(demoGetTime());
			vibrate();
		}   	
    }
	@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) { 	   
		int uc = 0;
		if (event !=null)
			uc = event.getUnicodeChar();
		if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
			quickCommand("chase targetNext");
		} else if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
			quickCommand("chase targetPrev");
		} else/* if (keyCode == KeyEvent.KEYCODE_ESCAPE
				   || keyCode == KeyEvent.KEYCODE_BACK
				   || keyCode == KeyEvent.KEYCODE_MENU
				   || keyCode == KeyEvent.KEYCODE_DEL)*/ {
			if (((flags & CONSOLE_ACTIVE) != 0 || (flags & CONSOLE_ACTIVE_FULLSCREEN) != 0)
					&& keyCode == KeyEvent.KEYCODE_BACK) {
				quickCommand("toggleconsole");
				return true;
			} else if ((flags & CONSOLE_ACTIVE) != 0 && keyCode == KeyEvent.KEYCODE_MENU) {
				quickCommand("toggleconsole");
				return true;
			}
			keypress(1, mapKey(keyCode, uc), uc);
			keypress(0, mapKey(keyCode, uc), uc);
			if (keyCode == KeyEvent.KEYCODE_DEL && et != null) {
				et.setText("");
			}
		}/* else {
			return super.onKeyDown(keyCode, event); 
		}*/
		return true;
	}
	private String PasteFromClipboard() {
		ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
		String paste = null;
		if (clipboard != null
				&& clipboard.hasPrimaryClip()
				&& clipboard.getPrimaryClipDescription() != null
				&& clipboard.getPrimaryClipDescription().hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN)
				&& clipboard.getPrimaryClip() != null
				&& clipboard.getPrimaryClip().getItemAt(0) != null) {
		    ClipData.Item item = clipboard.getPrimaryClip().getItemAt(0);
		    if (item.getText() != null) {
		    	paste = item.getText().toString();
		    }
		}
		return paste;
	}
	public static int LONG_PRESS_TIME = 717;
	final Handler _handler = new Handler(); 
	Runnable _longPressed = new Runnable() { 
	    public void run() {
	    	if (twoPointers) {
	    		quickCommand("screenshot");
	    	} else if ((flags & DEMO_PLAYBACK) != 0) {
	    		quickCommand("cut it");
	    	} else if (!textPasted && ((flags & CONSOLE_ACTIVE) != 0
	    			|| (flags & UI_EDITINGFIELD) != 0
	    			|| (flags & CHAT_SAY) != 0
	    			|| (flags & CHAT_SAY_TEAM) != 0)) {
				String paste = PasteFromClipboard();
				if (paste != null) {
					try {
						textPaste(paste.getBytes("US-ASCII"));
						textPasted = true;
					} catch (UnsupportedEncodingException e) {
						e.printStackTrace();
					}
				}
/*			} else if ((flags & DEMO_PAUSED) != 0) {
				if (demoCutting) {
					demoCutting = false;
				} else {
					demoCutting = true;
				}*/
			} else if (flags == 0) {
	    		quickCommand("+scores");
			}
	    	longPress = true;
	    }   
	};
	float touchX = 0, touchY = 0;
	float touchYtotal = 0;
	boolean touchMotion = false, longPress = false, textPasted = false, twoPointers = false;
	public boolean onTouchEvent(MotionEvent event) {
    	int action = event.getAction();
		int actionCode = action & MotionEvent.ACTION_MASK;
		int pointerCount = event.getPointerCount();
		if (pointerCount == 2) {
			twoPointers = true;
		}
		if (actionCode == MotionEvent.ACTION_DOWN) {
			touchX = event.getX();
			touchY = event.getY();
	        _handler.postDelayed(_longPressed, LONG_PRESS_TIME);
		} else if (actionCode == MotionEvent.ACTION_MOVE && !twoPointers) {
			float d = density;
			float deltaX = (event.getX() - touchX) / surfaceWidth;
			float deltaY = (event.getY() - touchY) / surfaceHeight;
			float adx = Math.abs(deltaX*1.7f*d);
			float ady = Math.abs(deltaY*1.7f*d);
			adx *= 300;
			ady *= 300;
			if (!touchMotion && longPress) {
				return true;
			} else if ((!touchMotion && (adx >= 4.5f || ady >= 4.5f))) {
				touchMotion = true;
		        _handler.removeCallbacks(_longPressed);
			} else if (!touchMotion) {
				return true;
			}
			if ((flags & CONSOLE_ACTIVE) == 0 && (flags & CONSOLE_ACTIVE_FULLSCREEN) == 0) {
				mouseMove(deltaX*1.7f*d, deltaY*1.7f*d);
			} else {
				touchYtotal += deltaY*23*d*2;
				int tYt = (int)Math.abs(touchYtotal);
				if (tYt > 1) {
					int qkey = 0;
					//scroll the console
					if ((flags & CONSOLE_ACTIVE_FULLSCREEN) != 0) {
						if (deltaY < 0) {
							qkey = jk_keycodes.A_MWHEELDOWN.ordinal();
						} else if (deltaY > 0) {
							qkey = jk_keycodes.A_MWHEELUP.ordinal();
						}
					//scroll the console input history
					} else if ((flags & CONSOLE_ACTIVE) != 0) {
						if (deltaY < 0) {
							qkey = jk_keycodes.A_CURSOR_DOWN.ordinal();
						} else if (deltaY > 0) {
							qkey = jk_keycodes.A_CURSOR_UP.ordinal();
						}
						tYt = (int)(tYt / 1.7);
					}
					for (int i = 0; i < tYt; i++) {
						keypress(1, qkey, 0);
						keypress(0, qkey, 0);
					}
					touchYtotal -= (int)touchYtotal;
				}
			}
			touchX = event.getX();
			touchY = event.getY();
		} else if (actionCode == MotionEvent.ACTION_POINTER_UP && twoPointers) {
			if (!longPress) {
				quickCommand("toggleconsole");
			}
			touchMotion = true;
    	} else if (actionCode == MotionEvent.ACTION_UP) {
        	if (!touchMotion) {
				if ((flags & ~INGAME) == 0 && !longPress) {
					if ((flags & CHAT_SAY) == 0 && (flags & CHAT_SAY_TEAM) == 0) {
						quickCommand("messagemode");
					} else if ((flags & CHAT_SAY) != 0) {
						//hack: the 1st call resets chat, and the 2nd call actually calls team chat
						quickCommand("messagemode2; messagemode2");
					} else if ((flags & CHAT_SAY_TEAM) != 0) {
						keypress(1, jk_keycodes.A_ESCAPE.ordinal(), 0);
						keypress(0, jk_keycodes.A_ESCAPE.ordinal(), 0);
					}
				} else if ((flags & CONSOLE_ACTIVE) == 0 && (flags & CONSOLE_ACTIVE_FULLSCREEN) == 0) {
					if ((flags & DEMO_PLAYBACK) == 0) {
						if (!longPress) {
							keypress(1, jk_keycodes.A_MOUSE1.ordinal(), 0);
							keypress(0, jk_keycodes.A_MOUSE1.ordinal(), 0);
						}
					} else {
						quickCommand("pause");
					}
				}
			}
			if (longPress && !twoPointers)
				quickCommand("-scores");
			touchMotion = false;
			longPress = false;
			textPasted = false;
			twoPointers = false;
			touchYtotal = 0;
	        _handler.removeCallbacks(_longPressed);
		}
		return true;
    }
    static public String[] createArgs(String appArgs) {
		ArrayList<String> a = new ArrayList<String>(Arrays.asList(appArgs.split(" ")));
		Iterator<String> iter = a.iterator();
		while (iter.hasNext()) {
			if (iter.next().contentEquals("")) {
				iter.remove();
			}
		}
		return a.toArray(new String[a.size()]);
	}
	public enum jk_keycodes {
		A_NULL ,
		A_SHIFT,
		A_CTRL,
		A_ALT,
		A_CAPSLOCK,
		A_NUMLOCK,
		A_SCROLLLOCK,
		A_PAUSE,
		A_BACKSPACE,
		A_TAB,
		A_ENTER,
		A_KP_PLUS,
		A_KP_MINUS,
		A_KP_ENTER,
		A_KP_PERIOD,
		A_PRINTSCREEN,
		A_KP_0,
		A_KP_1,
		A_KP_2,
		A_KP_3,
		A_KP_4,
		A_KP_5,
		A_KP_6,
		A_KP_7,
		A_KP_8,
		A_KP_9,
		A_CONSOLE,
		A_ESCAPE,
		A_F1,
		A_F2,
		A_F3,
		A_F4,

		A_SPACE,
		A_PLING,
		A_DOUBLE_QUOTE,
		A_HASH,
		A_STRING,
		A_PERCENT,
		A_AND,
		A_SINGLE_QUOTE,
		A_OPEN_BRACKET,
		A_CLOSE_BRACKET,
		A_STAR,
		A_PLUS,
		A_COMMA,
		A_MINUS,
		A_PERIOD,
		A_FORWARD_SLASH,
		A_0,
		A_1,
		A_2,
		A_3,
		A_4,
		A_5,
		A_6,
		A_7,
		A_8,
		A_9,
		A_COLON,
		A_SEMICOLON,
		A_LESSTHAN,
		A_EQUALS,
		A_GREATERTHAN,
		A_QUESTION,

		A_AT,
		A_CAP_A,
		A_CAP_B,
		A_CAP_C,
		A_CAP_D,
		A_CAP_E,
		A_CAP_F,
		A_CAP_G,
		A_CAP_H,
		A_CAP_I,
		A_CAP_J,
		A_CAP_K,
		A_CAP_L,
		A_CAP_M,
		A_CAP_N,
		A_CAP_O,
		A_CAP_P,
		A_CAP_Q,
		A_CAP_R,
		A_CAP_S,
		A_CAP_T,
		A_CAP_U,
		A_CAP_V,
		A_CAP_W,
		A_CAP_X,
		A_CAP_Y,
		A_CAP_Z,
		A_OPEN_SQUARE,
		A_BACKSLASH,
		A_CLOSE_SQUARE,
		A_CARET,
		A_UNDERSCORE,

		A_LEFT_SINGLE_QUOTE,
		A_LOW_A,
		A_LOW_B,
		A_LOW_C,
		A_LOW_D,
		A_LOW_E,
		A_LOW_F,
		A_LOW_G,
		A_LOW_H,
		A_LOW_I,
		A_LOW_J,
		A_LOW_K,
		A_LOW_L,
		A_LOW_M,
		A_LOW_N,
		A_LOW_O,
		A_LOW_P,
		A_LOW_Q,
		A_LOW_R,
		A_LOW_S,
		A_LOW_T,
		A_LOW_U,
		A_LOW_V,
		A_LOW_W,
		A_LOW_X,
		A_LOW_Y,
		A_LOW_Z,
		A_OPEN_BRACE,
		A_BAR,
		A_CLOSE_BRACE,
		A_TILDE,
		A_DELETE,

		A_EURO,
		A_SHIFT2,
		A_CTRL2,
		A_ALT2,
		A_F5,
		A_F6,
		A_F7,
		A_F8,
		A_CIRCUMFLEX,
		A_MWHEELUP,
		A_CAP_SCARON,
		A_MWHEELDOWN,
		A_CAP_OE,
		A_MOUSE1,
		A_MOUSE2,
		A_INSERT,
		A_HOME,
		A_PAGE_UP,
		A_RIGHT_SINGLE_QUOTE,
		A_LEFT_DOUBLE_QUOTE,
		A_RIGHT_DOUBLE_QUOTE,
		A_F9,
		A_F10,
		A_F11,
		A_F12,
		A_TRADEMARK,
		A_LOW_SCARON,
		A_SHIFT_ENTER,
		A_LOW_OE,
		A_END,
		A_PAGE_DOWN,
		A_CAP_YDIERESIS,

		A_SHIFT_SPACE,
		A_EXCLAMDOWN,
		A_CENT,
		A_POUND,
		A_SHIFT_KP_ENTER,
		A_YEN,
		A_MOUSE3,
		A_MOUSE4,
		A_MOUSE5,
		A_COPYRIGHT,
		A_CURSOR_UP,
		A_CURSOR_DOWN,
		A_CURSOR_LEFT,
		A_CURSOR_RIGHT,
		A_REGISTERED,
		A_UNDEFINED_7,
		A_UNDEFINED_8,
		A_UNDEFINED_9,
		A_UNDEFINED_10,
		A_UNDEFINED_11,
		A_UNDEFINED_12,
		A_UNDEFINED_13,
		A_UNDEFINED_14,
		A_UNDEFINED_15,
		A_UNDEFINED_16,
		A_UNDEFINED_17,
		A_UNDEFINED_18,
		A_UNDEFINED_19,
		A_UNDEFINED_20,
		A_UNDEFINED_21,
		A_UNDEFINED_22,
		A_QUESTION_DOWN,

		A_CAP_AGRAVE,
		A_CAP_AACUTE,
		A_CAP_ACIRCUMFLEX,
		A_CAP_ATILDE,
		A_CAP_ADIERESIS,
		A_CAP_ARING,
		A_CAP_AE,
		A_CAP_CCEDILLA,
		A_CAP_EGRAVE,
		A_CAP_EACUTE,
		A_CAP_ECIRCUMFLEX,
		A_CAP_EDIERESIS,
		A_CAP_IGRAVE,
		A_CAP_IACUTE,
		A_CAP_ICIRCUMFLEX,
		A_CAP_IDIERESIS,
		A_CAP_ETH,
		A_CAP_NTILDE,
		A_CAP_OGRAVE,
		A_CAP_OACUTE,
		A_CAP_OCIRCUMFLEX,
		A_CAP_OTILDE,
		A_CAP_ODIERESIS,
		A_MULTIPLY,
		A_CAP_OSLASH,
		A_CAP_UGRAVE,
		A_CAP_UACUTE,
		A_CAP_UCIRCUMFLEX,
		A_CAP_UDIERESIS,
		A_CAP_YACUTE,
		A_CAP_THORN,
		A_GERMANDBLS,

		A_LOW_AGRAVE,
		A_LOW_AACUTE,
		A_LOW_ACIRCUMFLEX,
		A_LOW_ATILDE,
		A_LOW_ADIERESIS,
		A_LOW_ARING,
		A_LOW_AE,
		A_LOW_CCEDILLA,
		A_LOW_EGRAVE,
		A_LOW_EACUTE,
		A_LOW_ECIRCUMFLEX,
		A_LOW_EDIERESIS,
		A_LOW_IGRAVE,
		A_LOW_IACUTE,
		A_LOW_ICIRCUMFLEX,
		A_LOW_IDIERESIS,
		A_LOW_ETH,
		A_LOW_NTILDE,
		A_LOW_OGRAVE,
		A_LOW_OACUTE,
		A_LOW_OCIRCUMFLEX,
		A_LOW_OTILDE,
		A_LOW_ODIERESIS,
		A_DIVIDE,
		A_LOW_OSLASH,
		A_LOW_UGRAVE,
		A_LOW_UACUTE,
		A_LOW_UCIRCUMFLEX,
		A_LOW_UDIERESIS,
		A_LOW_YACUTE,
		A_LOW_THORN,
		A_LOW_YDIERESIS,

		A_JOY0,
		A_JOY1,
		A_JOY2,
		A_JOY3,
		A_JOY4,
		A_JOY5,
		A_JOY6,
		A_JOY7,
		A_JOY8,
		A_JOY9,
		A_JOY10,
		A_JOY11,
		A_JOY12,
		A_JOY13,
		A_JOY14,
		A_JOY15,
		A_JOY16,
		A_JOY17,
		A_JOY18,
		A_JOY19,
		A_JOY20,
		A_JOY21,
		A_JOY22,
		A_JOY23,
		A_JOY24,
		A_JOY25,
		A_JOY26,
		A_JOY27,
		A_JOY28,
		A_JOY29,
		A_JOY30,
		A_JOY31,

		A_AUX0,
		A_AUX1,
		A_AUX2,
		A_AUX3,
		A_AUX4,
		A_AUX5,
		A_AUX6,
		A_AUX7,
		A_AUX8,
		A_AUX9,
		A_AUX10,
		A_AUX11,
		A_AUX12,
		A_AUX13,
		A_AUX14,
		A_AUX15,
		A_AUX16,
		A_AUX17,
		A_AUX18,
		A_AUX19,
		A_AUX20,
		A_AUX21,
		A_AUX22,
		A_AUX23,
		A_AUX24,
		A_AUX25,
		A_AUX26,
		A_AUX27,
		A_AUX28,
		A_AUX29,
		A_AUX30,
		A_AUX31,
		
		MAX_KEYS
	}
	public  int mapKey(int acode,  int unicode) {
		switch(acode) {
		case KeyEvent.KEYCODE_VOLUME_UP:
			return jk_keycodes.A_CAP_Q.ordinal();
		case KeyEvent.KEYCODE_VOLUME_DOWN:
			return jk_keycodes.A_CAP_E.ordinal();
		case KeyEvent.KEYCODE_ESCAPE:
		case KeyEvent.KEYCODE_BACK:
			return jk_keycodes.A_ESCAPE.ordinal();
		case KeyEvent.KEYCODE_MENU:
			return jk_keycodes.A_CONSOLE.ordinal();
		case KeyEvent.KEYCODE_TAB:
			return jk_keycodes.A_TAB.ordinal();
		case KeyEvent.KEYCODE_DPAD_CENTER:
		case KeyEvent.KEYCODE_ENTER:
			return jk_keycodes.A_ENTER.ordinal();
		case KeyEvent.KEYCODE_SPACE:
			return jk_keycodes.A_SPACE.ordinal();
		case KeyEvent.KEYCODE_DEL:
			return jk_keycodes.A_BACKSPACE.ordinal();
		case KeyEvent.KEYCODE_DPAD_UP:
			return jk_keycodes.A_CURSOR_UP.ordinal();
		case KeyEvent.KEYCODE_DPAD_DOWN:
			return jk_keycodes.A_CURSOR_DOWN.ordinal();
		case KeyEvent.KEYCODE_DPAD_LEFT:
			return jk_keycodes.A_CURSOR_LEFT.ordinal();
		case KeyEvent.KEYCODE_DPAD_RIGHT:
			return jk_keycodes.A_CURSOR_RIGHT.ordinal();
		case KeyEvent.KEYCODE_ALT_LEFT:
			return jk_keycodes.A_ALT.ordinal();
		case KeyEvent.KEYCODE_ALT_RIGHT:
			return jk_keycodes.A_ALT2.ordinal();
		case KeyEvent.KEYCODE_CTRL_LEFT:
			return jk_keycodes.A_CTRL.ordinal();
		case KeyEvent.KEYCODE_CTRL_RIGHT:
			return jk_keycodes.A_CTRL2.ordinal();
		case KeyEvent.KEYCODE_SHIFT_LEFT:
			return jk_keycodes.A_SHIFT.ordinal();
		case KeyEvent.KEYCODE_SHIFT_RIGHT:
			return jk_keycodes.A_SHIFT2.ordinal();
		case KeyEvent.KEYCODE_F1:
			return jk_keycodes.A_F1.ordinal();
		case KeyEvent.KEYCODE_F2:
			return jk_keycodes.A_F2.ordinal();
		case KeyEvent.KEYCODE_F3:
			return jk_keycodes.A_F3.ordinal();
		case KeyEvent.KEYCODE_F4:
			return jk_keycodes.A_F4.ordinal();
		case KeyEvent.KEYCODE_F5:
			return jk_keycodes.A_F5.ordinal();
		case KeyEvent.KEYCODE_F6:
			return jk_keycodes.A_F6.ordinal();
		case KeyEvent.KEYCODE_F7:
			return jk_keycodes.A_F7.ordinal();
		case KeyEvent.KEYCODE_F8:
			return jk_keycodes.A_F8.ordinal();
		case KeyEvent.KEYCODE_F9:
			return jk_keycodes.A_F9.ordinal();
		case KeyEvent.KEYCODE_F10:
			return jk_keycodes.A_F10.ordinal();
		case KeyEvent.KEYCODE_F11:
			return jk_keycodes.A_F11.ordinal();
		case KeyEvent.KEYCODE_F12:
			return jk_keycodes.A_F12.ordinal();	
		case KeyEvent.KEYCODE_FORWARD_DEL:
			return jk_keycodes.A_BACKSPACE.ordinal();
		case KeyEvent.KEYCODE_INSERT:
			return jk_keycodes.A_INSERT.ordinal();
		case KeyEvent.KEYCODE_PAGE_UP:
			return jk_keycodes.A_PAGE_UP.ordinal();
		case KeyEvent.KEYCODE_PAGE_DOWN:
			return jk_keycodes.A_PAGE_DOWN.ordinal();
		case KeyEvent.KEYCODE_MOVE_HOME:
			return jk_keycodes.A_HOME.ordinal();
		case KeyEvent.KEYCODE_MOVE_END:
			return jk_keycodes.A_END.ordinal();
		case KeyEvent.KEYCODE_BREAK:
			return jk_keycodes.A_PAUSE.ordinal();
		default:
			if (unicode < 128)
				return Character.toLowerCase(unicode);
		}
		return 0;
	}
	private static String []demoExtList = {
		".dm_26",
		".dm_25",
		".mme",
		null
	};
}
/*
class PathArgsActivity extends Activity {
	View view;
	EditText args;
	Button gamepath, start;
	boolean gameFound = false;
	@Override
	protected void onCreate(Bundle bundle) {
		super.onCreate(bundle);
		view = new View(this);
		setContentView(view);
		addContentView(gamepath, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

/* LAYOUT */
/*		RelativeLayout layout = new RelativeLayout(this);
        layout.setBackgroundColor(Color.BLACK);
        
/* ARGS */
/*		args.setId(2);
		RelativeLayout.LayoutParams argsParams = 
                new RelativeLayout.LayoutParams(
                    RelativeLayout.LayoutParams.WRAP_CONTENT, 
                    RelativeLayout.LayoutParams.WRAP_CONTENT);
        argsParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
        argsParams.addRule(RelativeLayout.CENTER_VERTICAL);
        Resources r = getResources();
        int px = (int)TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP, 200, r.getDisplayMetrics());
        args.setWidth(px);
        
/* GAME FOLDER */
/*		gamepath.setId(1);
		gamepath.setText("Game Folder");
        RelativeLayout.LayoutParams gamepathParams = 
                new RelativeLayout.LayoutParams(
                    RelativeLayout.LayoutParams.WRAP_CONTENT,   
                    RelativeLayout.LayoutParams.WRAP_CONTENT);
        gamepathParams.addRule(RelativeLayout.ABOVE, args.getId());
        gamepathParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
        gamepathParams.setMargins(0, 0, 0, 50);
        
/* START */
/*		start.setId(5);
		gamepath.setText("Start");
        RelativeLayout.LayoutParams startParams = 
                new RelativeLayout.LayoutParams(
                    RelativeLayout.LayoutParams.WRAP_CONTENT,   
                    RelativeLayout.LayoutParams.WRAP_CONTENT);
        startParams.addRule(RelativeLayout.BELOW, args.getId());
        startParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
        startParams.setMargins(0, 50, 0, 0);

        layout.addView(args, argsParams);
        layout.addView(gamepath, gamepathParams);
        layout.addView(start, startParams);
        setContentView(layout);
	}
}
*/
class BestEglChooser implements EGLConfigChooser {
	String LOG = "BestEglChooser";

	Context ctx;

	public BestEglChooser(Context ctx)
	{
		this.ctx = ctx;
	}


	public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {

		Log.i( LOG, "chooseConfig");


		int[][] mConfigSpec  = {	
				{
					EGL10.EGL_RED_SIZE, 8,
					EGL10.EGL_GREEN_SIZE, 8,
					EGL10.EGL_BLUE_SIZE, 8,
					EGL10.EGL_ALPHA_SIZE, 8,
					EGL10.EGL_DEPTH_SIZE, 24,
					EGL10.EGL_STENCIL_SIZE, 8,
					EGL10.EGL_NONE
				},{
					EGL10.EGL_RED_SIZE, 8,
					EGL10.EGL_GREEN_SIZE, 8,
					EGL10.EGL_BLUE_SIZE, 8,
					EGL10.EGL_ALPHA_SIZE, 8,
					EGL10.EGL_DEPTH_SIZE, 24,
					EGL10.EGL_STENCIL_SIZE, 0,
					EGL10.EGL_NONE
				},
				{
					EGL10.EGL_RED_SIZE, 8,
					EGL10.EGL_GREEN_SIZE, 8,
					EGL10.EGL_BLUE_SIZE, 8,
					EGL10.EGL_ALPHA_SIZE, 8,
					EGL10.EGL_DEPTH_SIZE, 16,
					EGL10.EGL_STENCIL_SIZE, 8,
					EGL10.EGL_NONE
				},{
					EGL10.EGL_RED_SIZE, 8,
					EGL10.EGL_GREEN_SIZE, 8,
					EGL10.EGL_BLUE_SIZE, 8,
					EGL10.EGL_ALPHA_SIZE, 8,
					EGL10.EGL_DEPTH_SIZE, 16,
					EGL10.EGL_STENCIL_SIZE, 0,
					EGL10.EGL_NONE
				},
		};

		Log.i(LOG,"Number of specs to test: " + mConfigSpec.length);

		int specChosen;
		int[] num_config = new int[1];
		int numConfigs=0;
		for (specChosen=0;specChosen<mConfigSpec.length;specChosen++)
		{
			egl.eglChooseConfig(display, mConfigSpec[specChosen], null, 0, num_config);
			if ( num_config[0] >0)
			{
				numConfigs =  num_config[0];
				break;
			}
		}

		if (specChosen ==mConfigSpec.length) {
			throw new IllegalArgumentException(
					"No EGL configs match configSpec");
		}

		EGLConfig[] configs = new EGLConfig[numConfigs];
		egl.eglChooseConfig(display, mConfigSpec[specChosen], configs, numConfigs,	num_config);

		String eglConfigsString = "";
		for(int n=0;n<configs.length;n++) {
			Log.i( LOG, "found EGL config : " + printConfig(egl,display,configs[n])); 
			eglConfigsString += n + ": " +  printConfig(egl,display,configs[n]) + ",";
		}
		Log.i(LOG,eglConfigsString);
//		AppSettings.setStringOption(ctx, "egl_configs", eglConfigsString);

			
		int selected = 0;
		
		int override = 0;//AppSettings.getIntOption(ctx, "egl_config_selected", 0);
		if (override < configs.length)
		{
			selected = override;
		}
			
		// best choice : select first config
		Log.i( LOG, "selected EGL config[" + selected + "]: " + printConfig(egl,display,configs[selected]));

		return configs[selected];
	}


	private  String printConfig(EGL10 egl, EGLDisplay display,
			EGLConfig config) {

		int r = findConfigAttrib(egl, display, config,
				EGL10.EGL_RED_SIZE, 0);
		int g = findConfigAttrib(egl, display, config,
				EGL10.EGL_GREEN_SIZE, 0);
		int b = findConfigAttrib(egl, display, config,
				EGL10.EGL_BLUE_SIZE, 0);
		int a = findConfigAttrib(egl, display, config,
				EGL10.EGL_ALPHA_SIZE, 0);
		int d = findConfigAttrib(egl, display, config,
				EGL10.EGL_DEPTH_SIZE, 0);
		int s = findConfigAttrib(egl, display, config,
				EGL10.EGL_STENCIL_SIZE, 0);

		/*
		 * 
		 * EGL_CONFIG_CAVEAT value 

     #define EGL_NONE		       0x3038	
     #define EGL_SLOW_CONFIG		       0x3050	
     #define EGL_NON_CONFORMANT_CONFIG      0x3051	
		 */

		return String.format("rgba=%d%d%d%d z=%d sten=%d", r,g,b,a,d,s)
				+ " n=" + findConfigAttrib(egl, display, config, EGL10.EGL_NATIVE_RENDERABLE, 0)
				+ " b=" + findConfigAttrib(egl, display, config, EGL10.EGL_BUFFER_SIZE, 0)
				+ String.format(" c=0x%04x" , findConfigAttrib(egl, display, config, EGL10.EGL_CONFIG_CAVEAT, 0))
				;

	}

	private int findConfigAttrib(EGL10 egl, EGLDisplay display,
			EGLConfig config, int attribute, int defaultValue) {

		int[] mValue = new int[1];
		if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
			return mValue[0];
		}
		return defaultValue;
	}

}
